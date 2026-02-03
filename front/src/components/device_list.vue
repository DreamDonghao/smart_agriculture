<script setup lang="ts">
  import {nowDeviceInfoStore} from "../stores/now_device_info.ts";
  import {ref} from "vue";
  let back_url = "http://127.0.0.1:18080"
  const now_device_info = nowDeviceInfoStore()
  
  let device_ids = ref<Array<string>>([]);
  let devices_name = ref<Map<string, string>>(new Map());
  let new_device_ids = ref<Array<string>>([]);
  
  function updateDevices(): void {
    const fetchData = async () => {
      try {
        const res = await fetch(back_url + '/front/api/get_all_device_ids');
        if (!res.ok){
          console.error("Network error");
          return;
        }
        const json = await res.json();
        device_ids.value = json.devices;
        new_device_ids.value = json.new_deviceIds;
        devices_name.value.clear();
        for (let i = 0; i < json.devices.length; ++i) {
          const id = json.devices[i];
          const note = json.notes[i] || id;  // 防止没有 note
          devices_name.value.set(id, note);
        }
      } catch (err) {
        console.error('获取数据失败:', err);
      }
    }
    fetchData();
  }
  updateDevices();
  async function getDeviceInfo(id: string) {
    const res = await fetch(back_url + "/front/api/get_device_info/" + id,);
    try {
      if (!res.ok) {
       console.error("Network error");
      }
      let data = await res.json()
      now_device_info.setDeviceID(data["id"])
      now_device_info.setDeviceName(data["name"])
      now_device_info.setDeviceDetails(data["details"])
    } catch (err) {
      console.log(err);
    }
  }
  
  function selectDevice(id: string) {
    getDeviceInfo(id)
  }
</script>

<template>
  <div class=device_list>
    <button @click="updateDevices" class="refresh-button">刷新设备</button>
    <h5>已添加设备:</h5>
    <div class="device-name" :class="{ active: id === now_device_info.deviceID }"
         v-for="id in device_ids" :key="id" @click="selectDevice(id)">
      {{ devices_name.get(id) }}
    </div>
    <h5>新设备:</h5>
    <div class="device-id" v-for="id in new_device_ids">{{ id }}</div>
  </div>
</template>

<style scoped>
  .device_list{
    margin:0 10px 0 10px;
  }
  .device-name {
    margin: 10px 0 10px 0;
    border: 1px solid #000000;
    padding: 1px 1px 1px 1px;
    border-radius: 5px;
    background-color: #ffffff;
  }
  
  .device-name:hover {
    background: #b0ffe5;
  }
</style>