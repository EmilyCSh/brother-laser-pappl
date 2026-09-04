module;
#include <print>
#include <string>
#include <cstddef>
#include <string_view>
#include <memory>
#include <utility>
#include <expected>
export module brother_laser.printer;
import brother_laser.common;
import brother_laser.pappl;

export namespace brother_laser {

class Printer
{
private:
    struct PrivateConstructor
    {
        explicit PrivateConstructor() = default;
    };

    std::string m_driver_name;
    std::string m_device_uri;
    std::shared_ptr<PapplDevice> m_device;

public:
    Printer(PrivateConstructor /* private constructor */, std::string driver_name, std::string device_uri, std::shared_ptr<PapplDevice> device)
        : m_driver_name(std::move(driver_name)), m_device_uri(std::move(device_uri)), m_device(std::move(device))
    {}

    ~Printer()
    {
        static_cast<void>(reset());
    }

    Printer(const Printer&)                    = delete;
    auto operator=(const Printer&) -> Printer& = delete;
    Printer(Printer&&)                         = default;
    auto operator=(Printer&&) -> Printer&      = default;

    [[nodiscard]] static auto create_shared(const std::string& driver_name, const std::string& device_uri)
        -> std::expected<std::shared_ptr<Printer>, common::DeviceError>
    {
        auto device = PapplDevice::create_shared(driver_name, device_uri);

        if (!device)
        {
            return std::unexpected(device.error());
        }

        auto printer = std::make_shared<Printer>(PrivateConstructor {}, driver_name, device_uri, device.value());

        auto result = printer->init();
        if (!result)
        {
            return std::unexpected(result.error());
        }

        return printer;
    }

    [[nodiscard]] auto send(std::string_view command) noexcept -> std::expected<void, common::DeviceError>
    {
        return m_device->write_all(command);
    }

    [[nodiscard]] auto send_uel() noexcept -> std::expected<void, common::DeviceError>
    {
        return send(common::UEL);
    }

    [[nodiscard]] auto send_pjl_cmd(std::string_view command) noexcept -> std::expected<void, common::DeviceError>
    {
        auto result = send_uel();
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send(common::PJL_PRE);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send(" ");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send(command);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send(common::CRLF);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        return {};
    }

    [[nodiscard]] auto receive_string(size_t output_len) -> std::expected<std::string, common::DeviceError>
    {
        return m_device->read_string(output_len);
    }

    [[nodiscard]] auto testprint() noexcept -> std::expected<void, common::DeviceError>
    {
        return send_pjl_cmd("EXECUTE TESTPRINT");
    }

private:
    [[nodiscard]] auto reset() noexcept -> std::expected<void, common::DeviceError>
    {
        /* Ensure printer is really reset */
        auto result = send_pjl_cmd("RESET");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_pjl_cmd("USTATUSOFF");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_uel();
        if (!result)
        {
            return std::unexpected(result.error());
        }

        return {};
    }

    [[nodiscard]] auto init() -> std::expected<void, common::DeviceError>
    {
        auto result = reset();
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_pjl_cmd("USTATUS DEVICE=VERBOSE");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_pjl_cmd("USTATUS JOB=ON");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_pjl_cmd("USTATUS PAGE=ON");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        return {};
    }
};

void test()
{
    std::println("Hello from brother_laser::test()");
}

} // namespace brother_laser
