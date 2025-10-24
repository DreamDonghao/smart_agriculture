//
// Created by donghao on 25-10-23.
//
#ifndef CORS_H
#define CORS_H

#include <crow.h>

struct CORS {
    struct context {
    };

    void before_handle(crow::request &req, crow::response &res, context &) {
        const std::string &origin = req.get_header_value("Origin");

        // 仅处理预检请求
        if (req.method == "OPTIONS"_method) {
            if (!origin.empty()) {
                res.set_header("Access-Control-Allow-Origin", origin);
                res.set_header("Vary", "Origin"); // 防止缓存污染
                res.set_header("Access-Control-Allow-Credentials", "true");
            }

            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers",
                           "Origin, X-Requested-With, Content-Type, Accept, Authorization");
            res.set_header("Access-Control-Max-Age", "86400"); // 预检缓存一天
            res.code = 204;                                    // No Content
            res.end();
        }
    }

    void after_handle(crow::request &req, crow::response &res, context &) {
        const std::string &origin = req.get_header_value("Origin");
        if (!origin.empty()) {
            res.set_header("Access-Control-Allow-Origin", origin);
            res.set_header("Vary", "Origin");
            res.set_header("Access-Control-Allow-Credentials", "true");
        }
    }
};

#endif // CORS_H