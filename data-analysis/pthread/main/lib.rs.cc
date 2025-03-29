#include <cstdint>

extern "C" {
::std::uint32_t cxxbridge1$return_five() noexcept;
} // extern "C"

::std::uint32_t return_five() noexcept {
  return cxxbridge1$return_five();
}
