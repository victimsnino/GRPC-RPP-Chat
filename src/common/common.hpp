#pragma once

#include <stdexcept>

#define ENSURE(condition) if (!condition) { throw std::runtime_error{" condition " #condition " failed"};  }
