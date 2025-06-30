<table align="center">
  <tr>
    <td><img src=".github/assets/logo.png" alt="Socks Logo" width="80"></td>
    <td><h1 style="margin: 0;">Socks: Simple JSON-based IPC via Pluggable Transports</h1></td>
  </tr>
</table>

**Socks** is a lightweight C++ library that enables structured interprocess communication (IPC) using **JSON** over customizable transport layers like **Unix Domain Sockets**, **UDP**, or **TCP**.  
Designed for simplicity and efficiency, it powers communication between components like our PyQt-based launcher UI and our robot's C++ core logic — as well as between robots themselves.

The protocol is intentionally simple: clients send a JSON request with a `_cmd` field, and the server responds with a structured JSON object.  
There is no session management, encryption, authentication, or streaming support — **Socks is intended for trusted environments**.

---

## ✨ Features

- 🔌 **Pluggable transport system** (Unix Domain Sockets, UDP, TCP)
- ⚡ **Minimal JSON-based request/response protocol**
- 🛠 **General-purpose thread pool** (usable outside Socks)
- 🛁 **Blocking, asynchronous, and background client requests**
- 🧐 **Type-safe schema validation** for deep JSON structures
- 🧹 **Modular design** (server, client, transports are separated)

---

## 🚦 How It Works

- A **client** sends a JSON object containing a `_cmd` field.
- The **server** looks up the appropriate handler registered for `_cmd`.
- The handler responds with `Socks::okay()` or `Socks::error()`.
- Communication is performed over a chosen **Transport**.

---

## 📦 Project Structure

| Path | Description |
|:-----|:------------|
| `server.hpp/cpp` | JSON server, handler registration, routing |
| `client.hpp/cpp` | JSON client, sending requests (sync, async, background) |
| `transport.hpp/cpp` | Abstract transport system (Unix, UDP, TCP) |
| `threadpool.hpp/cpp` | General-purpose ThreadPool implementation |
| `schema.hpp/cpp` | Type-safe request validation |

---

## 🛡️ Limitations

- **No security**: No encryption, authentication, or TLS
- **No connection pooling**: Each request is independent
- **Single buffer assumption**: Each message must fit into one buffer
- **No streaming/chunking support**
- **Designed for trusted and internal environments**

---

## 📚 Full Usage and Examples

For detailed C++ examples, setup instructions, threading, and Python clients, see:  
👉 [`docs/cpp.md`](docs/cpp.md)
👉 [`docs/python.md`](docs/python.md)
