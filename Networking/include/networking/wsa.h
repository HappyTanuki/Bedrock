#ifdef _WIN32
// clang-format off
// 이 헤더들은 무조건 이 순서로 include 되어야만 함
#include <winsock2.h>
#include <windows.h>
// clang-format on

#include <string>
#include <string_view>

namespace bedrock::network {

class WSAErrorMsg {
 public:
  explicit WSAErrorMsg(int code) : buf_(nullptr) {
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&buf_, 0, nullptr);
    if (!buf_) {
      fallback_ = "Unknown WSA error";
    }
  }

  ~WSAErrorMsg() {
    if (buf_) {
      LocalFree(buf_);
      buf_ = nullptr;
    }
  }

  std::string str() const { return buf_ ? buf_ : fallback_; }

 private:
  LPSTR buf_;                       // FormatMessage에서 할당된 버퍼
  const char* fallback_ = nullptr;  // 실패 시 반환용
};

}  // namespace bedrock::network

#endif

namespace bedrock::network {

struct WSAManager {
 public:
  WSAManager();
  ~WSAManager();
};

}  // namespace bedrock::network
