#include <atomic>
#include <cors.hpp>
#include <real_time_data.hpp>
#include <nlohmann/json.hpp>

int main() {
    std::unordered_map<std::string,RealTimeData> deviceRealTimeDatas;

    crow::App<CORS> app;

    CROW_ROUTE(app, "/api/data").methods("POST"_method)
    ([&](const crow::request &req) {
        const auto body = crow::json::load(req.body);
        deviceRealTimeDatas[body["device_id"].s()].loadForJson(body);

                return crow::response();
            });

            CROW_ROUTE(app, "/api/data")([&] {
                crow::response res(deviceRealTimeDatas["1234"].getJson().dump());
                res.add_header("Content-Type", "application/json");
                return res;
            });


            app.port(18080).multithreaded().run();

            return 0;
}
