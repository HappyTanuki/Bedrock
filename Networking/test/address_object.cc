#include <iostream>
#include <string>

#include "networking/address.h"

int main() {
  bedrock::network::Address addr;
  addr.SetAddr("happytanuki.kr", "800");

  std::cout << static_cast<std::string>(addr) << std::endl;

  return 0;
}
