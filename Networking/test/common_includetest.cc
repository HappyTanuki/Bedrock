#include <iostream>

#include "common.h"

int main() {
  auto fature = bedrock::intrinsic::GetCPUFeatures();
  bedrock::intrinsic::IsCpuEnabledFeature(fature, "AESNI");
  std::cout << "Include test successful!" << std::endl;
  return 0;
}
