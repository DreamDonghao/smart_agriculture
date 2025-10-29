char device_id[] = "1234";

#include <atomic>
#include <cors.hpp>
#include <real_time_data.hpp>
#include <nlohmann/json.hpp>
#include <mysqlx/xdevapi.h>

int main() {
    mysqlx::Client client(
        "mysqlx://root:********@47.93.221.71:5173/:3306/sa",
        mysqlx::ClientOption::POOL_MAX_SIZE, 10
    );
    std::unordered_map<std::string, RealTimeData> deviceRealTimeDatas;

    crow::App<CORS> app;

    deviceRealTimeDatas[device_id];
    CROW_ROUTE(app, "/api/data").methods("POST"_method)
    ([&](const crow::request &req) {
        const auto body = crow::json::load(req.body);
        deviceRealTimeDatas[device_id].loadForJson(body);

        return crow::response();
    });

    CROW_ROUTE(app, "/api/data")([&] {
        crow::response res(deviceRealTimeDatas[device_id].getJson().dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });


    app.port(18080).multithreaded().run();

    return 0;
}
