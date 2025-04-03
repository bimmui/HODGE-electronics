#include "gpio.h"
#include <cstdint>

extern "C" {
::std::uint32_t cxxbridge1$return_five() noexcept;

void cxxbridge1$toggle_loop() noexcept;

void cxxbridge1$set_pin(::std::int32_t pin, ::std::int32_t level) noexcept {
  void (*set_pin$)(::std::int32_t, ::std::int32_t) = ::set_pin;
  set_pin$(pin, level);
}
} // extern "C"

::std::uint32_t return_five() noexcept {
  return cxxbridge1$return_five();
}

void toggle_loop() noexcept {
  cxxbridge1$toggle_loop();
}
