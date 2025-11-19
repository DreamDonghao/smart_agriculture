#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

// ==== WiFi 配置 ====
const char *ssid = "name";          // WiFi 名
const char *password = "password";  // WiFi 密码

// ==== 后端 Crow 地址 ====
const char *serverURL = "http://47.93.221.71:18080/api/data";  // 服务器 IP + 路径

// ==== 上传周期 (毫秒) ====
const unsigned long interval = 5000;
unsigned long lastSendTime = 0;

// ==== 唯一设备 ID ====
String device_id;
//-----------------------------------------------------------------------------
// ====== OLED配置 ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== 土壤湿度传感器引脚定义 ======
#define SOIL_AO 25
#define SOIL_DO 2
#define PUMP_RELAY 4
const int SOIL_DRY_THRESHOLD = 3000;  // 土壤干燥阈值

// ====== 甲烷传感器引脚定义 ======
const int mqAOPin = 32;  // MQ的AO（模拟输出）
const int mqDOPin = 35;  // MQ的DO（数字输出）
const int fanPinA = 12;  // 风扇模块IN-A
const int fanPinB = 13;  // 风扇模块IN-B
int co2PPM = 0;          // 甲烷值显示为CO₂浓度

// ====== 语音导览系统引脚定义 ======
const int TRIG_PIN = 18;     // 超声波Trig
const int ECHO_PIN = 19;     // 超声波Echo
const int BUTTON_STOP = 5;   // 按键1 - 停止播报
const int BUTTON_PLAY = 34;  // 按键2 - 继续播报第二首
const int VOICE_PIN = 33;    // BY8301控制引脚

// ====== PH和NPK传感器RS485引脚定义 ======
#define DE_RE_PIN 2  // RS485 收发控制引脚（仅PH传感器使用，NPK为自动换向）
#define RS485_TX 17  // TTL转RS485模块DI引脚
#define RS485_RX 16  // TTL转RS485模块RO引脚

// ====== PH传感器配置 ======
unsigned char phCmd[8] = { 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x45, 0xDD };
String phData = "";
float phValue = 0.0;  // PH值存储变量

// ====== NPK传感器配置 ======
HardwareSerial RS485(2);  // 使用Serial2
const byte NPK_ADDR = 0x01;
const unsigned long NPK_TIMEOUT = 500;        // 等待应答时间
uint16_t nValue = 0, pValue = 0, kValue = 0;  // NPK值存储变量

// ====== 超声波参数 ======
const int DETECT_DISTANCE = 50;  // 检测距离50cm（半米）

// ====== 状态变量 ======
bool personDetected = false;
bool isWaitingChoice = false;
unsigned long choiceTime = 0;
const unsigned long TIME_OUT = 10000;  // 10秒超时
unsigned long lastDetectTime = 0;
const unsigned long DETECT_COOLDOWN = 30000;  // 30秒冷却

// ====== 按键防抖变量 ======
unsigned long lastButtonTime = 0;
const unsigned long BUTTON_DEBOUNCE = 500;  // 500ms防抖
bool button1Pressed = false;
bool button2Pressed = false;

// ====== 检测间隔变量 ======
const unsigned long GAS_CHECK_INTERVAL = 1000;   // 甲烷检测1秒一次
const unsigned long PH_CHECK_INTERVAL = 5000;    // PH检测5秒一次
const unsigned long SOIL_CHECK_INTERVAL = 2000;  // 土壤检测2秒一次
const unsigned long NPK_CHECK_INTERVAL = 3000;   // NPK检测3秒一次
const unsigned long OLED_UPDATE_INTERVAL = 500;  // OLED更新间隔500ms

// ====== 土壤状态变量 ======
int soilAOValue = 0;
bool isSoilDry = false;

void setup() {
  Serial.begin(115200);

  // ====== 初始化OLED ======
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("OLED初始化失败！");
      while (true)
        ;
    }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // 绘制固定文本框架
  display.setCursor(0, 0);
  display.print("土壤湿度: ");
  display.setCursor(0, 12);
  display.print("CO₂浓度: ");
  display.setCursor(0, 24);
  display.print("PH值: ");
  display.setCursor(0, 36);
  display.print("NPK: ");
  display.setCursor(0, 48);
  display.print("设备状态: ");
  display.display();

  // ====== 初始化RS485通信（PH和NPK共用） ======
  RS485.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  RS485.setTimeout(1000);  // 设置接收超时

  // 初始化PH传感器控制引脚
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);  // 初始为接收模式

  // ====== 初始化土壤湿度传感器引脚 ======
  pinMode(SOIL_AO, INPUT);
  pinMode(SOIL_DO, INPUT);
  pinMode(PUMP_RELAY, OUTPUT);

  // ====== 初始化甲烷传感器引脚 ======
  pinMode(mqAOPin, INPUT);
  pinMode(mqDOPin, INPUT);
  pinMode(fanPinA, OUTPUT);
  pinMode(fanPinB, OUTPUT);

  // ====== 初始化语音导览系统引脚 ======
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_STOP, INPUT_PULLUP);
  pinMode(BUTTON_PLAY, INPUT_PULLUP);
  pinMode(VOICE_PIN, OUTPUT);

  // ====== 初始状态设置 ======
  digitalWrite(fanPinA, LOW);
  digitalWrite(fanPinB, LOW);
  digitalWrite(VOICE_PIN, HIGH);
  digitalWrite(PUMP_RELAY, HIGH);  // 初始关闭水泵（继电器HIGH为关闭）

  // 初始读取按键状态
  button1Pressed = (digitalRead(BUTTON_STOP) == LOW);
  button2Pressed = (digitalRead(BUTTON_PLAY) == LOW);

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // 计算唯一设备 ID
  uint64_t chipid = ESP.getEfuseMac();
  char idBuffer[20];
  sprintf(idBuffer, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  device_id = String(idBuffer);
  Serial.println("Device ID: " + device_id);

  // 连接 WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  //-----------------------------------------------------------------------------

  Serial.println("草莓大棚智能系统启动中...");
  Serial.println("PH/NPK传感器引脚: RX=GPIO16，TX=GPIO17，DE/RE=GPIO2");
  Serial.println("包含：语音导览 + 甲烷检测 + 通风系统 + PH监测 + 自动灌溉 + NPK检测");
  Serial.println("按键1-停止播报, 按键2-继续播报");

  // 传感器预热
  Serial.println("传感器预热60秒...");
  display.setCursor(80, 0);
  display.print("预热中");
  display.display();
  delay(60000);

  Serial.println("所有系统启动完成！");
  display.setCursor(80, 0);
  display.print("就绪  ");
  display.display();
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int fan_status = 0;
int pump_status = 0;
float temperature = 0;
float humidity = 0;
float co2 = 0;
float ph = 0;
float nitrogen = 0;
float phosphorus = 0;
float potassium = 0;
float light = 0;
void sendSensorData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }
  // 构建 JSON
  StaticJsonDocument<512> doc;
  doc["device_id"] = device_id;      // 唯一设备 ID
  doc["temperature"] = temperature;  // 温度
  doc["humidity"] = humidity;        // 湿度
  doc["co2"] = co2;                  // 二氧化碳浓度（ppm）
  doc["ph"] = ph;                    // 酸碱度
  doc["nitrogen"] = nitrogen;        // 氮
  doc["phosphorus"] = phosphorus;    // 磷
  doc["potassium"] = potassium;      // 钾
  doc["light"] = light;              // 光照强度
  doc["fan_status"] = fan_status;    // 风扇状态（0关 1开）
  doc["pump_status"] = pump_status;  // 水泵状态（0关 1开）

  String payload;
  serializeJson(doc, payload);

  // HTTP POST
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.printf("HTTP %d: %s\n", httpCode, http.getString().c_str());
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  Serial.println("Sending payload: " + payload);
}

// 定义尝试连接间隔
const unsigned long wifiRetryInterval = 5000;
unsigned long lastWifiAttempt = 0;
//-----------------------------------------------------------------------------
void loop() {
  // ====== 语音导览系统功能 ======
  checkUltrasonic();  // 检测是否有人进入半米内
  checkButtons();     // 检测按键选择
  checkTimeout();     // 检查选择超时

  // ====== 甲烷检测与通风功能 ======
  checkGasSensor();  // 检测甲烷浓度并控制风扇

  // ====== PH传感器检测功能 ======
  checkPHSensor();  // 检测PH值

  // ====== 土壤湿度检测与灌溉功能 ======
  checkSoilMoisture();  // 检测土壤湿度并控制水泵

  // ====== NPK传感器检测功能 ======
  checkNPKSensor();  // 检测氮磷钾含量

  // ====== OLED显示更新 ======
  static unsigned long lastOLEDUpdate = 0;
  if (millis() - lastOLEDUpdate >= OLED_UPDATE_INTERVAL) {
    lastOLEDUpdate = millis();
    updateOLED();
  }

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiAttempt > wifiRetryInterval) {
    Serial.println("Trying to connect WiFi...");
    WiFi.begin(ssid, password);
    lastWifiAttempt = millis();
  }

  // ==== 循环上传 ====
  if (millis() - lastSendTime >= interval) {
    sendSensorData();
    lastSendTime = millis();
  }
  //-----------------------------------------------------------------------------

  delay(100);
}

// ====== PH传感器检测函数 ======
void checkPHSensor() {
  static unsigned long lastPHTime = 0;

  if (millis() - lastPHTime >= PH_CHECK_INTERVAL) {
    lastPHTime = millis();

    // 1. 发送读取指令
    digitalWrite(DE_RE_PIN, HIGH);  // 切换为发送模式
    delay(50);                      // 稳定发送状态
    for (int i = 0; i < 8; i++) {   // 发送完整指令帧
      RS485.write(phCmd[i]);
    }
    delay(50);                     // 确保指令发送完成
    digitalWrite(DE_RE_PIN, LOW);  // 切换为接收模式

    // 2. 接收返回数据
    delay(300);  // 等待传感器响应
    phData = "";
    Serial.print("\n[PH传感器] 原始返回数据（十六进制）：");
    while (RS485.available() > 0) {
      unsigned char in = RS485.read();
      phData += String(in, HEX);
      phData += ",";
      Serial.print(String(in, HEX));
      Serial.print(" ");
    }
    Serial.println();

    // 3. 解析并显示PH值
    if (phData.length() > 0) {
      float newValue = parseAndConvertPH(phData);
      if (newValue != -1) {
        phValue = newValue;
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        ph = phValue;
        //-----------------------------------------------------------------------------
        Serial.print("[PH传感器] 测量PH值：");
        Serial.println(phValue, 1);  // 保留1位小数

        if (phValue < 6.0) Serial.println("[PH传感器] PH值偏低，建议调整");
        else if (phValue > 7.5) Serial.println("[PH传感器] PH值偏高，建议调整");
        else Serial.println("[PH传感器] PH值正常");
      } else {
        Serial.println("[PH传感器] PH值解析失败，检查参数配置");
      }
    } else {
      Serial.println("[PH传感器] 未收到数据，检查接线或地址");
    }
  }
}

// ====== 解析PH数据 ======
float parseAndConvertPH(String data) {
  int commaPos;
  String parts[20];  // 存储分割后的十六进制数据
  int index = 0;

  // 分割字符串
  while ((commaPos = data.indexOf(',')) != -1 && index < 20) {
    parts[index++] = data.substring(0, commaPos);
    data = data.substring(commaPos + 1);
  }

  // 传感器返回帧格式：地址(1) + 功能码(1) + 数据长度(1) + 数据(8) + CRC(2)
  if (index >= 19) {
    // 转换十六进制到十进制
    int phHigh = strtol(parts[17].c_str(), NULL, 16);
    int phLow = strtol(parts[18].c_str(), NULL, 16);
    int phDecimal = phHigh * 256 + phLow;

    return phDecimal / 10.0;  // 计算实际PH值
  }
  return -1;  // 解析失败
}

// ====== NPK传感器检测函数 ======
void checkNPKSensor() {
  static unsigned long lastNPKTime = 0;

  if (millis() - lastNPKTime >= NPK_CHECK_INTERVAL) {
    lastNPKTime = millis();

    uint16_t n, p, k;
    if (readNPK(n, p, k)) {
      nValue = n;
      pValue = p;
      kValue = k;
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      nitrogen = static_cast<float>(n);
      phosphorus = static_cast<float>(p);
      potassium = static_cast<float>(k);
      //-----------------------------------------------------------------------------
      Serial.printf("\n[NPK传感器] 氮: %u mg/kg, 磷: %u mg/kg, 钾: %u mg/kg\n", n, p, k);

      // 可以根据NPK值进行相应提示
      if (n < 100) Serial.println("[NPK传感器] 氮含量偏低，建议施肥");
      else if (n > 300) Serial.println("[NPK传感器] 氮含量偏高");

      if (p < 50) Serial.println("[NPK传感器] 磷含量偏低，建议施肥");

      if (k < 100) Serial.println("[NPK传感器] 钾含量偏低，建议施肥");
    } else {
      Serial.println("\n[NPK传感器] 读取失败，检查供电/接线/A-B线序");
    }
  }
}

// ====== NPK传感器读取核心函数 ======
bool readNPK(uint16_t &n, uint16_t &p, uint16_t &k) {
  // 构建读取指令帧
  uint8_t tx[] = {
    NPK_ADDR,
    0x03,        // 读保持寄存器功能码
    0x00, 0x00,  // 起始地址
    0x00, 0x03,  // 读取3个寄存器
    0x00, 0x00   // CRC校验位（占位）
  };

  // 计算CRC16校验
  uint16_t crc = crc16(tx, 6);
  tx[6] = crc & 0xFF;
  tx[7] = crc >> 8;

  // 发送指令（NPK使用自动换向，无需控制DE/RE）
  RS485.write(tx, 8);
  RS485.flush();

  // 接收返回数据
  uint8_t rx[32] = { 0 };
  int idx = 0;
  unsigned long t = millis();
  while (millis() - t < NPK_TIMEOUT) {
    if (RS485.available()) {
      rx[idx++] = RS485.read();
      if (idx == 11) break;  // 预计接收11字节
    }
  }

  // 数据校验
  if (idx != 11) return false;                           // 长度校验
  if (rx[0] != NPK_ADDR || rx[1] != 0x03) return false;  // 地址和功能码校验

  // CRC校验
  uint16_t rcvCrc = (rx[10] << 8) | rx[9];
  if (rcvCrc != crc16(rx, 9)) return false;

  // 解析数据
  n = (rx[3] << 8) | rx[4];
  p = (rx[5] << 8) | rx[6];
  k = (rx[7] << 8) | rx[8];
  return true;
}

// ====== 计算Modbus CRC16校验 ======
uint16_t crc16(const uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= buf[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x01) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  return crc;
}

// ====== 土壤湿度检测函数 ======
void checkSoilMoisture() {
  static unsigned long lastSoilTime = 0;

  if (millis() - lastSoilTime >= SOIL_CHECK_INTERVAL) {
    lastSoilTime = millis();

    // 读取土壤湿度
    soilAOValue = analogRead(SOIL_AO);
    int soilDO = digitalRead(SOIL_DO);
    isSoilDry = (soilAOValue > SOIL_DRY_THRESHOLD) || (soilDO == HIGH);
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    humidity = static_cast<float>(soilAOValue);
    //-----------------------------------------------------------------------------
    // 湿度控制（水泵）
    if (isSoilDry) {
      digitalWrite(PUMP_RELAY, LOW);  // 打开水泵
      Serial.print("[灌溉] 土壤AO:");
      Serial.print(soilAOValue);
      Serial.print(" | DO:");
      Serial.print(soilDO);
      Serial.println(" | 状态:干燥 | 水泵:开启");
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      pump_status = 1;
      //-----------------------------------------------------------------------------
    } else {
      digitalWrite(PUMP_RELAY, HIGH);  // 关闭水泵
      Serial.print("[灌溉] 土壤AO:");
      Serial.print(soilAOValue);
      Serial.print(" | DO:");
      Serial.print(soilDO);
      Serial.println(" | 状态:湿润 | 水泵:关闭");
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      pump_status = 0;
      //-----------------------------------------------------------------------------
    }
  }
}

// ====== 甲烷检测函数（显示为CO₂） ======
void checkGasSensor() {
  static unsigned long lastGasTime = 0;

  if (millis() - lastGasTime >= GAS_CHECK_INTERVAL) {
    lastGasTime = millis();

    int mqAOValue = analogRead(mqAOPin);
    int mqDOState = digitalRead(mqDOPin);
    co2PPM = map(mqAOValue, 0, 4095, 400, 2000);  // 映射为CO₂浓度范围
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    co2 = static_cast<float>(mqAOValue);
    //-----------------------------------------------------------------------------
    // 直接根据DO值控制风扇
    if (mqDOState == 0) {
      // 气体超标，启动风扇
      digitalWrite(fanPinA, HIGH);
      digitalWrite(fanPinB, LOW);
      Serial.print("[甲烷] 浓度:");
      Serial.print(mqAOValue);
      Serial.println(" | 状态:超标 | 风扇:开启");
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      fan_status = 1;
      //-----------------------------------------------------------------------------
    } else {
      // 气体正常，关闭风扇
      digitalWrite(fanPinA, LOW);
      digitalWrite(fanPinB, LOW);
      Serial.print("[甲烷] 浓度:");
      Serial.print(mqAOValue);
      Serial.println(" | 状态:正常 | 风扇:关闭");
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      fan_status = 0;
      //-----------------------------------------------------------------------------
    }
  }
}

// ====== 语音导览系统函数 ======

// 超声波测距函数
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  if (duration == 0) {
    return 999;
  }
  long distance = duration * 0.034 / 2;

  return distance;
}

void checkUltrasonic() {
  long distance = getDistance();

  // 检测逻辑：距离小于50cm且在冷却时间外
  if (distance > 0 && distance < DETECT_DISTANCE && !personDetected && !isWaitingChoice && (millis() - lastDetectTime > DETECT_COOLDOWN)) {

    personDetected = true;
    lastDetectTime = millis();
    triggerWelcome();

    Serial.print("[导览] 检测到人员，距离: ");
    Serial.print(distance);
    Serial.println(" cm - 播放欢迎词");
  }
}

void triggerWelcome() {
  // 播放第一首曲目 - 欢迎光临
  playVoice(1);
  delay(6000);

  // 进入等待选择状态
  isWaitingChoice = true;
  choiceTime = millis();

  Serial.println("[导览] 等待按键选择：按键1停止，按键2继续播报");

  // 重置按键状态
  button1Pressed = (digitalRead(BUTTON_STOP) == LOW);
  button2Pressed = (digitalRead(BUTTON_PLAY) == LOW);
}

void checkButtons() {
  if (!isWaitingChoice) return;

  // 防抖处理
  if (millis() - lastButtonTime < BUTTON_DEBOUNCE) {
    return;
  }

  // 读取当前按键状态
  bool currentButton1 = (digitalRead(BUTTON_STOP) == LOW);
  bool currentButton2 = (digitalRead(BUTTON_PLAY) == LOW);

  // 检测按键1按下
  if (currentButton1 && !button1Pressed) {
    lastButtonTime = millis();
    button1Pressed = true;
    Serial.println("[导览] 按键1按下 - 停止播报");
    resetSystem();
    return;
  }

  // 检测按键2按下
  if (currentButton2 && !button2Pressed) {
    lastButtonTime = millis();
    button2Pressed = true;
    Serial.println("[导览] 按键2按下 - 继续播报");
    playSecondSong();
    return;
  }

  // 更新按键释放状态
  if (!currentButton1) button1Pressed = false;
  if (!currentButton2) button2Pressed = false;
}

void playSecondSong() {
  isWaitingChoice = false;
  Serial.println("[导览] 播放详细介绍");
  playVoice(2);
  delay(10000);
  resetSystem();
}

void resetSystem() {
  isWaitingChoice = false;
  personDetected = false;
  Serial.println("[导览] 系统重置，等待下次检测");
  Serial.println("--------------------------------");
}

// BY8301播放控制函数
void playVoice(int fileNumber) {
  Serial.print("[导览] 播放音频文件: ");
  Serial.println(fileNumber);

  for (int i = 0; i < fileNumber; i++) {
    digitalWrite(VOICE_PIN, LOW);
    delay(150);
    digitalWrite(VOICE_PIN, HIGH);
    delay(200);
  }
}

void checkTimeout() {
  if (isWaitingChoice && (millis() - choiceTime > TIME_OUT)) {
    Serial.println("[导览] 选择超时，自动结束");
    resetSystem();
  }
}

// ====== OLED显示更新函数 ======
void updateOLED() {
  // 只更新数值部分，不清除整个屏幕
  // 使用黑色背景覆盖旧数值，然后写入新数值

  // 土壤湿度值
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // 黑色文字，白色背景（覆盖）
  display.setCursor(65, 0);
  display.print(soilAOValue);
  display.print(" ");  // 清除多余字符
  display.print(isSoilDry ? "(干)" : "(湿)");

  // CO₂浓度值
  display.setCursor(65, 12);
  display.print(co2PPM);
  display.print(" PPM   ");

  // PH值
  display.setCursor(45, 24);
  display.print(phValue, 1);
  display.print("    ");

  // NPK值
  display.setCursor(45, 36);
  display.print(nValue);
  display.print("/");
  display.print(pValue);
  display.print("/");
  display.print(kValue);
  display.print("   ");

  // 设备状态
  display.setCursor(70, 48);
  bool fanState = (digitalRead(fanPinA) == HIGH);
  bool pumpState = isSoilDry;
  display.print("风扇:");
  display.print(fanState ? "开" : "关");
  display.print(" 水泵:");
  display.print(pumpState ? "开" : "关");

  // 恢复白色文字
  display.setTextColor(SSD1306_WHITE);

  display.display();
}