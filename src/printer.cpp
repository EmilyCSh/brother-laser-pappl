module;
#include <atomic>
#include <chrono>
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
    enum class EnergyState
    {
        WORKING,
        IDLE,
        SLEEP,
    };

private:
    struct PrivateConstructor
    {
        explicit PrivateConstructor() = default;
    };

    std::string m_driver_name;
    std::string m_device_uri;
    std::shared_ptr<PapplDevice> m_device;

    std::atomic<EnergyState> m_energy_state;
    std::atomic<std::chrono::milliseconds::rep> m_delta_time_last_job;

public:
    Printer(PrivateConstructor /* private constructor */, std::string driver_name, std::string device_uri, std::shared_ptr<PapplDevice> device)
        : m_driver_name(std::move(driver_name)), m_device_uri(std::move(device_uri)), m_device(std::move(device)),
          m_energy_state(EnergyState::WORKING), m_delta_time_last_job(0)
    {}

    ~Printer()
    {
        static_cast<void>(reset());
    }

    Printer(const Printer&)                    = delete;
    auto operator=(const Printer&) -> Printer& = delete;
    Printer(Printer&&)                         = delete;
    auto operator=(Printer&&) -> Printer&      = delete;

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
        energystate_heartbeat();
        return send_impl(command);
    }

    [[nodiscard]] auto send_pjl_cmd(std::string_view command) noexcept -> std::expected<void, common::DeviceError>
    {
        energystate_heartbeat();

        auto result = send_impl(common::UEL);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_impl(common::PJL_PRE);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_impl(" ");
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_impl(command);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        result = send_impl(common::CRLF);
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

    auto delta_last_job_add_time(std::chrono::milliseconds duration) -> void
    {
        m_delta_time_last_job.fetch_add(duration.count(), std::memory_order_relaxed);
    }

    [[nodiscard]] auto get_delta_last_job() const -> std::chrono::milliseconds
    {
        return std::chrono::milliseconds(m_delta_time_last_job.load(std::memory_order_relaxed));
    }

    auto delta_last_job_reset() -> void
    {
        m_delta_time_last_job.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] auto get_energystate() noexcept -> EnergyState
    {
        return m_energy_state.load(std::memory_order::acquire);
    }

    auto set_energystate(EnergyState state) -> void
    {
        m_energy_state.store(state, std::memory_order::release);
    }

    auto energystate_heartbeat() -> void
    {
        set_energystate(EnergyState::WORKING);
        delta_last_job_reset();
    }

private:
    [[nodiscard]] auto send_impl(std::string_view command) noexcept -> std::expected<void, common::DeviceError>
    {
        return m_device->write_all(command);
    }

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

        result = send(common::UEL);
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
