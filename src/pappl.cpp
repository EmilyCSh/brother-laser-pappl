module;
#include <pappl/pappl.h>
#include <cstddef>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
export module brother_laser:pappl;
import :common;

export namespace brother_laser::pappl {

struct PapplDeviceDeleter
{
    void operator()(pappl_device_t* device) const
    {
        if (device != nullptr)
        {
            papplDeviceClose(device);
        }
    }
};

using PapplDevicePtr = std::unique_ptr<pappl_device_t, PapplDeviceDeleter>;

//NOLINTNEXTLINE(bugprone-exception-escape)
auto device_open(const std::string& driver_name, const std::string& device_uri) noexcept -> std::expected<PapplDevicePtr, common::DeviceError>
{
    auto* device = papplDeviceOpen(device_uri.c_str(), driver_name.c_str(), nullptr, nullptr);
    if (device == nullptr)
    {
        return std::unexpected(common::DeviceError::OpenFailed);
    }

    return PapplDevicePtr {device};
};

//NOLINTNEXTLINE(bugprone-exception-escape)
auto device_write(const PapplDevicePtr& device, const std::span<const std::byte> buffer) noexcept -> std::expected<size_t, common::DeviceError>
{
    const auto bytes_written = papplDeviceWrite(device.get(), buffer.data(), buffer.size());
    if (bytes_written < 0)
    {
        return std::unexpected(common::DeviceError::WriteFailed);
    }

    return static_cast<size_t>(bytes_written);
};

auto device_write_all(const PapplDevicePtr& device, const std::span<const std::byte> buffer) noexcept -> std::expected<void, common::DeviceError>
{
    size_t total_bytes_written = 0;

    while (total_bytes_written < buffer.size())
    {
        const auto remaining     = buffer.subspan(total_bytes_written);
        const auto bytes_written = device_write(device, remaining);
        if (!bytes_written)
        {
            return std::unexpected(bytes_written.error());
        }

        total_bytes_written += *bytes_written;
    }

    return {};
};

auto device_write_all(const PapplDevicePtr& device, const std::string_view buffer) noexcept -> std::expected<void, common::DeviceError>
{
    return device_write_all(device, std::as_bytes(std::span(buffer)));
};

//NOLINTNEXTLINE(bugprone-exception-escape)
auto device_read(const PapplDevicePtr& device, std::span<std::byte> buffer) noexcept -> std::expected<std::span<std::byte>, common::DeviceError>
{
    const auto bytes_read = papplDeviceRead(device.get(), buffer.data(), buffer.size());
    if (bytes_read < 0)
    {
        return std::unexpected(common::DeviceError::ReadFailed);
    }

    return buffer.first(static_cast<size_t>(bytes_read));
};

auto device_read_all(const PapplDevicePtr& device, std::span<std::byte> buffer) noexcept -> std::expected<std::span<std::byte>, common::DeviceError>
{
    size_t total_bytes_read = 0;

    while (total_bytes_read < buffer.size())
    {
        const auto remaining  = buffer.subspan(total_bytes_read);
        const auto bytes_read = device_read(device, remaining);
        if (!bytes_read)
        {
            return std::unexpected(bytes_read.error());
        }

        total_bytes_read += bytes_read->size();
    }

    return buffer.first(total_bytes_read);
};

auto device_read_all(const PapplDevicePtr& device, size_t bytes) -> std::expected<std::vector<std::byte>, common::DeviceError>
{
    std::vector<std::byte> buffer(bytes);
    const auto bytes_read = device_read_all(device, std::as_writable_bytes(std::span(buffer)));
    if (!bytes_read)
    {
        return std::unexpected(bytes_read.error());
    }

    buffer.resize(bytes_read->size());
    return buffer;
};

auto device_read_string(const PapplDevicePtr& device, size_t string_len) -> std::expected<std::string, common::DeviceError>
{
    std::string buffer(string_len, '\0');
    const auto bytes_read = device_read_all(device, std::as_writable_bytes(std::span(buffer)));
    if (!bytes_read)
    {
        return std::unexpected(bytes_read.error());
    }

    buffer.resize(bytes_read->size());
    return buffer;
};

} // namespace brother_laser::pappl