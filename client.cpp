#include "client.hpp"

#include <thread>

namespace Socks {

Client::Client(std::unique_ptr<Transport> transport)
    : transport_(std::move(transport)) {}

Client::~Client() {
    if (transport_) {
        transport_->close();
    }
}

json Client::send_request(const std::string &endpoint, const json &request) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Construct full request JSON with "_cmd" field
    json full_request = request;
    full_request["_cmd"] = endpoint;

    // Use new Transport::send(data) to send and receive in one go
    std::string response_str = transport_->send(full_request.dump());

    json response = json::parse(response_str);

    if (!response.value("_success", false)) {
        std::string msg = response.value("_msg", "Unknown server error.");
        throw std::runtime_error("Request failed: " + msg);
    }

    return response;
}

std::future<json> Client::send_request_async(const std::string &endpoint,
                                             const json &request) {
    return std::async(std::launch::async, [this, endpoint, request]() {
        return send_request(endpoint, request);
    });
}

void Client::send_request_bg(const std::string &endpoint, const json &request,
                             std::function<void(json)> callback) {
    std::thread([this, endpoint, request, callback]() {
        try {
            json response = send_request(endpoint, request);
            callback(response);
        } catch (const std::exception &e) {
            callback(json{{"_success", false}, {"_msg", e.what()}});
        }
    }).detach();
}

} // namespace Socks
