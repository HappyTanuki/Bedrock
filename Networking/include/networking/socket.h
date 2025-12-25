#ifndef BEDROCK_NETWORKING_NETWORKING_SOCKET_H_
#define BEDROCK_NETWORKING_NETWORKING_SOCKET_H_

#include <sys/socket.h>
#include <unistd.h>

#include "address.h"
#include "common/interfaces.h"
#include "socket_error_handle.h"

namespace bedrock::network {

enum class SocketType : std::uint16_t {
  kInvalid,
  kTCP = SOCK_STREAM,
  kUDP = SOCK_DGRAM,
  kRAW = SOCK_RAW
};

enum class SocketErrorStatus { kSuccess, kFailure, kInternal, kAddress };

class Socket : public bedrock::Validatable,
               public bedrock::SocketErrorReportable,
               public bedrock::ReadWritable<SocketErrorStatus> {
 public:
  Socket(const Socket& other) = delete;
  Socket& operator=(const Socket& other) = delete;

  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  Socket() = default;
  Socket(SocketType socket_type, const Address& address) {
    SetAddr(socket_type, address);
  }
  ~Socket() override;

  SocketErrorStatus Init();
  SocketErrorStatus Init(int fd);

  SocketErrorStatus Bind();

  SocketErrorStatus Listen();
  SocketErrorStatus Connect();

  DataWithStatus<Socket, SocketErrorStatus> Accept();

  SocketErrorStatus SetAddr(SocketType socket_type, const Address& address);
  DataWithStatus<Address, SocketErrorStatus> GetAddr() const;

  DataWithStatus<SocketType, SocketErrorStatus> GetType() const;

  // Interface implements
  bool IsValid() const final override { return valid; }

  std::string GetErrorMessage() const final override {
    return last_error_message;
  }
  int GetLastErrno() const final override { return last_errno; }

  DataWithStatus<std::pair<std::vector<std::byte>, std::uint32_t>,
                 SocketErrorStatus>
  Read(std::uint32_t request_size) final override;
  SocketErrorStatus Write(std::span<const std::byte> data) final override;

 private:
  bool valid = false;

  std::string last_error_message;
  int last_errno = 0;

  SocketType type = SocketType::kInvalid;
  Address addr;

  int socket_fd = -1;
};

}  // namespace bedrock::network

#endif
