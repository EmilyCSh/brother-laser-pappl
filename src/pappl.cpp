module;
#include <mutex>
#include <pappl/base.h>
#include <pappl/device.h>
#include <cstddef>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
export module brother_laser.pappl;
import brother_laser.common;

export namespace brother_laser {

class PapplDevice
{
private:
    struct PrivateConstructor
    {
        explicit PrivateConstructor() = default;
    };

    pappl_device_t* m_pappl_device;
    std::mutex m_mutex;

public:
    PapplDevice(PrivateConstructor /* private constructor */, pappl_device_t* pappl_device) : m_pappl_device(pappl_device)
    {}

    PapplDevice(const PapplDevice&)               = delete;
    auto operator=(PapplDevice) -> PapplDevice&   = delete;
    PapplDevice(PapplDevice&&)                    = delete;
    auto operator=(PapplDevice&&) -> PapplDevice& = delete;

    ~PapplDevice()
    {
        // FIXME: Needs to be logged
        if (m_pappl_device != nullptr)
        {
            papplDeviceClose(m_pappl_device);
        }
    }

    [[nodiscard]] static auto create_shared(const std::string& driver_name, const std::string& device_uri)
        -> std::expected<std::shared_ptr<PapplDevice>, common::DeviceError>
    {
        auto* device = papplDeviceOpen(device_uri.c_str(), driver_name.c_str(), nullptr, nullptr);

        if (device == nullptr)
        {
            return std::unexpected(common::DeviceError::OpenFailed);
        }

        return std::make_shared<PapplDevice>(PrivateConstructor {}, device);
    }

    [[nodiscard]] auto write(std::span<const std::byte> buffer) noexcept -> std::expected<size_t, common::DeviceError>
    {
        const std::scoped_lock<std::mutex> lock(m_mutex);
        return write_impl(buffer);
    }

    [[nodiscard]] auto write_all(std::span<const std::byte> buffer) noexcept -> std::expected<void, common::DeviceError>
    {
        size_t total_bytes_written = 0;

        {
            const std::scoped_lock<std::mutex> lock(m_mutex);
            while (total_bytes_written < buffer.size())
            {
                const auto remaining     = buffer.subspan(total_bytes_written);
                const auto bytes_written = write_impl(remaining);
                if (!bytes_written)
                {
                    return std::unexpected(bytes_written.error());
                }

                total_bytes_written += *bytes_written;
            }
        }

        return {};
    }

    [[nodiscard]] auto write_all(std::string_view buffer) noexcept -> std::expected<void, common::DeviceError>
    {
        return write_all(std::as_bytes(std::span(buffer)));
    }

    [[nodiscard]] auto read(std::span<std::byte> buffer) noexcept -> std::expected<std::span<std::byte>, common::DeviceError>
    {
        const std::scoped_lock<std::mutex> lock(m_mutex);
        return read_impl(buffer);
    }

    [[nodiscard]] auto read_all(std::span<std::byte> buffer) noexcept -> std::expected<std::span<std::byte>, common::DeviceError>
    {
        size_t total_bytes_read = 0;
        {
            const std::scoped_lock<std::mutex> lock(m_mutex);
            while (total_bytes_read < buffer.size())
            {
                const auto remaining  = buffer.subspan(total_bytes_read);
                const auto bytes_read = read_impl(remaining);
                if (!bytes_read)
                {
                    return std::unexpected(bytes_read.error());
                }
                total_bytes_read += bytes_read->size();
            }
        }
        return buffer.first(total_bytes_read);
    }

    [[nodiscard]] auto read_all(size_t bytes) -> std::expected<std::vector<std::byte>, common::DeviceError>
    {
        std::vector<std::byte> buffer(bytes);
        const auto bytes_read = read_all(std::as_writable_bytes(std::span(buffer)));
        if (!bytes_read)
        {
            return std::unexpected(bytes_read.error());
        }

        buffer.resize(bytes_read->size());
        return buffer;
    }

    [[nodiscard]] auto read_string(size_t string_len) -> std::expected<std::string, common::DeviceError>
    {
        std::string buffer(string_len, '\0');
        const auto bytes_read = read_all(std::as_writable_bytes(std::span(buffer)));
        if (!bytes_read)
        {
            return std::unexpected(bytes_read.error());
        }

        buffer.resize(bytes_read->size());
        return buffer;
    }

private:
    [[nodiscard]] auto write_impl(std::span<const std::byte> buffer) noexcept -> std::expected<size_t, common::DeviceError>
    {
        const auto bytes_written = papplDeviceWrite(m_pappl_device, buffer.data(), buffer.size());

        if (bytes_written < 0)
        {
            return std::unexpected(common::DeviceError::WriteFailed);
        }

        return static_cast<size_t>(bytes_written);
    }

    [[nodiscard]] auto read_impl(std::span<std::byte> buffer) noexcept -> std::expected<std::span<std::byte>, common::DeviceError>
    {
        const auto bytes_read = papplDeviceRead(m_pappl_device, buffer.data(), buffer.size());
        if (bytes_read < 0)
        {
            return std::unexpected(common::DeviceError::ReadFailed);
        }

        return buffer.first(static_cast<size_t>(bytes_read));
    }
};

} // namespace brother_laser