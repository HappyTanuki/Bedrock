#include "networking/socket.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include <vector>

#include "networking/address.h"
#include "networking/socket_error_handle.h"

namespace bedrock::network {

Socket::Socket(Socket&& other) noexcept {
  SetAddr(other.GetType().data, other.GetAddr().data);
  Init(other.socket_fd);
  other.socket_fd = INVALID_SOCKET;
  other.valid = false;
  return;
}

Socket& Socket::operator=(Socket&& other) noexcept {
  SetAddr(other.GetType().data, other.GetAddr().data);
  Init(other.socket_fd);
  other.socket_fd = INVALID_SOCKET;
  other.valid = false;
  return *this;
}

Socket::~Socket() {
  if (socket_fd != INVALID_SOCKET) {
#ifndef _WIN32
    ::close(socket_fd);
#else
    ::closesocket(socket_fd);
#endif
  }
}

SocketErrorStatus Socket::Init() {
  auto address_ip_returned = addr.GetIPVersion();
  if (address_ip_returned.status != AddressErrorStatus::kSuccess) {
    return SocketErrorStatus::kAddress;
  }

  socket_fd = ::socket(static_cast<int>(address_ip_returned.data),
                       static_cast<int>(type), 0);
  if (socket_fd == INVALID_SOCKET) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return SocketErrorStatus::kFailure;
  }

  valid = true;

  return SocketErrorStatus::kSuccess;
}

SocketErrorStatus Socket::Init(int fd) {
  if (!addr.IsValid()) {
    return SocketErrorStatus::kAddress;
  }

  socket_fd = fd;
  if (socket_fd == INVALID_SOCKET) {
    // This error codes and messages could be wrong.
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return SocketErrorStatus::kFailure;
  }

  valid = true;

  return SocketErrorStatus::kSuccess;
}

SocketErrorStatus Socket::Bind() {
  if (!IsValid()) {
    return SocketErrorStatus::kInternal;
  }

  auto retval =
      ::bind(socket_fd, static_cast<const ::sockaddr*>(addr), Address::Size());
  if (retval == SOCKET_ERROR) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return SocketErrorStatus::kFailure;
  }

  return SocketErrorStatus::kSuccess;
}

SocketErrorStatus Socket::Listen() {
  if (!IsValid()) {
    return SocketErrorStatus::kInternal;
  }

  auto retval = ::listen(socket_fd, SOMAXCONN);
  if (retval == SOCKET_ERROR) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return SocketErrorStatus::kFailure;
  }

  return SocketErrorStatus::kSuccess;
}

SocketErrorStatus Socket::Connect() {
  if (!IsValid()) {
    return SocketErrorStatus::kInternal;
  }
  if (type != SocketType::kTCP) {
    return SocketErrorStatus::kFailure;
  }

  auto retval = ::connect(socket_fd, static_cast<const ::sockaddr*>(addr),
                          Address::Size());
  if (retval == SOCKET_ERROR) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return SocketErrorStatus::kFailure;
  }

  return SocketErrorStatus::kSuccess;
}

DataWithStatus<Socket, SocketErrorStatus> Socket::Accept() {
  Address new_addr;
  Socket new_socket;

  if (!IsValid()) {
    return {{}, SocketErrorStatus::kInternal};
  }
  if (type != SocketType::kTCP) {
    return {{}, SocketErrorStatus::kFailure};
  }

  ::sockaddr_storage new_raw_addr = {};
  ::socklen_t ignored = sizeof(new_raw_addr);

  auto retval = ::accept(
      socket_fd, reinterpret_cast<::sockaddr*>(&new_raw_addr), &ignored);
  if (retval == INVALID_SOCKET) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return {{}, SocketErrorStatus::kFailure};
  }
  new_addr.SetAddr(new_raw_addr);
  new_socket.SetAddr(SocketType::kTCP, new_addr);
  new_socket.Init(retval);

  return {std::move(new_socket), SocketErrorStatus::kSuccess};
}

SocketErrorStatus Socket::SetAddr(SocketType socket_type,
                                  const Address& address) {
  if (!address.IsValid()) {
    return SocketErrorStatus::kAddress;
  }

  type = socket_type;
  this->addr = address;

  return SocketErrorStatus::kSuccess;
}

DataWithStatus<Address, SocketErrorStatus> Socket::GetAddr() const {
  if (!IsValid()) {
    return {{}, SocketErrorStatus::kInternal};
  }
  return {addr, SocketErrorStatus::kSuccess};
}

DataWithStatus<SocketType, SocketErrorStatus> Socket::GetType() const {
  if (!IsValid()) {
    return {type, SocketErrorStatus::kInternal};
  }
  return {type, SocketErrorStatus::kSuccess};
}

DataWithStatus<std::pair<std::vector<std::byte>, std::uint32_t>,
               SocketErrorStatus>
Socket::Read(std::uint32_t request_size) {
  std::vector<std::byte> buffer(request_size);

  if (!IsValid()) {
    return {{{}, 0}, SocketErrorStatus::kInternal};
  }

  auto retval = SOCKET_ERROR;

  ::sockaddr_storage apponant_raw_addr = {};
  ::socklen_t apponant_raw_addr_size = sizeof(apponant_raw_addr);

  switch (type) {
    case SocketType::kTCP:
      retval = ::recv(socket_fd, buffer.data(), request_size, 0);
      break;
    case SocketType::kUDP:
      retval = ::recvfrom(socket_fd, buffer.data(), request_size, 0,
                          reinterpret_cast<::sockaddr*>(&apponant_raw_addr),
                          &apponant_raw_addr_size);
      break;
    default:
      return {{{}, 0}, SocketErrorStatus::kAddress};
  }

  if (retval == SOCKET_ERROR) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return {{{}, 0}, SocketErrorStatus::kFailure};
  }

  if (type == SocketType::kUDP) {
    addr.SetAddr(apponant_raw_addr);
  }

  return {{buffer, retval}, SocketErrorStatus::kSuccess};
}

SocketErrorStatus Socket::Write(std::span<const std::byte> data) {
  if (!IsValid()) {
    return SocketErrorStatus::kInternal;
  }

  auto retval = SOCKET_ERROR;

  switch (type) {
    case SocketType::kTCP:
      retval = ::send(socket_fd, data.data(), data.size(), 0);
      break;
    case SocketType::kUDP:
      retval = ::sendto(socket_fd, data.data(), data.size(), 0,
                        static_cast<const ::sockaddr*>(addr), Address::Size());
      break;
    default:
      return SocketErrorStatus::kAddress;
  }
  if (retval == SOCKET_ERROR) {
    last_errno = GetSocketLastErrorCode();
    last_error_message = GetSocketErrorMessage(last_errno);
    return SocketErrorStatus::kFailure;
  }

  return SocketErrorStatus::kSuccess;
}

}  // namespace bedrock::network