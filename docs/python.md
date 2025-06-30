# Socks Python Usage Guide

This document covers how to use Socks transports with simple and consistent Python clients.

---

## 📬 Unified Helper Library

All clients share a consistent API and are organized to be used like a small library.

---

## 🔧 Demo Code

### Core Helpers (Exception-Safe)

```python
import socket
import json
import asyncio

class SocksClientError(Exception):
    pass

def _prepare_payload(cmd, payload):
    if not isinstance(cmd, str):
        raise SocksClientError("Command must be a string.")
    if not isinstance(payload, dict):
        raise SocksClientError("Payload must be a dictionary.")
    return {"_cmd": cmd, **payload}

def _process_response(raw_response):
    try:
        response = json.loads(raw_response)
    except json.JSONDecodeError:
        raise SocksClientError("Received invalid JSON.")

    if not response.get("_success", False):
        raise SocksClientError(response.get("_msg", "Unknown error"))

    return response
```

### Unix Domain Socket Client

```python
def send_unix(cmd, payload, socket_path="/tmp/socks.sock"):
    payload_with_cmd = _prepare_payload(cmd, payload)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(socket_path)
        s.sendall(json.dumps(payload_with_cmd).encode())
        response = s.recv(2048)
        return _process_response(response)
```

### UDP Client

```python
def send_udp(cmd, payload, host="127.0.0.1", port=8080):
    payload_with_cmd = _prepare_payload(cmd, payload)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(2.0)
        s.sendto(json.dumps(payload_with_cmd).encode(), (host, port))
        response, _ = s.recvfrom(2048)
        return _process_response(response)
```

### TCP Client

```python
def send_tcp(cmd, payload, host="127.0.0.1", port=8080):
    payload_with_cmd = _prepare_payload(cmd, payload)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(json.dumps(payload_with_cmd).encode())
        response = s.recv(2048)
        return _process_response(response)
```

### Async TCP Client

```python
async def send_tcp_async(cmd, payload, host="127.0.0.1", port=8080):
    payload_with_cmd = _prepare_payload(cmd, payload)
    reader, writer = await asyncio.open_connection(host, port)

    writer.write(json.dumps(payload_with_cmd).encode())
    await writer.drain()

    data = await reader.read(2048)
    writer.close()
    await writer.wait_closed()

    return _process_response(data)
```

---

## 🚀 Usage Examples

### 1. Blocking Example

```python
try:
    response = send_unix("ping", {})
    print("Response:", response)
except SocksClientError as e:
    print("Error:", e)
```

### 2. Asyncio Example (TCP)

```python
async def main():
    try:
        response = await send_tcp_async("ping", {})
        print("Async Response:", response)
    except SocksClientError as e:
        print("Async Error:", e)

asyncio.run(main())
```

### 3. Threaded Example

```python
import threading

def threaded_request():
    try:
        print("Threaded Response:", send_tcp("ping", {}))
    except SocksClientError as e:
        print("Threaded Error:", e)

thread = threading.Thread(target=threaded_request)
thread.start()

print("Main thread continues...")
thread.join()
```

---

## 📄 Field Explanation

| Field | Description |
|:------|:------------|
| `_cmd` | **Required**. Command name to execute on the server. |
| other fields | Optional. Any additional parameters for the command. |

- Always include `_cmd` in your requests.
- Server expects properly structured JSON.

---

## ❗ Error Handling

- Any missing `_success` field or `_success: false` will raise a `SocksClientError`.
- If server returns a meaningful `_msg`, it will be included in the exception message.
- If response is not valid JSON, a parsing error will also raise `SocksClientError`.

Example:
```python
try:
    response = send_unix("nonexistent-command", {})
except SocksClientError as e:
    print("Caught Socks error:", e)
```
