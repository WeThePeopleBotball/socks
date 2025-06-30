#pragma once

#include "schema.hpp"
#include "threadpool.hpp"
#include "transport.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

/**
 * @file socks.hpp
 * @brief Lightweight JSON-based IPC server with pluggable transport.
 *
 * This library enables interprocess communication via JSON messages, routed
 * by command keys over a customizable transport layer (Unix, UDP, TCP).
 *
 * A JSON request looks like:
 * @code
 * {
 *   "_cmd": "my_command",
 *   "param": 42
 * }
 * @endcode
 *
 * And a typical successful response:
 * @code
 * {
 *   "_success": true,
 *   "result": 123
 * }
 * @endcode
 */

namespace Socks {

using json = nlohmann::json;

/**
 * @brief Type alias for a handler that receives and returns a JSON object.
 */
using Handler = ::std::function<json(const json &)>;

/**
 * @brief Wrap a successful JSON result.
 * @param result Payload to include.
 * @return JSON with `_success: true`.
 */
json okay(const json &result);

/**
 * @brief Wrap a failed JSON result.
 * @param result Partial payload or context.
 * @param message Descriptive error message.
 * @return JSON with `_success: false` and `_msg`.
 */
json error(const json &result, const std::string &message);

/**
 * @brief Socks Server class that routes JSON requests over a transport.
 */
class Server {
  public:
    /**
     * @brief Construct a Socks server.
     * @param transport Unique pointer to a transport backend (Unix, TCP, etc.).
     * @param thread_pool Optional shared pointer to a ThreadPool for concurrent
     * request handling.
     */
    explicit Server(std::unique_ptr<Transport> transport,
                    std::shared_ptr<ThreadPool> thread_pool = nullptr);

    /**
     * @brief Destructor: shuts down the server.
     */
    ~Server();

    /**
     * @brief Register a handler for a given `_cmd` name.
     * @param command The `_cmd` string key.
     * @param handler A callback returning a JSON response.
     */
    void add_handler(const std::string &command, Handler handler);

    /**
     * @brief Start the server. Will be blocking unless used with a threadpool.
     */
    void start();

    /**
     * @brief Stop the server and cleanup.
     */
    void stop();

  private:
    void serve(); ///< Main loop for receiving requests

    std::unique_ptr<Transport> transport_; ///< Communication backend
    std::unordered_map<std::string, Handler>
        handlers_; ///< Registered command handlers
    std::shared_ptr<ThreadPool>
        thread_pool_; ///< Optional thread pool for concurrent request handling
    std::atomic<bool> running_ = false; ///< Run state flag
};

} // namespace Socks
