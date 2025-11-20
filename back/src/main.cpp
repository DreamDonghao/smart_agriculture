#include <atomic>
#include <cors.hpp>
#include "crow/middlewares/cors.h"
#include <real_time_data.hpp>
#include <nlohmann/json.hpp>
#include <mysqlx/xdevapi.h>

int main() {
    try {
        mysqlx::Client client(
            "mysqlx://argi:your_password@47.93.221.71:33060/smart_agriculture",
            mysqlx::ClientOption::POOL_MAX_SIZE, 10
        );
        std::unordered_map<std::string, RealTimeData> deviceRealTimeDatas;

        crow::App<crow::CORSHandler> app;
        auto &cors = app.get_middleware<crow::CORSHandler>();
        cors.global()
                .origin("http://127.0.0.1:5173")
                .origin("http://localhost:5173")
                .origin("http://47.93.221.71:5173")
                .origin("http://localhost:5173")
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
            auto session = client.getSession();
            crow::json::wvalue result;
            int i = 0;
            for (const auto &device_id: deviceRealTimeDatas | std::views::keys) {
                result["devices"][i] = device_id;
                auto res = session.sql(
                    "SELECT note "
                    "FROM device_notes "
                    "WHERE id = ?"
                ).bind(device_id).execute();
                auto row = res.fetchOne();
                if (row) {
                    result["notes"][i] = row[0].get<std::string>();
                } else {
                    result["notes"][i] = "";
                }
                ++i;
            }
            return crow::response(result);
        });


        CROW_ROUTE(app, "/front/api/note_device").methods("POST"_method)
        ([&](const crow::request &req) {
            auto session = client.getSession();

            const auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "Invalid JSON");
            }

            std::string device_id = body["device_id"].s();
            std::string note = body["device_note"].s();

            auto res = session.sql(
                "SELECT EXISTS("
                "   SELECT 1 "
                "   FROM device_notes "
                "   WHERE id = ?"
                ") AS has_value;"
            ).bind(device_id).execute();

            if (res.fetchOne()[0].get<bool>()) {
                crow::json::wvalue rsp;
                rsp["status"] = "exist";
                rsp["message"] = "设备已存在";
                return crow::response(200, rsp);
            } else {
                session.sql(
                    "INSERT INTO device_notes(id, note) VALUES (?, ?)"
                ).bind(device_id, note).execute();

                crow::json::wvalue rsp;
                rsp["status"] = "inserted";
                rsp["message"] = "已新增设备并记录备注";
                return crow::response(200, rsp);
            }
        });

        app.port(18080).multithreaded().run();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
