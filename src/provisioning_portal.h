#pragma once

#include "config.h"
#include "display.h"

class ProvisioningPortal {
 public:
  bool run(AppConfig &config, Display &display, uint32_t timeoutMs);
};
