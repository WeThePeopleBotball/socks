#include "../client.hpp"
#include "../transport.hpp"

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace Socks;

int main() {
  // auto transport = std::make_unique<UnixSocketTransport>("/tmp/fibo.sock");
  auto transport = std::make_unique<UdpTransport>(8080);
  // auto transport = std::make_unique<TcpTransport>(8080);

  Client client(std::move(transport));

  while (true) {
    std::cout << "Enter Fibonacci number to calculate (or -1 to exit): ";
    int n;
    if (!(std::cin >> n)) {
      std::cerr << "Invalid input. Exiting.\n";
      break;
    }
    if (n == -1) {
      std::cout << "Goodbye!\n";
      break;
    }

    // Create request JSON with command
    json request = {
        {"n", n}
        // _cmd is injected by the Client wrapper
    };

    try {
      // Send request to "fibo" endpoint using the client
      json response = client.send_request("fibo", request);
      std::cout << "fib(" << n << ") = " << response.at("result").get<int>()
                << "\n";
    } catch (const std::exception &e) {
      std::cerr << "Request failed: " << e.what() << "\n";
    }
  }

  return 0;
}
