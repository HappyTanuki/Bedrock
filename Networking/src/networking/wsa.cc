#include "networking/wsa.h"

namespace bedrock::network {

WSAManager::WSAManager() {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    std::exit(EXIT_FAILURE);
  }
#endif
}

WSAManager::~WSAManager() {
#ifdef _WIN32
  WSACleanup();
#endif
}

}  // namespace bedrock::network
