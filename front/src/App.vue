<template>
  <div class="dashboard">
    <h2>环境监测面板</h2>
    
    <div class="info-panel" v-if="data">
      <div class="item" v-for="(value, key) in data" :key="key">
        <strong>{{ labels[key] }}</strong>
        <span>{{ formatValue(value, key) }}</span>
      </div>
    </div>
    
    <div class="charts">
      <div
          v-for="(key, idx) in chartKeys"
          :key="key"
          class="chart-item"
          :ref="el => setChartRef(idx, el)"
      ></div>
    </div>
  </div>
</template>

<script setup lang="ts">
  import * as echarts from 'echarts'
  import {type ComponentPublicInstance, nextTick, onBeforeUnmount, onMounted, reactive, ref} from 'vue'
  
  interface EnvData {
    temperature: number
    humidity: number
    ph: number
    nitrogen: number
    phosphorus: number
    potassium: number
    light: number
  }
  
  // 当前数据
  const data = ref<EnvData | null>(null)
  
  // 响应式历史数据（确保每个键都有数组）
  const historyData = reactive({
    time: [] as string[],
    temperature: [] as number[],
    humidity: [] as number[],
    ph: [] as number[],
    nitrogen: [] as number[],
    phosphorus: [] as number[],
    potassium: [] as number[],
    light: [] as number[]
  })
  
  // 图表 DOM 引用
  const chartRefs = ref<(HTMLElement | null)[]>([])
  
  // 图表实例
  const chartInstances = ref<echarts.ECharts[]>([])
  
  // 需要绘图的字段
  const chartKeys: (keyof EnvData)[] = [
    'temperature', 'humidity', 'ph',
    'nitrogen', 'phosphorus', 'potassium', 'light'
  ]
  
  // 修改 1：单位更清晰（农业场景）
  const labels: Record<keyof EnvData, string> = {
    temperature: '温度 (°C)',
    humidity: '湿度 (%)',
    ph: '土壤 pH',
    nitrogen: '氮 N (mg/L)',
    phosphorus: '磷 P (mg/L)',
    potassium: '钾 K (mg/L)',
    light: '光照 (lux)'
  }
  
  // 修改 2：新增农业合理范围（Y轴用）
  const ranges: Record<keyof EnvData, { min: number; max: number; interval: number }> = {
    temperature: { min: 10, max: 40, interval: 5 },
    humidity: { min: 30, max: 100, interval: 10 },
    ph: { min: 4.0, max: 9.0, interval: 0.5 },
    nitrogen: { min: 0, max: 400, interval: 50 },
    phosphorus: { min: 0, max: 150, interval: 20 },
    potassium: { min: 0, max: 500, interval: 50 },
    light: { min: 0, max: 70000, interval: 10000 }
  }
  
  // 定时器引用
  let timer: ReturnType<typeof setInterval> | null = null
  
  // 正确设置 ref
  const setChartRef = (
      idx: number,
      el: Element | ComponentPublicInstance | null
  ) => {
    if (el instanceof HTMLElement) {
      chartRefs.value[idx] = el
    }
  }
  
  // 获取数据
  const fetchData = async () => {
    try {
      const res = await fetch('http://localhost:18080/api/data')
      if (!res.ok) throw new Error('Network error')
      const json: EnvData = await res.json()
      data.value = json
      
      const now = new Date().toLocaleTimeString('zh-CN', { hour12: false })
      historyData.time.push(now)
      
      for (const key of chartKeys) {
        historyData[key].push(json[key])
        if (historyData[key].length > 20) historyData[key].shift()
      }
      if (historyData.time.length > 20) historyData.time.shift()
      
      await nextTick()
      updateCharts()
    } catch (err) {
      console.error('获取数据失败:', err)
    }
  }
  
  // 更新所有图表
  const updateCharts = () => {
    chartKeys.forEach((key, idx) => {
      const chart = chartInstances.value[idx]
      const el = chartRefs.value[idx]
      if (!chart || !el) return
      
      const range = ranges[key]  // 使用合理范围
      
      chart.setOption({
        tooltip: { trigger: 'axis' },
        grid: { left: 50, right: 20, top: 30, bottom: 50 },
        xAxis: {
          type: 'category',
          data: historyData.time,
          axisLabel: { rotate: 30, fontSize: 11 }
        },
        // 修改 3：Y轴使用农业合理范围 + 刻度
        yAxis: {
          type: 'value',
          name: labels[key],
          nameLocation: 'middle',
          nameGap: 35,
          nameTextStyle: { fontSize: 12 },
          min: range.min,
          max: range.max,
          interval: range.interval,
          axisLabel: {
            formatter: (v: number) => {
              if (key === 'ph') return v.toFixed(1)
              if (key === 'light' && v >= 1000) return (v / 1000) + 'k'
              return v.toFixed(1)
            }
          }
        },
        series: [{
          name: labels[key],
          data: historyData[key],
          type: 'line',
          smooth: true,
          showSymbol: false,
          lineStyle: { width: 2 },
          itemStyle: { color: getColor(key) }
        }]
      }, true)
    })
  }
  
  // 颜色映射
  const getColor = (key: keyof EnvData): string => {
    const colors: Record<keyof EnvData, string> = {
      temperature: '#ff6b6b',
      humidity: '#4ecdc4',
      ph: '#45b7d1',
      nitrogen: '#96ceb4',
      phosphorus: '#feca57',
      potassium: '#ff9ff3',
      light: '#f8d568'
    }
    return colors[key]
  }
  
  // 格式化显示值
  const formatValue = (val: number, key: string): string => {
    if (key === 'ph') return val.toFixed(2)
    if (key === 'temperature' || key === 'humidity') return val.toFixed(1)
    if (key === 'light') return Math.round(val).toLocaleString()
    return val.toFixed(1)
  }
  
  // 生命周期
  onMounted(() => {
    nextTick(() => {
      // 初始化所有图表
      chartKeys.forEach((_, idx) => {
        const el = chartRefs.value[idx]
        if (el) {
          chartInstances.value[idx] = echarts.init(el)
        }
      })
      updateCharts()
    })
    
    fetchData()
    timer = setInterval(fetchData, 2000)
  })
  
  onBeforeUnmount(() => {
    if (timer) clearInterval(timer)
    
    chartInstances.value.forEach(chart => {
      if (chart && !chart.isDisposed()) {
        chart.dispose()
      }
    })
    chartInstances.value = []
  })
</script>

<style scoped>
  .dashboard {
    max-width: 1200px;
    margin: 0 auto;
    padding: 1.5rem;
    font-family: "Segoe UI", sans-serif;
  }
  
  h2 {
    text-align: center;
    margin-bottom: 1.5rem;
    color: #333;
    font-size: 1.8rem;
  }
  
  .info-panel {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
    gap: 1rem;
    margin-bottom: 2rem;
  }
  
  .item {
    background: #f8f9fa;
    border-radius: 12px;
    padding: 1rem;
    box-shadow: 0 2px 6px rgba(0,0,0,0.08);
    transition: transform 0.2s;
  }
  
  .item:hover {
    transform: translateY(-2px);
  }
  
  .item strong {
    display: block;
    color: #666;
    font-size: 0.9rem;
    margin-bottom: 0.3rem;
  }
  
  .item span {
    color: #222;
    font-weight: bold;
    font-size: 1.3rem;
  }
  
  .charts {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1.5rem;
  }
  
  .chart-item {
    height: 300px;
    width: 100%;
    background: #fff;
    border-radius: 12px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    overflow: hidden;
  }
  
  /* 响应式优化 */
  @media (max-width: 768px) {
    .charts {
      grid-template-columns: 1fr;
    }
    .chart-item {
      height: 250px;
    }
  }
</style>