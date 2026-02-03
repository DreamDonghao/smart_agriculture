import { defineStore } from "pinia"

export const nowDeviceInfoStore = defineStore("nowDeviceInfo", {
    state: () => ({
        deviceID: "",
        deviceName: "",
        deviceDetails: ""
    }),

    actions: {
        setDeviceID(deviceID: string) {
            this.deviceID = deviceID
        },
        setDeviceName(deviceName: string) {
            this.deviceName = deviceName
        },
        setDeviceDetails(deviceDetails: string) {
            this.deviceDetails = deviceDetails
        }
    }
})
