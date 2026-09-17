// SPDX-License-Identifier: GPL-3.0-only
/*
 *  src/printer.cpp
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

module;
#include <string>
#include <cstddef>
#include <future>
#include <string_view>
#include <memory>
#include <utility>
#include <expected>
export module brother_laser.printer;
import brother_laser.common;
import brother_laser.pappl;
import brother_laser.printer_worker;

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
    PrinterWorker m_worker;

public:
    Printer(
        PrivateConstructor /* private constructor */,
        std::string driver_name,
        std::string device_uri,
        std::shared_ptr<PapplDevice> device)
        : m_driver_name(std::move(driver_name)), m_device_uri(std::move(device_uri)), m_device(std::move(device)),
          m_worker(m_device)
    {}

    ~Printer()
    {
        try
        {
            static_cast<void>(reset());
        }
        catch (...)
        {
            return;
        }
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

    [[nodiscard]] auto send(std::string_view command) -> std::expected<void, common::DeviceError>
    {
        auto command_copy = std::string(command);

        return m_worker
            .submit(
                [command = std::move(command_copy)](PapplDevice& device) -> std::expected<void, common::DeviceError> {
                    return device.write_all(command);
                })
            .get();
    }

    [[nodiscard]] auto send_pjl_cmd(std::string_view command) -> std::expected<void, common::DeviceError>
    {
        return m_worker.submit(pjl_command(command)).get();
    }

    [[nodiscard]] static auto pjl_command(std::string_view command) -> PrinterWorker::Job
    {
        auto command_copy = std::string(command);

        return [command = std::move(command_copy)](PapplDevice& device) -> std::expected<void, common::DeviceError> {
            auto result = device.write_all(common::UEL);
            if (result)
            {
                result = device.write_all(common::PJL_PRE);
            }
            if (result)
            {
                result = device.write_all(" ");
            }
            if (result)
            {
                result = device.write_all(command);
            }
            if (result)
            {
                result = device.write_all(common::CRLF);
            }
            if (result)
            {
                result = device.write_all(common::UEL);
            }

            return result;
        };
    }

    [[nodiscard]] auto receive_string(size_t output_len) -> std::expected<std::string, common::DeviceError>
    {
        const auto promise = std::make_shared<std::promise<std::expected<std::string, common::DeviceError>>>();
        auto result        = promise->get_future();

        static_cast<void>(
            m_worker.submit([output_len, promise](PapplDevice& device) -> std::expected<void, common::DeviceError> {
                promise->set_value(device.read_string(output_len));
                return std::expected<void, common::DeviceError> {};
            }));

        return result.get();
    }

    [[nodiscard]] auto testprint() -> std::expected<void, common::DeviceError>
    {
        return send_pjl_cmd("EXECUTE TESTPRINT");
    }

private:
    [[nodiscard]] auto reset() -> std::expected<void, common::DeviceError>
    {
        /* Ensure printer is really reset */
        return m_worker
            .submit_many({
                pjl_command("RESET"),
                pjl_command("USTATUSOFF"),
            })
            .get();
    }

    [[nodiscard]] auto init() -> std::expected<void, common::DeviceError>
    {
        auto result = reset();
        if (!result)
        {
            return std::unexpected(result.error());
        }

        return m_worker
            .submit_many({
                pjl_command("USTATUS DEVICE=VERBOSE"),
                pjl_command("USTATUS JOB=ON"),
                pjl_command("USTATUS PAGE=ON"),
            })
            .get();
    }
};

} // namespace brother_laser
