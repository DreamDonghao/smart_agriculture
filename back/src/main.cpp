#include <atomic>
#include <cors.hpp>
#include "crow/middlewares/cors.h"
#include <real_time_data.hpp>
#include <nlohmann/json.hpp>
#include <mysqlx/xdevapi.h>

int main() {
    try {
        // mysqlx::Client client(
        //     "mysqlx://root:********@47.93.221.71:5173/:3306/sa",
        //     mysqlx::ClientOption::POOL_MAX_SIZE, 10
        // );
        std::unordered_map<std::string, RealTimeData> deviceRealTimeDatas;

        crow::App<crow::CORSHandler> app;
        auto &cors = app.get_middleware<crow::CORSHandler>();
        cors.global()
                // .origin("http://127.0.0.1:5173")
                // .origin("http://localhost:5173")
                .origin("http://47.93.221.71:5173")
                // .origin("http://localhost:5173")
                .methods("GET"_method, "POST"_method, "OPTIONS"_method)
                .headers("Content-Type", "X-Custom-Header")
                .max_age(300)
                .allow_credentials();

        CROW_ROUTE(app, "/api/data").methods("POST"_method)
        ([&](const crow::request &req) {
            const auto body = crow::json::load(req.body);
            deviceRealTimeDatas[body["device_id"].s()].loadForJson(body);
            return crow::response();
        });

        CROW_ROUTE(app, "/front/api/data").methods("POST"_method)
        ([&](const crow::request &req) {
            const auto body = crow::json::load(req.body);
            crow::response res(deviceRealTimeDatas[body["device_id"].s()].getJson().dump());
            res.add_header("Content-Type", "application/json");
            return res;
        });


        CROW_ROUTE(app, "/front/api/get_all_device_ids")([&]() {
            crow::json::wvalue result;
            int i = 0;
            for (const auto &device_id: deviceRealTimeDatas | std::views::keys) {
                result["devices"][i++] = device_id;
            }
            return crow::response(result);
        });


        app.port(18080).multithreaded().run();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
