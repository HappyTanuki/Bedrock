#ifndef BEDROCK_NETWORKING_NETWORKING_SOCKET_ERROR_HANDLE_H_
#define BEDROCK_NETWORKING_NETWORKING_SOCKET_ERROR_HANDLE_H_

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __linux__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstring>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#else
#error "이 플랫폼은 지원되지 않습니다."
#endif

#include <string>

namespace bedrock {

#ifdef _WIN32
#include "networking/wsa.h"
inline int GetSocketLastErrorCode() { return WSAGetLastError(); }
inline std::string GetSocketErrorMessage(int err) {
  return bedrock::network::WSAErrorMsg(err).str();
}
#else
inline int GetSocketLastErrorCode() { return errno; }
inline std::string GetSocketErrorMessage(int err) { return std::strerror(err); }
#endif

class SocketErrorReportable {
 public:
  SocketErrorReportable() = default;
  SocketErrorReportable(const SocketErrorReportable&) = default;
  SocketErrorReportable& operator=(const SocketErrorReportable&) = default;

  SocketErrorReportable(SocketErrorReportable&&) = default;
  SocketErrorReportable& operator=(SocketErrorReportable&&) = default;

  virtual ~SocketErrorReportable();

  virtual std::string GetErrorMessage() const = 0;
  virtual int GetLastErrno() const = 0;
};

}  // namespace bedrock

#endif
