#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>

#include <string>
#include <string_view>

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

  std::string_view str() const { return buf_ ? buf_ : fallback_; }

 private:
  LPSTR buf_;                       // FormatMessage에서 할당된 버퍼
  const char* fallback_ = nullptr;  // 실패 시 반환용
};

#endif
