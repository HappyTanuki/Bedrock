#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "networking/address.h"
#include "networking/socket.h"
#include "networking/wsa.h"

int main() {
  bedrock::network::WSAManager wsamanager;
  bedrock::network::Address addr;
  addr.SetAddr(bedrock::network::IPVersion::kIPV6, "::1", 8008);

  bedrock::network::Socket sock(bedrock::network::SocketType::kTCP, addr);
  if (sock.Init() != bedrock::network::SocketErrorStatus::kSuccess) {
    std::cout << "Error: " << sock.GetErrorMessage() << std::endl;
    std::exit(EXIT_FAILURE);
  }
  if (sock.Connect() != bedrock::network::SocketErrorStatus::kSuccess) {
    std::cout << "Error: " << sock.GetErrorMessage() << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << "Client is now connected to: " << static_cast<std::string>(addr)
            << std::endl;
  while (true) {
    std::string input;

    std::cin >> input;

    if (input == "") {
      break;
    }

    auto bytes = std::as_bytes(std::span(input));

    sock.Write(bytes);
    auto read = sock.Read(BUFSIZ);
    if (read.status != bedrock::network::SocketErrorStatus::kSuccess) {
      std::cout << "Error: " << sock.GetErrorMessage() << std::endl;
      std::exit(EXIT_FAILURE);
    }

    std::cout << "Received: \""
              << std::string(
                     reinterpret_cast<const char*>(read.data.first.data()),
                     read.data.first.size())
              << "\" from " << static_cast<std::string>(addr) << std::endl;
  }

  std::exit(EXIT_SUCCESS);
}
