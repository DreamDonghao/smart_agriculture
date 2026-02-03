<script setup lang="ts">
  import {nowDeviceInfoStore} from "../stores/now_device_info.ts";
  const now_device_info = nowDeviceInfoStore()
  
</script>

<template>
  <div class="device-info">
    <div v-if="now_device_info.deviceID != 'null'">
      <h2>🌿 环境监测面板</h2>
      <h3>设备ID: {{ now_device_info.deviceID || '未选择设备' }}</h3>
      <h3>设备名称: {{ now_device_info.deviceName || '未选择设备' }}</h3>
      <h3>详情: {{ now_device_info.deviceDetails || '无' }}</h3>
      <button>修改信息</button>
      <hr>
    </div>
  </div>
</template>

<style scoped>

</style>