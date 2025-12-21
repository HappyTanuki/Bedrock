#ifndef BEDROCK_NETWORKING_NETWORKING_ADDRESS_H_
#define BEDROCK_NETWORKING_NETWORKING_ADDRESS_H_

#ifdef _WIN32
#define NOMINMAX
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

#include "common/types_enums.h"

namespace bedrock::network {

enum class IPVersion { kInvalid, kIPV4 = AF_INET, kIPV6 = AF_INET6 };

enum class AddressErrorStatus {
  kSuccess,
  kFailure,
  kInternal,
  kIPVersion,
  kAddrinfo,
  kInetPton,
  kInetNtop
};

class Address {
 public:
  Address() {}
  explicit Address(std::string_view domain_address, std::string_view port) {
    SetAddr(domain_address, port);
  }
  Address(IPVersion version, std::string_view presentation_address,
          std::uint16_t port) {
    SetAddr(version, presentation_address, port);
  }
  explicit Address(const ::sockaddr_in& ipv4addr) { SetAddr(ipv4addr); }
  explicit Address(const ::sockaddr_in6& ipv6addr) { SetAddr(ipv6addr); }

  Address& operator=(const ::sockaddr_in& ipv4addr);
  Address& operator=(const ::sockaddr_in6& ipv6addr);

  AddressErrorStatus SetAddr(std::string_view domain_address,
                             std::string_view port);
  AddressErrorStatus SetAddr(const ::sockaddr_in& ipv4addr);
  AddressErrorStatus SetAddr(const ::sockaddr_in6& ipv6addr);
  AddressErrorStatus SetAddr(IPVersion version,
                             std::string_view presentation_address,
                             std::uint16_t port);

  explicit operator bedrock::DataWithStatus<std::string, AddressErrorStatus>();
  explicit operator std::string();

  explicit operator ::sockaddr_in() const;
  explicit operator ::sockaddr_in6() const;

  bedrock::DataWithStatus<IPVersion, AddressErrorStatus> GetIPVersion() const;
  bedrock::DataWithStatus<std::uint16_t, AddressErrorStatus> GetPort() const;
  bool IsValid() { return valid; }

  std::string last_error_message;
  int last_errno = 0;

 private:
  bool valid = false;

  IPVersion ip_version = IPVersion::kInvalid;

  ::sockaddr_storage addr;
};

}  // namespace bedrock::network

#endif
