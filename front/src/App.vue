<script setup lang="ts">
  
  let back_url = "http://127.0.0.1:18080"
  import {onBeforeUnmount, onMounted, ref} from 'vue'
  
  let now_device_info = ref({
    id:"null",name:"null",details:"null"
  })
  
  async function getDeviceInfo(id:string){
    const res = await fetch(back_url+"/front/api/get_device_info/"+id,);
    try{
      if(!res.ok){
        throw new Error("error")
      }
      now_device_info.value = await res.json()
    }catch(err){
      console.log(err);
    }
  }
  
  let device_ids = ref<Array<string>>([]);
  let device_notes = ref<Map<string, string>>(new Map());
  let new_device_ids = ref<Array<string>>([]);
  function updateDevices(): void {
    const fetchData = async () => {
      try {
        const res = await fetch(back_url + '/front/api/get_all_device_ids');
        if (!res.ok) throw new Error('Network error');
        const json = await res.json();
        device_ids.value = json.devices;
        new_device_ids.value = json.new_deviceIds;
        device_notes.value.clear(); // 先清空旧数据
        for (let i = 0; i < json.devices.length; ++i) {
          const id = json.devices[i];
          const note = json.notes[i] || id;  // 防止没有 note
          device_notes.value.set(id, note);
        }
      } catch (err) {
        console.error('获取数据失败:', err);
      }
    }
    fetchData();
  }
  
  
  
  function selectDevice(id: string) {
    getDeviceInfo(id)
    fetchData() // 立即获取一次
    if (timer) clearInterval(timer)
    timer = setInterval(fetchData, 2000)
  }
  
  updateDevices();
  
  let device_note = ref<string>("")
  
  async function noteDevice() {
    const res = await fetch(back_url + "/front/api/note_device", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        device_id: now_device_info.value.id,
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
    if (now_device_info.value.id !== "null") {
      try {
        const res = await fetch(back_url + '/front/api/data', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            device_id: now_device_info.value.id   // ← 发送 device_id
          })
        })
        if (!res.ok) {
          throw new Error('Network error')
        }
        const json = await res.json()
        if (!device_ids.value.includes(now_device_info.value.id)) {
          device_ids.value.push(now_device_info.value.id)
        }
        data.value = json
      } catch (err) {
        console.error('获取数据失败:', err)
      }
    }
  }
  onMounted(() => {
    if (now_device_info.value.id !== "") {
      fetchData()
      timer = setInterval(fetchData, 2000)
    }
  })
  
  onBeforeUnmount(() => {
    if (timer) clearInterval(timer)
  })

</script>

<template>
  <div class="dashboard">
    <div class="button-column">
      <button @click="updateDevices" class="refresh-button">刷新设备</button>
      <h5>已添加设备:</h5>
      <button class="device-id-button" :class="{ active: id === now_device_info.id }"
              v-for="id in device_ids" :key="id" @click="selectDevice(id)">
        {{ device_notes.get(id) }}
      </button>
      <h5>新设备:</h5>
      <h6 v-for = "id in new_device_ids" >{{id}}</h6>
    </div>
    
    <div class="content-panel">
      <div v-if = "now_device_info.id != 'null'">
      <h2>🌿 环境监测面板</h2>
      <h3>设备ID: {{ now_device_info.id || '未选择设备' }}</h3>
      <h3>设备名称: {{ now_device_info.name || '未选择设备' }}</h3>
      <h3>详情: {{now_device_info.details || '无'}}</h3>
      <button>修改信息</button>
      <hr>
      </div>
      <div class="info-panel" v-if="data && now_device_info.id != 'null'">
        
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
    
    /* 侧边栏按钮栏的调整 (横向滚动) */
    .button-column {
      flex: 0 0 auto;
      width: 100%;
      border-right: none;
      border-bottom: 1px solid var(--border-color);
      
      /* 重点：确保它是横向 flex 容器 */
      display: flex;
      flex-direction: row;
      flex-wrap: nowrap;
      
      /* 设定最大高度并允许横向滚动，确保它只占据一行 */
      max-height: 80px; /* 明确设定最大高度，防止其过高 */
      overflow-x: auto;
      overflow-y: hidden; /* 隐藏垂直滚动 */
      
      justify-content: flex-start;
      padding: 0.5rem;
      -webkit-overflow-scrolling: touch;
    }
    
    /* 隐藏滚动条 */
    .button-column::-webkit-scrollbar {
      display: none;
    }
    
    .button-column {
      -ms-overflow-style: none;
      scrollbar-width: none;
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
