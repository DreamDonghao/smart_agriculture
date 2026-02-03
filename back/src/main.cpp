#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <mysqlx/xdevapi.h>
#include <shared_mutex>
#include <unordered_map>
#include <set>
#include <iostream>
#include <real_time_data.hpp>
static std::shared_mutex mtx;
using json = nlohmann::json;

/* ======== CORS 工具函数 ======== */

void set_cors_headers(const httplib::Request &req, httplib::Response &res) {
    static const std::unordered_set<std::string> allowed_origins = {
        "http://localhost:5173",
        "http://127.0.0.1:5173",
        "https://example.com"
    };

    const auto origin = req.get_header_value("Origin");
    if (!origin.empty() && allowed_origins.contains(origin)) {
        res.set_header("Access-Control-Allow-Origin", origin);
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    }
}


std::string call_deepseek(const std::string &userInput) {
    httplib::SSLClient cli("api.deepseek.com", 443);  // 注意端口 HTTPS 默认是 443
    cli.enable_server_certificate_verification(true); // 验证证书，可选

    cli.set_default_headers({
        {"Authorization", "Bearer sk-b9f3ee3a3ddd45ebb31d4a918c828217"},
        {"Content-Type", "application/json"}
    });

    json body;
    body["model"] = "deepseek-chat";
    body["stream"] = false;
    body["messages"] = {
        {{"role", "system"}, {"content", "You are a helpful assistant"}},
        {{"role", "user"}, {"content", userInput}}
    };

    auto res = cli.Post("/v1/chat/completions", body.dump(), "application/json");
    if (!res || res->status != 200) {
        return "DeepSeek API error";
    }

    json resp = json::parse(res->body);
    return resp["choices"][0]["message"]["content"];
}

int main() {
    try {
        mysqlx::Client client(
            "mysqlx://argi:your_password@47.93.221.71:33060/smart_agriculture",
            mysqlx::ClientOption::POOL_MAX_SIZE, 10
        );

        httplib::Server server;

        /* ======== 设备状态 ======== */
        std::set<std::string> newDeviceIds;
        std::set<std::string> addedDeviceIds;
        std::unordered_map<std::string, RealTimeData> deviceRealTimeDatas; {
            auto session = client.getSession();
            auto res = session.sql("SELECT id FROM device_info;").execute();
            for (const auto &row: res.fetchAll()) {
                auto id = row[0].get<std::string>();
                addedDeviceIds.insert(id);
                deviceRealTimeDatas[id];
            }
        }

        /* ======== OPTIONS 预检 ======== */
        server.Options(R"(.*)", [](const httplib::Request &req, httplib::Response &res) {
            std::cout << res.body << std::endl;
            set_cors_headers(req,res);
            res.status = 204;
        });

        /* ======== /api/data ======== */
        server.Post("/api/data", [&](const httplib::Request &req, httplib::Response &res) {
            std::unique_lock lock(mtx);

            auto body = json::parse(req.body);
            std::string device_id = body["device_id"];

            if (addedDeviceIds.contains(device_id)) {
                deviceRealTimeDatas[device_id].loadForJson(body);
            } else {
                newDeviceIds.insert(device_id);
            }

            set_cors_headers(req, res);
            res.status = 200;
        });

        /* ======== /front/api/data ======== */
        server.Post("/front/api/data", [&](const httplib::Request &req, httplib::Response &res) {
            std::shared_lock lock(mtx);

            auto body = json::parse(req.body);
            auto &data = deviceRealTimeDatas[body["device_id"]];

            set_cors_headers(req, res);
            res.set_content(data.getJson().dump(), "application/json");
        });

        /* ======== /front/api/get_all_device_ids ======== */
        server.Get("/front/api/get_all_device_ids",
            [&](const httplib::Request &req, httplib::Response &res) {
            std::shared_lock lock(mtx);

            auto session = client.getSession();
            json result;
            int i = 0;

            for (const auto &id: addedDeviceIds) {
                result["devices"][i] = id;

                auto r = session.sql(
                    "SELECT name FROM device_info WHERE id = ?"
                ).bind(id).execute();

                if (auto row = r.fetchOne()) {
                    result["notes"][i] = row[0].get<std::string>();
                } else {
                    result["notes"][i] = "";
                }
                ++i;
            }

            i = 0;
            for (const auto &id: newDeviceIds) {
                result["new_deviceIds"][i++] = id;
            }

            set_cors_headers(req,res);
            res.set_content(result.dump(), "application/json");
        });

        /* ======== /front/api/get_device_info/{id} ======== */
        server.Get(R"(/front/api/get_device_info/(\w+))",
                   [&](const httplib::Request &req, httplib::Response &res) {
                       std::shared_lock lock(mtx);

                       auto session = client.getSession();
                       int id_int = std::stoi(req.matches[1].str()); // 转成 int

                       auto r = session.sql(
                           "SELECT id, name, details FROM device_info WHERE id = ?"
                       ).bind(id_int).execute();


                       auto row = r.fetchOne();
                       const json result{
                           {"id", row[0].get<std::string>()},
                           {"name", row[1].get<std::string>()},
                           {"details", row[2].get<std::string>()}
                       };

                       set_cors_headers(req,res);
                       res.set_content(result.dump(), "application/json");
                   });

        server.Post(
            "/front/api/note_device",
            [&](const httplib::Request &req, httplib::Response &res) {
                std::unique_lock lock(mtx);

                auto session = client.getSession();
                auto body = json::parse(req.body);

                std::string device_id = body["device_id"];
                std::string note = body["device_note"];

                auto r = session.sql(
                    "SELECT EXISTS("
                    "SELECT 1 FROM device_info WHERE id = ?"
                    ")"
                ).bind(device_id).execute();

                json rsp;

                if (r.fetchOne()[0].get<bool>()) {
                    rsp["status"] = "exist";
                    rsp["message"] = "设备已存在";
                } else {
                    session.sql(
                        "INSERT INTO device_info(id, name) VALUES (?, ?)"
                    ).bind(device_id, note).execute();

                    rsp["status"] = "inserted";
                    rsp["message"] = "已新增设备并记录备注";
                }

                set_cors_headers(req,res);
                res.set_content(rsp.dump(), "application/json");
            });

        std::cout << call_deepseek("Hello") << std::endl;



        // POST 请求
        server.Post("/api/chat", [&](const httplib::Request &req, httplib::Response &res) {
            set_cors_headers(req,res);
            std::unique_lock lock(mtx);
            try {
                const auto body = json::parse(req.body);
                const std::string message = body.value("message", ""); // 避免空字段

                json r;
                std::cout << message << std::endl;
                r["reply"] = call_deepseek(message);

                res.set_content(r.dump(), "application/json");
            } catch (const std::exception &e) {
                res.status = 500;
                json err;
                err["error"] = e.what();
                res.set_content(err.dump(), "application/json");
            }
        });
        std::cout << "Server running at http://localhost:18080\n";
        server.listen("0.0.0.0", 18080);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }


    return 0;
}
