#pragma once

#include <cstddef>

namespace modules::constants {

// Max number of simultaneously opened session on a single module
constexpr size_t nSessionsPerModuleLimit = 64;

// Max number of simultaneously registered services in Messanger module
namespace messanger {
    constexpr size_t nServicesLimit       = 32;
    constexpr size_t nMaxRequestTimeoutMs = 5000;
    constexpr size_t nSessionsLimit       = 256;
};

};