#pragma once

#include <string>

/**
 * @file transport.hpp
 * @brief Abstract interface for pluggable transport mechanisms (Unix, UDP,
 * TCP).
 *
 * This interface allows Socks to support multiple underlying communication
 * protocols by abstracting away the low-level send/receive/bind operations.
 */

namespace Socks {

/**
 * @class Transport
 * @brief Abstract base class for communication transports.
 *
 * Each derived transport must implement the ability to bind for communication,
 * receive a message (returning the content and client identity), respond to a
 * client, send a request as a client, and cleanly shut down.
 */
class Transport {
  public:
    virtual ~Transport() = default;

    /**
     * @brief Prepare the transport to receive messages.
     *
     * For example, bind a socket to an address or path so it can accept
     * incoming data. Must be called before calling receive().
     *
     * @throws std::runtime_error if binding fails.
     */
    virtual void bind() = 0;

    /**
     * @brief Receive a message from a client.
     *
     * This function blocks until a message is received. It also outputs a
     * `client_id` used to identify the sender (e.g., a file descriptor,
     * IP:port, or path).
     *
     * @param[out] client_id A unique identifier for the client, used later in
     * send().
     * @return The raw message as a string.
     * @throws std::runtime_error if receiving fails.
     */
    virtual std::string receive(std::string &client_id) = 0;

    /**
     * @brief Send a response back to a client (server-side).
     *
     * Sends the `data` string to the client identified by `client_id`, which
     * was previously returned by receive().
     *
     * @param data The message to send back (usually a JSON string).
     * @param client_id The identifier of the target client.
     * @throws std::runtime_error if sending fails.
     */
    virtual void send(const std::string &data,
                      const std::string &client_id) = 0;

    /**
     * @brief Send a message to the server and receive the response
     * (client-side).
     *
     * This sends a message from a client to the configured destination
     * (via IP or socket path) and waits for the server's reply.
     *
     * @param data The message to send (usually a JSON string).
     * @return The response from the server.
     * @throws std::runtime_error if sending or receiving fails.
     */
    virtual std::string send(const std::string &data) = 0;

    /**
     * @brief Clean up and close the transport.
     *
     * Should release any socket/file descriptors or other resources.
     */
    virtual void close() = 0;
};

/**
 * @class UnixSocketTransport
 * @brief UNIX domain socket implementation of the Transport interface.
 *
 * Uses a local file path (e.g., /tmp/socks.sock) for interprocess
 * communication.
 */
class UnixSocketTransport : public Transport {
  public:
    /**
     * @brief Construct a new UnixSocketTransport using a UNIX socket path.
     * @param socket_path Filesystem path for the UNIX socket.
     */
    explicit UnixSocketTransport(const std::string &socket_path);

    void bind() override;
    std::string receive(std::string &client_id) override;
    void send(const std::string &data, const std::string &client_id) override;
    std::string send(const std::string &data) override;
    void close() override;

  private:
    std::string socket_path_;
    int server_fd_ = -1;
};

/**
 * @class UdpTransport
 * @brief UDP implementation of the Transport interface.
 *
 * Allows both server-side message reception and client-side message sending.
 */
class UdpTransport : public Transport {
  public:
    /**
     * @brief Construct a server-side UdpTransport that binds to a local port.
     * @param port Port number to bind.
     */
    explicit UdpTransport(int port);

    /**
     * @brief Construct a client-side UdpTransport that targets a remote IP and
     * port.
     * @param ip Destination IPv4 address (e.g., "127.0.0.1").
     * @param port Destination port number.
     */
    UdpTransport(const std::string &ip, int port);

    void bind() override;
    std::string receive(std::string &client_id) override;
    void send(const std::string &data, const std::string &client_id) override;
    std::string send(const std::string &data) override;
    void close() override;

  private:
    int port_;
    std::string ip_ = "127.0.0.1";
    int sock_ = -1;
};

/**
 * @class TcpTransport
 * @brief TCP implementation of the Transport interface.
 *
 * Allows both server-side message reception and client-side message sending.
 */
class TcpTransport : public Transport {
  public:
    /**
     * @brief Construct a server-side TcpTransport that binds to a local port.
     * @param port Port number to bind.
     */
    explicit TcpTransport(int port);

    /**
     * @brief Construct a client-side TcpTransport that connects to a remote IP
     * and port.
     * @param ip Destination IPv4 address (e.g., "127.0.0.1").
     * @param port Destination port number.
     */
    TcpTransport(const std::string &ip, int port);

    void bind() override;
    std::string receive(std::string &client_id) override;
    void send(const std::string &data, const std::string &client_id) override;
    std::string send(const std::string &data) override;
    void close() override;

  private:
    int port_;
    std::string ip_ = "127.0.0.1";
    int server_fd_ = -1;
};

} // namespace Socks
