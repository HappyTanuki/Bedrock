#include "networking/address.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __linux__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#error "이 플랫폼은 지원되지 않습니다."
#endif

#include <format>
#include <string>

namespace bedrock::network {

Address::~Address() = default;

Address& Address::operator=(const ::sockaddr_in& ipv4addr) {
  SetAddr(ipv4addr);
  return *this;
}

Address& Address::operator=(const ::sockaddr_in6& ipv6addr) {
  SetAddr(ipv6addr);
  return *this;
}

AddressErrorStatus Address::SetAddr(std::string_view domain_address,
                                    std::string_view port) {
  ::addrinfo hint = {};
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;

  ::addrinfo* addr;

  int status = ::getaddrinfo(domain_address.data(), port.data(), &hint, &addr);
  if (status != 0) {
    last_errno = status;
#ifdef _WIN32
    last_error_message = std::string(gai_strerrorA(status));
#else
    last_error_message = std::string(gai_strerror(status));
#endif
    return AddressErrorStatus::kAddrinfo;
  }

  for (::addrinfo* p = addr; p != nullptr; p = p->ai_next) {
    if (p->ai_family == AF_INET6) {
      SetAddr(*reinterpret_cast<::sockaddr_in6*>(p->ai_addr));
      break;
    } else if (p->ai_family == AF_INET) {
      SetAddr(*reinterpret_cast<::sockaddr_in*>(p->ai_addr));
      break;
    }
  }

  freeaddrinfo(addr);

  return AddressErrorStatus::kSuccess;
}

AddressErrorStatus Address::SetAddr(IPVersion version,
                                    std::string_view presentation_address,
                                    std::uint16_t port) {
  auto addrv4 = reinterpret_cast<::sockaddr_in*>(&addr);
  auto addrv6 = reinterpret_cast<::sockaddr_in6*>(&addr);

  switch (version) {
    case IPVersion::kIPV4: {
      addrv4->sin_family = AF_INET;
      auto error =
          ::inet_pton(AF_INET, presentation_address.data(), &addrv4->sin_addr);
      if (error == -1) {
        last_errno = GetSocketLastErrorCode();
        last_error_message = GetSocketErrorMessage(last_errno);
        return AddressErrorStatus::kFailure;
      } else if (error != 1) {
        last_errno = error;
        last_error_message = "Invalid IP string.";
        return AddressErrorStatus::kFailure;
      }
      addrv4->sin_port = htons(port);
    } break;
    case IPVersion::kIPV6: {
      addrv6->sin6_family = AF_INET6;
      auto error = ::inet_pton(AF_INET6, presentation_address.data(),
                               &addrv6->sin6_addr);
      if (error == -1) {
        last_errno = GetSocketLastErrorCode();
        last_error_message = GetSocketErrorMessage(last_errno);
        return AddressErrorStatus::kFailure;
      } else if (error != 1) {
        last_errno = error;
        last_error_message = "Invalid IP string.";
        return AddressErrorStatus::kFailure;
      }
      addrv6->sin6_port = htons(port);
    } break;
    default:
      return AddressErrorStatus::kIPVersion;
      break;
  }

  ip_version = version;
  valid = true;

  return AddressErrorStatus::kSuccess;
}

AddressErrorStatus Address::SetAddr(const ::sockaddr_storage& generic_addr) {
  const ::sockaddr* sockaddr_casted =
      reinterpret_cast<const ::sockaddr*>(&generic_addr);

  switch (sockaddr_casted->sa_family) {
    case AF_INET:
      SetAddr(*reinterpret_cast<const ::sockaddr_in*>(&generic_addr));
      break;
    case AF_INET6:
      SetAddr(*reinterpret_cast<const ::sockaddr_in6*>(&generic_addr));
      break;
    default:
      return AddressErrorStatus::kAddrinfo;
  }
  return AddressErrorStatus::kSuccess;
}

AddressErrorStatus Address::SetAddr(const ::sockaddr_in& ipv4addr) {
  if (ipv4addr.sin_family != AF_INET) {
    return AddressErrorStatus::kFailure;
  }

  static_assert(sizeof(addr) >= sizeof(ipv4addr));

  std::memcpy(&addr, &ipv4addr, sizeof(ipv4addr));

  ip_version = IPVersion::kIPV4;
  valid = true;

  return AddressErrorStatus::kSuccess;
}

AddressErrorStatus Address::SetAddr(const ::sockaddr_in6& ipv6addr) {
  if (ipv6addr.sin6_family != AF_INET6) {
    return AddressErrorStatus::kFailure;
  }

  static_assert(sizeof(addr) >= sizeof(ipv6addr));

  std::memcpy(&addr, &ipv6addr, sizeof(ipv6addr));

  ip_version = IPVersion::kIPV6;
  valid = true;

  return AddressErrorStatus::kSuccess;
}

Address::operator bedrock::DataWithStatus<std::string, AddressErrorStatus>() {
  auto addrv4 = reinterpret_cast<const ::sockaddr_in*>(&addr);
  auto addrv6 = reinterpret_cast<const ::sockaddr_in6*>(&addr);

  if (!IsValid()) {
    return {"Invalid address internal state.", AddressErrorStatus::kInternal};
  }

  switch (ip_version) {
    case IPVersion::kIPV4: {
      char addrStr[INET_ADDRSTRLEN];
      auto error =
          ::inet_ntop(AF_INET, &addrv4->sin_addr, addrStr, sizeof(addrStr));
      if (error == nullptr) {
        last_errno = GetSocketLastErrorCode();
        last_error_message = GetSocketErrorMessage(last_errno);
        return {"", AddressErrorStatus::kFailure};
      }
      return {std::format("{}:{}", addrStr, GetPort().data),
              AddressErrorStatus::kSuccess};
    }
    case IPVersion::kIPV6: {
      char addrStr[INET6_ADDRSTRLEN];
      auto error =
          ::inet_ntop(AF_INET6, &addrv6->sin6_addr, addrStr, sizeof(addrStr));
      if (error == nullptr) {
        last_errno = GetSocketLastErrorCode();
        last_error_message = GetSocketErrorMessage(last_errno);
        return {"", AddressErrorStatus::kFailure};
      }

      return {std::format("{}:{}", addrStr, GetPort().data),
              AddressErrorStatus::kSuccess};
    }
    default:
      return {"Invalid ip version was provided.",
              AddressErrorStatus::kIPVersion};
  }
}

Address::operator std::string() {
  auto result = static_cast<bedrock::DataWithStatus<
      std::string, bedrock::network::AddressErrorStatus>>(*this);

  if (result.status != AddressErrorStatus::kSuccess) {
    return "";
  }

  return result.data;
}

Address::operator const ::sockaddr*() const {
  if (!IsValid()) {
    return nullptr;
  }

  return reinterpret_cast<const ::sockaddr*>(&addr);
}

Address::operator ::sockaddr_in() const {
  if (!IsValid() || ip_version != IPVersion::kIPV4) {
    return {};
  }

  return *reinterpret_cast<const ::sockaddr_in*>(&addr);
}

Address::operator ::sockaddr_in6() const {
  if (!IsValid() || ip_version != IPVersion::kIPV6) {
    return {};
  }

  return *reinterpret_cast<const ::sockaddr_in6*>(&addr);
}

bedrock::DataWithStatus<IPVersion, AddressErrorStatus> Address::GetIPVersion()
    const {
  if (IsValid()) {
    return {ip_version, AddressErrorStatus::kSuccess};
  } else {
    return {IPVersion::kInvalid, AddressErrorStatus::kFailure};
  }
}

bedrock::DataWithStatus<std::uint16_t, AddressErrorStatus> Address::GetPort()
    const {
  switch (ip_version) {
    case IPVersion::kIPV4:
      return {::ntohs(reinterpret_cast<const ::sockaddr_in*>(&addr)->sin_port),
              AddressErrorStatus::kSuccess};
    case IPVersion::kIPV6:
      return {
          ::ntohs(reinterpret_cast<const ::sockaddr_in6*>(&addr)->sin6_port),
          AddressErrorStatus::kSuccess};
    default:
      return {0x00u, AddressErrorStatus::kIPVersion};
  }
}

}  // namespace bedrock::network