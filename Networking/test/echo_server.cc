#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "networking/address.h"
#include "networking/socket.h"
#include "networking/wsa.h"

int main() {
  bedrock::network::WSAManager wsamanager;
  bedrock::network::Address addr;
  addr.SetAddr(bedrock::network::IPVersion::kIPV6, "::", 8008);

  bedrock::network::Socket sock(bedrock::network::SocketType::kTCP, addr);
  if (sock.Init() != bedrock::network::SocketErrorStatus::kSuccess) {
    std::cout << "Error: " << sock.GetErrorMessage() << std::endl;
    std::exit(EXIT_FAILURE);
  }
  if (sock.Bind() != bedrock::network::SocketErrorStatus::kSuccess) {
    std::cout << "Error: " << sock.GetErrorMessage() << std::endl;
    std::exit(EXIT_FAILURE);
  }
  if (sock.Listen() != bedrock::network::SocketErrorStatus::kSuccess) {
    std::cout << "Error: " << sock.GetErrorMessage() << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << "Server is now listening on: " << static_cast<std::string>(addr)
            << std::endl;
  while (true) {
    auto ret_newsock = sock.Accept();
    if (ret_newsock.status != bedrock::network::SocketErrorStatus::kSuccess) {
      break;
    }
    std::cout << "Connected to: "
              << static_cast<std::string>(ret_newsock.data.GetAddr().data)
              << std::endl;

    std::thread th(
        [](bedrock::network::Socket new_sock) {
          while (true) {
            auto read = new_sock.Read(BUFSIZ);
            if (read.status != bedrock::network::SocketErrorStatus::kSuccess) {
              std::cout << "Error: " << new_sock.GetErrorMessage() << std::endl;
              std::exit(EXIT_FAILURE);
            }

            if (read.data.first.size() != read.data.second) {
              read.data.first.resize(read.data.second);
            }

            if (read.data.first.size() == 0) {
              break;
            }

            new_sock.Write(read.data.first);

            std::cout << "Echoed \""
                      << std::string(reinterpret_cast<const char*>(
                                         read.data.first.data()),
                                     read.data.first.size())
                      << "\"" << " to :"
                      << static_cast<std::string>(new_sock.GetAddr().data)
                      << std::endl;
          }

          std::exit(EXIT_SUCCESS);
        },
        std::move(ret_newsock.data));
    th.detach();
  }

  std::exit(EXIT_SUCCESS);
}
