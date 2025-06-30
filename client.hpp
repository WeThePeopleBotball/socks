#pragma once

#include "nlohmann/json.hpp"
#include "transport.hpp"
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>

/**
 * @file client.hpp
 * @brief Lightweight JSON-based client for sending requests over Socks
 * transports.
 *
 * Supports blocking, asynchronous, and background callback-based request
 * handling.
 */

namespace Socks {

using json = nlohmann::json;

/**
 * @class Client
 * @brief Client for sending JSON requests over a pluggable transport.
 *
 * Features:
 * - Synchronous blocking requests
 * - Asynchronous requests returning futures
 * - Background requests with callback on completion
 * - Automatic error checking (_success field)
 * - Thread-safe
 */
class Client {
  public:
    /**
     * @brief Constructs a Client with the specified transport.
     * @param transport Unique pointer to a transport backend (Unix, UDP, TCP,
     * etc.).
     */
    explicit Client(std::unique_ptr<Transport> transport);

    /**
     * @brief Destroys the client, closing the transport.
     */
    ~Client();

    /**
     * @brief Sends a request and blocks until a response is received.
     * @param endpoint Target endpoint identifier (not always used depending on
     * transport).
     * @param request JSON object containing the request.
     * @return JSON object containing the response.
     * @throws std::runtime_error if an error response is received or
     * communication fails.
     */
    json send_request(const std::string &endpoint, const json &request);

    /**
     * @brief Sends a request asynchronously.
     * @param endpoint Target endpoint identifier.
     * @param request JSON object containing the request.
     * @return std::future<json> which will hold the response or throw an error.
     */
    std::future<json> send_request_async(const std::string &endpoint,
                                         const json &request);

    /**
     * @brief Sends a request in the background and invokes a callback upon
     * completion.
     * @param endpoint Target endpoint identifier.
     * @param request JSON object containing the request.
     * @param callback Function to call with the response.
     */
    void send_request_bg(const std::string &endpoint, const json &request,
                         std::function<void(json)> callback);

  private:
    std::unique_ptr<Transport> transport_; ///< Communication backend
    std::mutex mutex_;                     ///< Ensures thread-safe send/receive
};

} // namespace Socks
