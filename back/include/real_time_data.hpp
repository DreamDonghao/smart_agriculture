//
// Created by donghao on 25-10-23.
//
#ifndef REAL_TIME_DATA_HPP
#define REAL_TIME_DATA_HPP
#include <mutex>
#include <crow.h>
#include <nlohmann/json.hpp>

class RealTimeData {
public:
    RealTimeData() = default;

    ~RealTimeData() = default;

    void loadForJson(const crow::json::rvalue &data) {
        std::scoped_lock lock(m_mutex);
        m_temperature = data["temperature"].d();
        m_humidity = data["humidity"].d();
        m_ph = data["ph"].d();
        m_nitrogen = data["nitrogen"].d();
        m_phosphorus = data["phosphorus"].d();
        m_potassium = data["potassium"].d();
        m_light = data["light"].d();

        m_co2 = data["co2"].d();
        m_fanStatus = data["fan_status"].i();
        pump_status = data["pump_status"].i();

    }

    nlohmann::json getJson() const {
        std::scoped_lock lock(m_mutex);
        nlohmann::json json;
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
    mutable std::mutex m_mutex;
    /*
    temperature:float   # 温度
    humidity:float      # 湿度
    ph:float            # PH
    nitrogen:float      # 氮
    phosphorus:float    # 磷
    potassium:float     # 钾
    light:float         # 光照强度
    */
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


#endif //REAL_TIME_DATA_HPP
