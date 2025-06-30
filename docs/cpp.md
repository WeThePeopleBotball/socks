# 🧦 Socks C++ Usage Guide

This document explains how to use **Socks**, a pluggable, JSON-based IPC framework for C++ projects. It covers server setup, transport switching, schema validation, multi-threading, and client usage (blocking, async, and background).

---

## 🧩 Overview

Socks abstracts communication over:

- **UNIX domain sockets** (local IPC)
- **UDP** (low-latency, connectionless)
- **TCP** (reliable, stream-based)

You can easily swap transports without changing your logic.

---

## ⚙️ Transport Setup

### 🔁 Transport Constructors

Each transport can act as a **server** or a **client**, depending on how it's constructed:

| Transport Type           | Server Constructor                     | Client Constructor                        |
|--------------------------|-----------------------------------------|-------------------------------------------|
| `UnixSocketTransport`    | `UnixSocketTransport("/tmp/f.sock")`    | Same as server – connect to path          |
| `UdpTransport`           | `UdpTransport(8080)`                    | `UdpTransport("127.0.0.1", 8080)`          |
| `TcpTransport`           | `TcpTransport(8080)`                    | `TcpTransport("127.0.0.1", 8080)`          |

---

## 🌐 Basic Server Example

```cpp
#include "server.hpp"
#include "transport.hpp"

using namespace Socks;

int main() {
    auto transport = std::make_unique<UnixSocketTransport>("/tmp/socks.sock");
    Server server(std::move(transport));

    server.add_handler("ping", [](const json&) {
        return okay({{"pong", true}});
    });

    server.start(); // Blocks
}
```

### 🔄 Handler Signature

```cpp
using Handler = std::function<json(const json&)>;
```

Return structured responses using:

```cpp
return Socks::okay({...});
return Socks::error("message");
```

---

## 🧵 Async Server with ThreadPool

```cpp
#include "server.hpp"
#include "threadpool.hpp"
using namespace Socks;

int main() {
    auto transport = std::make_unique<TcpTransport>(8080);
    auto pool = std::make_shared<ThreadPool>(4); // 4 threads
    Server server(std::move(transport), pool);

    server.add_handler("ping", [](const json&) {
        return okay({{"pong", true}});
    });

    server.start();
}
```

- Without `ThreadPool`, server handles requests sequentially.
- With `ThreadPool`, handlers are run concurrently.

---

## 🔁 Switching Transport Types

You can switch protocols with no code change except constructor:

```cpp
auto transport = std::make_unique<UnixSocketTransport>("/tmp/sock");
auto transport = std::make_unique<UdpTransport>(8080);
auto transport = std::make_unique<TcpTransport>("127.0.0.1", 8080);
```

The rest of your app remains unchanged.

---

## 📐 Schema Validation

```cpp
Socks::assert_parameters(req, {
  {"value", json::value_t::boolean},
  {"meta", {
    {"retries", json::value_t::number_integer}
  }}
});
```

- Ensures request contains expected structure.
- Throws `std::runtime_error` if any mismatch occurs.

---

## 📬 Client Usage

### Blocking Example

```cpp
#include "client.hpp"
#include "transport.hpp"
using namespace Socks;

int main() {
    auto transport = std::make_unique<UnixSocketTransport>("/tmp/socks.sock");
    Client client(std::move(transport));

    json request = {{"hello", "world"}};

    try {
        json response = client.send_request("ping", request);
        std::cout << "Response: " << response.dump(2) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
```

### Async Example

```cpp
auto future = client.send_request_async("ping", request);
auto response = future.get();
```

### Background (Callback) Example

```cpp
client.send_request_bg("ping", request, [](json response) {
    if (response.value("_success", false)) {
        std::cout << "BG Response: " << response.dump(2) << "\n";
    } else {
        std::cerr << "BG Error: " << response.value("_msg", "Unknown error") << "\n";
    }
});
```

---

## ✅ Client Behavior

- Automatically adds `"_cmd"` field from `send_request(cmd, req)`
- Uses `Transport::send(data)` under the hood
- Thread-safe via internal mutex
- Throws if `_success == false` or missing

---

## 🛠 ThreadPool API

```cpp
ThreadPool pool(4);

pool.enqueue([] {
    std::cout << "Task\n";
});

auto result = pool.submit_async([] { return 42; });
std::cout << result.get() << "\n";
```

| Method          | Description                          |
|-----------------|--------------------------------------|
| `enqueue(fn)`   | Fire-and-forget execution            |
| `submit_async`  | Future-returning task execution      |
| `wait()`        | Graceful shutdown after all tasks    |
| `terminate()`   | Immediate stop                       |

