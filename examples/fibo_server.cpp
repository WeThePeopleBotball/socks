#include "../server.hpp"
#include "../threadpool.hpp"
#include "../transport.hpp"

#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>

using namespace Socks;

std::unordered_map<int, int> memo;
std::mutex memo_mutex;

int compute_fibo(int n) {
  std::cout << "CALC " << n << std::endl;
  if (n <= 1)
    return n;
  if (memo.count(n))
    return memo[n];
  int value = compute_fibo(n - 1) + compute_fibo(n - 2);
  {
    std::lock_guard<std::mutex> lock(memo_mutex);
    memo[n] = value;
  }
  return value;
}

int main() {
  // Choose your transport:
  // auto transport = std::make_unique<UnixSocketTransport>("/tmp/fibo.sock");
  auto transport = std::make_unique<UdpTransport>(8080);
  // auto transport = std::make_unique<TcpTransport>(8080);

  auto pool = std::make_shared<ThreadPool>(4);
  Server server(std::move(transport), pool);

  server.add_handler("fibo", [](const json &req) {
    try {
      assert_parameters(req, {{"n", types({json::value_t::number_integer,
                                           json::value_t::number_unsigned})}});

      int n = static_cast<int>(req.at("n").get<int>());
      int result = compute_fibo(n);
      return okay({{"result", result}});
    } catch (const std::exception &e) {
      return error({}, e.what());
    }
  });

  server.start(); // Blocking
  return 0;
}
