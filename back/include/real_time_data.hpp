//
// Created by donghao on 25-10-23.
//
#ifndef REAL_TIME_DATA_HPP
#define REAL_TIME_DATA_HPP

#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <nlohmann/json.hpp>

class RealTimeData {
public:
    RealTimeData() = default;
    ~RealTimeData() = default;

    // 使用 nlohmann::json 加载数据
    void loadForJson(const nlohmann::json &data) {
        std::unique_lock lock(m_mutex);

        // 可选打印调试
        std::cout << data.dump() << std::endl;

        m_deviceId = data.value("device_id", "");
        m_temperature = data.value("temperature", 0.0);
        m_humidity = data.value("humidity", 0.0);
        m_ph = data.value("ph", 0.0);
        m_nitrogen = data.value("nitrogen", 0.0);
        m_phosphorus = data.value("phosphorus", 0.0);
        m_potassium = data.value("potassium", 0.0);
        m_light = data.value("light", 0.0);
        m_co2 = data.value("co2", 0.0);
        m_fanStatus = data.value("fan_status", 0);
        pump_status = data.value("pump_status", 0);
    }

    // 转成 nlohmann::json
    nlohmann::json getJson() const {
        std::shared_lock lock(m_mutex);

        nlohmann::json json;
        json["device_id"] = m_deviceId;
        json["temperature"] = m_temperature;
        json["humidity"] = m_humidity;
        json["ph"] = m_ph;
        json["nitrogen"] = m_nitrogen;
        json["phosphorus"] = m_phosphorus;
        json["potassium"] = m_potassium;
        json["light"] = m_light;
        json["co2"] = m_co2;
        json["fan_status"] = m_fanStatus;
        json["pump_status"] = pump_status;

        return json;
    }

private:
    mutable std::shared_mutex m_mutex;

    std::string m_deviceId;
    double m_temperature = 0.0;
    double m_humidity = 0.0;
    double m_co2 = 0.0;
    double m_ph = 0.0;
    double m_nitrogen = 0.0;
    double m_phosphorus = 0.0;
    double m_potassium = 0.0;
    double m_light = 0.0;
    int64_t m_fanStatus = 0;
    int64_t pump_status = 0;
};

#endif // REAL_TIME_DATA_HPP
