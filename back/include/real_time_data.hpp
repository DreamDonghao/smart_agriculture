//
// Created by donghao on 25-10-23.
//
#ifndef REAL_TIME_DATA_HPP
#define REAL_TIME_DATA_HPP
#include <atomic>
#include <crow.h>
#include <nlohmann/json.hpp>
class RealTimeData {
public:
    RealTimeData() = default;

    ~RealTimeData() = default;

    void loadForJson(const crow::json::rvalue &data) {
        m_temperature.store(data["temperature"].d());
        m_humidity.store(data["humidity"].d());
        m_ph.store(data["ph"].d());
        m_nitrogen.store(data["nitrogen"].d());
        m_phosphorus.store(data["phosphorus"].d());
        m_potassium.store(data["potassium"].d());
        m_light.store(data["light"].d());
    }
    nlohmann::json getJson() const {
        nlohmann::json json;
        json["temperature"] = m_temperature.load();
        json["humidity"] = m_humidity.load();
        json["ph"] = m_ph.load();
        json["nitrogen"] = m_nitrogen.load();
        json["phosphorus"] = m_phosphorus.load();
        json["potassium"] = m_potassium.load();
        json["light"] = m_light.load();
        return json;
    }
private:
    /*
    temperature:float   # 温度
    humidity:float      # 湿度
    ph:float            # PH
    nitrogen:float      # 氮
    phosphorus:float    # 磷
    potassium:float     # 钾
    light:float         # 光照强度
    */
    std::atomic<double> m_temperature;
    std::atomic<double> m_humidity;
    std::atomic<double> m_ph;
    std::atomic<double> m_nitrogen;
    std::atomic<double> m_phosphorus;
    std::atomic<double> m_potassium;
    std::atomic<double> m_light;
};


#endif //REAL_TIME_DATA_HPP
