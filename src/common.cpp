module;
#include <cstdint>
#include <string_view>
export module brother_laser.common;
export namespace brother_laser::common {

constexpr std::string_view CRLF    = "\r\n";
constexpr std::string_view UEL     = "\033%-12345X@PJL\r\n";
constexpr std::string_view PJL_PRE = "@PJL";

enum class DeviceError : std::uint8_t
{
    OpenFailed,
    ReadFailed,
    WriteFailed,
};

} // namespace brother_laser::common