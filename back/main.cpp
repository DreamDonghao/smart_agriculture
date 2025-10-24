#include <atomic>
#include <cors.hpp>
#include <real_time_data.hpp>
#include <nlohmann/json.hpp>

int main() {
    RealTimeData realTimeData;

    crow::App<CORS> app;

    CROW_ROUTE(app, "/api/data").methods("POST"_method)
    ([&](const crow::request &req) {
        const auto body = crow::json::load(req.body);
        realTimeData.loadForJson(body);

        return crow::response();
    });

    CROW_ROUTE(app, "/api/data")([&realTimeData] {
        crow::response res(realTimeData.getJson().dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });


    app.port(18080).multithreaded().run();

    return 0;
}
