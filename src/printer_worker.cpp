module;
#include <condition_variable>
#include <chrono>
#include <expected>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>
#include <stop_token>
export module brother_laser.printer_worker;
import brother_laser.common;
import brother_laser.pappl;

export namespace brother_laser {

class PrinterWorker
{
public:
    constexpr static auto WORKING_SLEEP_TIME = std::chrono::milliseconds(10);
    constexpr static auto IDLE_SLEEP_TIME    = std::chrono::milliseconds(100);
    constexpr static auto SLEEP_SLEEP_TIME   = std::chrono::seconds(1);

    constexpr static auto IDLE_TRANSITION_TIME  = std::chrono::minutes(1);
    constexpr static auto SLEEP_TRANSITION_TIME = std::chrono::seconds(10);

    enum class EnergyState
    {
        WORKING,
        IDLE,
        SLEEP,
    };

    using Job = std::function<std::expected<void, common::DeviceError>(PapplDevice&)>;

private:
    std::shared_ptr<PapplDevice> m_device;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<std::function<void(PapplDevice&)>> m_jobs;
    EnergyState m_energy_state = EnergyState::WORKING;
    std::chrono::steady_clock::time_point m_last_job_add_time;
    std::jthread m_thread;

public:
    explicit PrinterWorker(std::shared_ptr<PapplDevice> device)
        : m_device(std::move(device)), m_last_job_add_time(std::chrono::steady_clock::now())
    {
        m_thread = std::jthread([this](std::stop_token stop_token) -> void {
            thread_function(std::move(stop_token));
        });
    }

    ~PrinterWorker()
    {
        stop();
    }

    PrinterWorker(const PrinterWorker&)                    = delete;
    auto operator=(const PrinterWorker&) -> PrinterWorker& = delete;
    PrinterWorker(PrinterWorker&&)                         = delete;
    auto operator=(PrinterWorker&&) -> PrinterWorker&      = delete;

    [[nodiscard]] auto submit(Job job) -> std::future<std::expected<void, common::DeviceError>>
    {
        const auto promise = std::make_shared<std::promise<std::expected<void, common::DeviceError>>>();
        auto result        = promise->get_future();

        enqueue([job = std::move(job), promise](PapplDevice& device) -> void {
            promise->set_value(job(device));
        });

        return result;
    }

    [[nodiscard]] auto submit_many(std::vector<Job> jobs) -> std::future<std::expected<void, common::DeviceError>>
    {
        const auto promise = std::make_shared<std::promise<std::expected<void, common::DeviceError>>>();
        auto result        = promise->get_future();

        if (jobs.empty())
        {
            promise->set_value(std::expected<void, common::DeviceError> {});
            return result;
        }

        enqueue([jobs = std::move(jobs), promise](PapplDevice& device) -> void {
            for (const auto& job : jobs)
            {
                const auto job_result = job(device);
                if (!job_result)
                {
                    promise->set_value(job_result);
                    return;
                }
            }

            promise->set_value(std::expected<void, common::DeviceError> {});
        });

        return result;
    }

    auto stop() -> void
    {
        m_thread.request_stop();
        m_condition.notify_all();

        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    [[nodiscard]] auto get_energy_state() -> EnergyState
    {
        const std::scoped_lock lock(m_mutex);
        return m_energy_state;
    }

private:
    auto enqueue(std::function<void(PapplDevice&)> job) -> void
    {
        {
            const std::scoped_lock lock(m_mutex);
            m_jobs.push(std::move(job));
        }

        m_condition.notify_one();
    }

    [[nodiscard]] static auto get_sleep_time(EnergyState energy_state) -> std::chrono::milliseconds
    {
        switch (energy_state)
        {
            case EnergyState::WORKING:
                return WORKING_SLEEP_TIME;
            case EnergyState::IDLE:
                return IDLE_SLEEP_TIME;
            case EnergyState::SLEEP:
                return SLEEP_SLEEP_TIME;
        }

        std::unreachable();
    }

    auto update_energy_state(std::chrono::steady_clock::time_point now) -> void
    {
        const auto elapsed = now - m_last_job_add_time;

        if (elapsed >= SLEEP_TRANSITION_TIME)
        {
            m_energy_state = EnergyState::SLEEP;
        }
        else if (elapsed >= IDLE_TRANSITION_TIME)
        {
            m_energy_state = EnergyState::IDLE;
        }
    }

    auto thread_function(std::stop_token stop_token) -> void
    {
        while (!stop_token.stop_requested())
        {
            std::unique_lock lock(m_mutex, std::defer_lock);
            lock.lock();

            const auto sleep_time = get_sleep_time(m_energy_state);

            m_condition.wait_for(lock, sleep_time, [this, &stop_token] -> bool {
                return stop_token.stop_requested() || !m_jobs.empty();
            });

            if (stop_token.stop_requested())
            {
                return;
            }

            if (!m_jobs.empty())
            {
                const auto job = std::move(m_jobs.front());
                m_jobs.pop();

                m_energy_state      = EnergyState::WORKING;
                m_last_job_add_time = std::chrono::steady_clock::now();

                lock.unlock();
                job(*m_device);
            }
            else
            {
                update_energy_state(std::chrono::steady_clock::now());
                lock.unlock();
            }
        }
    }
};

} // namespace brother_laser
