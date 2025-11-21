#include <cors.hpp>
#include "crow/middlewares/cors.h"
#include <real_time_data.hpp>
#include <nlohmann/json.hpp>
#include <mysqlx/xdevapi.h>
static std::shared_mutex mtx;

int main() {
    try {
        mysqlx::Client client(
            "mysqlx://argi:your_password@47.93.221.71:33060/smart_agriculture",
            mysqlx::ClientOption::POOL_MAX_SIZE, 10
        );
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
        // 新连接的设备
        std::set<std::string> newDeviceIds;
        // 已添加设备
        std::set<std::string> addedDeviceIds;

        std::unordered_map<std::string, RealTimeData> deviceRealTimeDatas;

        for (auto res = client.getSession().sql("SELECT id FROM device_info;")
                     .execute(); const auto &row: res.fetchAll()) {
            auto id = row[0].get<std::string>();
            addedDeviceIds.insert(id);
            deviceRealTimeDatas[id];
        }


        CROW_ROUTE(app, "/api/data").methods("POST"_method)
        ([&](const crow::request &req) {
            std::shared_lock lock(mtx);
            const auto body = crow::json::load(req.body);
            if (const std::string device_id = body["device_id"].s(); addedDeviceIds.contains(device_id)) {
                deviceRealTimeDatas[device_id].loadForJson(body);
            } else {
                newDeviceIds.insert(device_id);
            }
            return crow::response();
        });

        CROW_ROUTE(app, "/front/api/data").methods("POST"_method)
        ([&deviceRealTimeDatas](const crow::request &req) {
            std::shared_lock lock(mtx);
            const auto body = crow::json::load(req.body);
            crow::response res(deviceRealTimeDatas[body["device_id"].s()].getJson().dump());
            res.add_header("Content-Type", "application/json");
            std::cout << res.body << std::endl;
            return res;
        });

        CROW_ROUTE(app, "/front/api/get_all_device_ids")([&]() {
            std::shared_lock lock(mtx);
            auto session = client.getSession();
            crow::json::wvalue result;
            int i = 0;
            for (const auto &device_id: addedDeviceIds) {
                result["devices"][i] = device_id;
                auto res = session.sql(
                    "SELECT name "
                    "FROM device_info "
                    "WHERE id = ?"
                ).bind(device_id).execute();
                if (auto row = res.fetchOne()) {
                    result["notes"][i] = row[0].get<std::string>();
                } else {
                    result["notes"][i] = "";
                }
                ++i;
            }
            i = 0;
            for (const auto &newDeviceid: newDeviceIds) {
                result["new_deviceIds"][i++] = newDeviceid;
            }
            return crow::response(result);
        });

        CROW_ROUTE(app, "/front/api/get_device_info/<string>")
        ([&](const std::string &id) {
            std::shared_lock lock(mtx);
            auto session = client.getSession();
            crow::json::wvalue result;
            auto res = session.sql(
                "SELECT id,name,details "
                "FROM device_info "
                "WHERE id = ?"
            ).bind(id).execute();
            auto row = res.fetchOne();
            result["id"] = row[0].get<std::string>();
            result["name"] = row[1].get<std::string>();
            result["details"] = row[2].get<std::string>();
            return result;
        });

        CROW_ROUTE(app, "/front/api/note_device").methods("POST"_method)
        ([&](const crow::request &req) {
            std::shared_lock lock(mtx);
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
                "   FROM device_info "
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
                    "INSERT INTO device_info(id, name) VALUES (?, ?)"
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
