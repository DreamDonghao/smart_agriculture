<script setup lang="ts">
  import {onBeforeUnmount, onMounted, ref} from 'vue'
  import {nowDeviceInfoStore} from "./stores/now_device_info.ts";
  import device_list from "./components/device_list.vue"
  import device_info from "./components/device_info.vue"
  
  let back_url = "http://127.0.0.1:18080"
  const now_device_info = nowDeviceInfoStore()
  
  function selectDevice(id: string) {
    fetchData() // 立即获取一次
    if (timer) clearInterval(timer)
    
  }
  
  async function noteDevice() {
    const res = await fetch(back_url + "/front/api/note_device", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        device_id: now_device_info.deviceID,
        device_note: device_note.value
      })
    })
    
    const data = await res.json()
    
    if (data.status === "exist") {
      alert("❗设备已存在：" + data.message)
    } else if (data.status === "inserted") {
      alert("✨新增成功：" + data.message)
    }
  }
  
  
  interface EnvData {
    device_id: string
    humidity: number
    co2: number
    ph: number
    nitrogen: number   // 新增
    phosphorus: number // 新增
    potassium: number  // 新增
    pump_status: number
    fan_status: number
  }
  
  const data = ref<EnvData | null>(null)
  let timer: ReturnType<typeof setInterval> | null = null
  
  const fetchData = async () => {
    if (now_device_info.deviceID !== "null") {
      try {
        const res = await fetch(back_url + '/front/api/data', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            device_id: now_device_info.deviceID   // ← 发送 device_id
          })
        })
        if (!res.ok) {
          throw new Error('Network error')
        }
        data.value = await res.json()
      } catch (err) {
        console.error('获取数据失败:', err)
      }
    }
  }
  onMounted(() => {
    timer = setInterval(fetchData, 2000)
    if (now_device_info.deviceID !== "") {
      fetchData()
      timer = setInterval(fetchData, 2000)
    }
  })
  
  onBeforeUnmount(() => {
    if (timer) clearInterval(timer)
  })
  
  const messages = ref([])
  const inputText = ref('')
  
  const sendMessage = async () => {
    if (!data.value) return // 没有数据就不发送
    
    // 拼接环境信息
    const dataString = `分析植物生张状况：湿度: ${data.value.humidity.toFixed(1)}%, ` +
        `二氧化碳: ${data.value.co2.toFixed(0)} ppm, ` +
        `pH: ${data.value.ph.toFixed(2)}, ` +
        `氮 (N): ${data.value.nitrogen.toFixed(1)} mg/L, ` +
        `磷 (P): ${data.value.phosphorus.toFixed(1)} mg/L, ` +
        `钾 (K): ${data.value.potassium.toFixed(1)} mg/L`
    
    
    // 用户消息显示
    messages.value.push({ from: '你', text: dataString })
    
    try {
      const response = await fetch(back_url + '/api/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: dataString }) // 注意字段名 chat
      })
      
      if (!response.ok) throw new Error('请求失败')
      
      const resData = await response.json()
      messages.value.push({ from: 'AI', text: resData.reply })
      
    } catch (err) {
      messages.value.push({ from: '系统', text: '发送失败' })
      console.error(err)
    }
  }

</script>

<template>
  <div class="dashboard">
    <device_list/>
    <div class="content-panel">
      <device_info/>
      <div class="info-panel" v-if="data && now_device_info.deviceID != 'null'">
        
        <div class="item">
          <strong>水泵状态</strong>
          <span :class="data.pump_status ? 'on' : 'off'">
            {{ data.pump_status ? '开启 💧' : '关闭 🚫' }}
          </span>
        </div>
        
        <div class="item">
          <strong>风扇状态</strong>
          <span :class="data.fan_status ? 'on' : 'off'">
            {{ data.fan_status ? '开启 💨' : '关闭 💤' }}
          </span>
        </div>
        
        <div class="item">
          <strong>湿度 (%)</strong>
          <span>{{ data.humidity.toFixed(1) }}</span>
        </div>
        
        <div class="item">
          <strong>二氧化碳浓度 (ppm)</strong>
          <span>{{ data.co2.toFixed(0) }}</span>
        </div>
        
        <div class="item">
          <strong>pH 值</strong>
          <span>{{ data.ph.toFixed(2) }}</span>
        </div>
        
        <div class="item">
          <strong>氮 (N) mg/L</strong>
          <span>{{ data.nitrogen.toFixed(1) }}</span>
        </div>
        
        <div class="item">
          <strong>磷 (P) mg/L</strong>
          <span>{{ data.phosphorus.toFixed(1) }}</span>
        </div>
        
        <div class="item">
          <strong>钾 (K) mg/L</strong>
          <span>{{ data.potassium.toFixed(1) }}</span>
        </div>
        
        <div class="dialog-container">
          <div class="chat-window">
            <div v-for="(msg, index) in messages" :key="index" class="message">
              <strong>{{ msg.from }}:</strong> {{ msg.text }}
            </div>
          </div>
          
          <div class="input-area">
            <input
                v-model="inputText"
                @keyup.enter="sendMessage"
                placeholder="输入消息..."
            />
            <button @click="sendMessage">发送</button>
          </div>
        </div>
      </div>
      
      <div v-else-if="now_device_info.id === 'null'">
        <p style="text-align: center; margin-top: 50px; color: #666;">请先选择或刷新设备列表。</p>
      </div>
    
    </div>
  </div>
</template>


<style scoped>
  /* ... :root 保持不变 ... */
  :root {
    --primary-color: #3498db;
    --success-color: #2ecc71;
    --danger-color: #e74c3c;
    --bg-sidebar: #f4f4f4;
    --bg-content: #ffffff;
    --bg-card: #f8f9fa;
    --border-color: #ddd;
    --font-family-base: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif;
    --sidebar-width: 220px;
  }
  
  /* 整体仪表板布局 */
  .dashboard {
    display: flex;
    flex-direction: row;
    height: 100vh;
    min-height: 100vh; /* 确保占据整个视口高度 */
    font-family: var(--font-family-base), serif;
    background-color: var(--bg-content);
  }
  
  /* 左侧边栏 (PC 端: 垂直滚动) */
  .button-column {
    flex: 0 0 var(--sidebar-width);
    background-color: var(--bg-sidebar);
    padding: 1rem;
    border-right: 1px solid var(--border-color);
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    overflow-y: auto;
  }
  
  /* 边栏中的按钮样式 (包括刷新和设备按钮) */
  .button-column button {
    width: 100%;
    margin: 6px 0;
    padding: 0.6rem 0.5rem;
    font-size: 1rem;
    border-radius: 8px;
    border: 1px solid var(--border-color);
    background-color: #ffffff;
    color: #333;
    cursor: pointer;
    transition: background-color 0.2s, box-shadow 0.2s;
    text-align: left;
    /* 确保按钮不会被 flex 挤压 */
    flex-shrink: 0;
  }
  
  /* 刷新按钮的额外样式 (PC 端) */
  .button-column .refresh-button {
    background-color: #f0f0f0;
    font-weight: bold;
    color: var(--primary-color);
    border: 1px solid var(--primary-color);
    margin-bottom: 12px;
  }
  
  .button-column .refresh-button:hover {
    background-color: #e0f0ff;
  }
  
  .button-column button:hover {
    background-color: #e0f7fa;
    border-color: var(--primary-color);
    box-shadow: 0 1px 4px rgba(0, 0, 0, 0.1);
  }
  
  /* 选中状态 */
  .button-column button.active {
    background-color: var(--primary-color);
    border-color: var(--primary-color);
    font-weight: bold;
  }
  
  
  /* 右侧内容面板 (PC 端: 占据剩余宽度，垂直滚动) */
  .content-panel {
    flex: 1;
    padding: 1.5rem;
    overflow-y: auto;
  }
  
  /* ... 标题、卡片等样式保持不变 ... */
  h2 {
    text-align: left;
    margin-bottom: 0.5rem;
    font-size: 2rem;
    color: #2c3e50;
    border-bottom: 2px solid var(--border-color);
    padding-bottom: 0.5rem;
  }
  
  h3 {
    text-align: left;
    margin-bottom: 1.5rem;
    font-size: 1.2rem;
    color: #555;
    font-weight: normal;
  }
  
  .info-panel {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 1.5rem;
  }
  
  .item {
    background: var(--bg-card);
    border-radius: 12px;
    padding: 1.5rem;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
    text-align: center;
    border: 1px solid transparent;
    transition: transform 0.2s, box-shadow 0.2s, border-color 0.2s;
  }
  
  .item:hover {
    transform: translateY(-3px);
    box-shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
    border-color: var(--primary-color);
  }
  
  .item strong {
    display: block;
    color: #555;
    font-size: 1rem;
    margin-bottom: 0.5rem;
  }
  
  .item span {
    font-weight: 700;
    font-size: 1.6rem;
  }
  
  span.on {
    color: var(--success-color);
  }
  
  span.off {
    color: var(--danger-color);
  }
  
  /* 手机端适配 (max-width: 768px) */
  @media (max-width: 768px) {
    .dashboard {
      flex-direction: column;
      height: auto; /* 允许根据内容自然撑开 */
      min-height: 100vh;
    }
    
    /* 移动端按钮样式 */
    .button-column button {
      flex: 0 0 auto;
      margin: 0 4px;
      padding: 0.4rem 0.8rem;
      font-size: 0.9rem;
      text-align: center;
      white-space: nowrap;
      width: auto; /* 宽度由内容决定 */
    }
    
    /* 移动端刷新按钮样式 */
    .button-column .refresh-button {
      margin-right: 8px; /* 与第一个设备按钮分隔 */
      margin-bottom: 0;
    }
    
    /* 右侧内容面板 (移动端: 占据剩余空间，垂直滚动) */
    .content-panel {
      flex: 1; /* 确保内容面板占据剩余的垂直空间 */
      padding: 1rem;
      overflow-y: auto;
      max-height: calc(100vh - 80px); /* 视口高度 - 侧边栏最大高度 */
    }
    
    h2 {
      font-size: 1.5rem;
      margin-bottom: 0.5rem;
    }
    
    h3 {
      font-size: 1rem;
      margin-bottom: 1rem;
    }
    
    .info-panel {
      grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
      gap: 1rem;
    }
    
    .item {
      padding: 1rem;
    }
    
    .item span {
      font-size: 1.3rem;
    }
  }
</style>
