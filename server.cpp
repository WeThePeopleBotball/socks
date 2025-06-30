#include "server.hpp"
#include <iostream>

void log_success(const std::string &message) {
    std::cout << "\033[1;32mSUCCESS\033[0m " << message << std::endl; // green
}

void log_info(const std::string &message) {
    std::cout << "\033[1;34mINFO\033[0m " << message << std::endl; // blue
}

void log_warning(const std::string &message) {
    std::cout << "\033[1;33mWARNING\033[0m " << message << std::endl; // yellow
}

void log_error(const std::string &message) {
    std::cerr << "\033[1;31mERROR\033[0m " << message << std::endl; // red
}

namespace Socks {

json okay(const json &result) {
    json res = result;
    res["_success"] = true;
    return res;
}

json error(const json &result, const std::string &message) {
    json res = result;
    res["_success"] = false;
    res["_msg"] = message;
    return res;
}

Server::Server(std::unique_ptr<Transport> transport,
               std::shared_ptr<ThreadPool> thread_pool)
    : transport_(std::move(transport)), thread_pool_(thread_pool) {}

Server::~Server() { stop(); }

void Server::add_handler(const std::string &command, Handler handler) {
    handlers_[command] = std::move(handler);
}

void Server::start() {
    running_ = true;
    transport_->bind();
    log_info("[Socks] Server started. Waiting for connections...");
    serve();
}

void Server::stop() {
    running_ = false;
    transport_->close();
    log_info("[Socks] Server stopped.");
}

void Server::serve() {
    while (running_) {
        try {
            std::string client_id;
            std::string data = transport_->receive(client_id);

            auto task = [this, data, client_id]() {
                json response;
                std::string command = "<unknown>";

                try {
                    json request = json::parse(data);
                    command = request.value("_cmd", "<no _cmd>");
                    log_info("[Socks] Received request for command: " +
                             command);

                    if (auto it = handlers_.find(command);
                        it != handlers_.end()) {
                        response = it->second(request);
                    } else {
                        response = error({}, "Unknown command: " + command);
                    }

                    if (response.value("_success", false)) {
                        log_success("[Socks] Command '" + command +
                                    "' handled successfully.");
                    } else {
                        log_error("[Socks] Command '" + command + "' failed: " +
                                  response.value("_msg", "No error message"));
                    }

                } catch (const std::exception &e) {
                    log_error("[Socks] JSON parse or internal error: " +
                              std::string(e.what()));
                    response = error({}, "Invalid JSON or internal error: " +
                                             std::string(e.what()));
                }

                try {
                    transport_->send(response.dump(), client_id);
                } catch (const std::exception &e) {
                    log_error("[Socks] Send error: " + std::string(e.what()));
                }
            };

            if (thread_pool_) {
                thread_pool_->enqueue(std::move(task));
            } else {
                task();
            }

        } catch (const std::exception &e) {
            log_error("[Socks] Receive error: " + std::string(e.what()));
        }
    }
}

} // namespace Socks
