#ifndef BEDROCK_NETWORKING_NETWORKING_ADDRESS_H_
#define BEDROCK_NETWORKING_NETWORKING_ADDRESS_H_

#ifdef _WIN32
#define NOMINMAX
// clang-format off
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#define in_addr_t ULONG
#elif __linux__
#include <netinet/in.h>
#else
#error "이 플랫폼은 지원되지 않습니다."
#endif

#include <cstdint>
#include <string>
#include <string_view>

#include "common/interfaces.h"
#include "common/types_enums.h"
#include "socket_error_handle.h"

namespace bedrock::network {

enum class IPVersion : std::uint16_t {
  kInvalid,
  kIPV4 = AF_INET,
  kIPV6 = AF_INET6
};

enum class AddressErrorStatus {
  kSuccess,
  kFailure,
  kInternal,
  kIPVersion,
  kAddrinfo
};

class Address : public Validatable,
                public SocketErrorReportable {
 public:
  Address(const Address&) = default;
  Address& operator=(const Address&) = default;

  Address(Address&&) = default;
  Address& operator=(Address&&) = default;

  Address() {}
  explicit Address(std::string_view domain_address, std::string_view port) {
    SetAddr(domain_address, port);
  }
  Address(IPVersion version, std::string_view presentation_address,
          std::uint16_t port) {
    SetAddr(version, presentation_address, port);
  }
  explicit Address(const ::sockaddr_storage& generic_addr) {
    SetAddr(generic_addr);
  }
  explicit Address(const ::sockaddr_in& ipv4addr) { SetAddr(ipv4addr); }
  explicit Address(const ::sockaddr_in6& ipv6addr) { SetAddr(ipv6addr); }
  virtual ~Address() override;

  Address& operator=(const ::sockaddr_in& ipv4addr);
  Address& operator=(const ::sockaddr_in6& ipv6addr);

  AddressErrorStatus SetAddr(std::string_view domain_address,
                             std::string_view port);
  AddressErrorStatus SetAddr(const ::sockaddr_storage& generic_addr);
  AddressErrorStatus SetAddr(const ::sockaddr_in& ipv4addr);
  AddressErrorStatus SetAddr(const ::sockaddr_in6& ipv6addr);
  AddressErrorStatus SetAddr(IPVersion version,
                             std::string_view presentation_address,
                             std::uint16_t port);

  explicit operator DataWithStatus<std::string, AddressErrorStatus>();
  explicit operator std::string();

  explicit operator const ::sockaddr*() const;
  explicit operator ::sockaddr_in() const;
  explicit operator ::sockaddr_in6() const;

  DataWithStatus<IPVersion, AddressErrorStatus> GetIPVersion() const;
  DataWithStatus<std::uint16_t, AddressErrorStatus> GetPort() const;
  static std::uint32_t Size() { return sizeof(addr); }

  // Interface implements
  bool IsValid() const final override { return valid; }

  std::string GetErrorMessage() const final override {
    return last_error_message;
  }
  int GetLastErrno() const final override { return last_errno; }

 private:
  bool valid = false;

  std::string last_error_message;
  int last_errno = 0;

  IPVersion ip_version = IPVersion::kInvalid;

  ::sockaddr_storage addr = {};
};

}  // namespace bedrock::network

#endif
