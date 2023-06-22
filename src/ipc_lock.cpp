#include "ipc_lock.hpp"

#include <fcntl.h>

#include <stdexcept>
#include <system_error>
#include <cassert>
#include <thread>

tristan::IPC_Lock::IPC_Lock(std::string name, bool lock) :
    m_name(std::move(name)),
    m_mutex(nullptr),
    m_locked(false) {

    if (m_name.empty()) {
        throw std::invalid_argument("Name for IPC_Lock should be provided");
    }

    m_mutex = sem_open(m_name.c_str(), O_CREAT, 0666, 1);
    if (m_mutex == SEM_FAILED) {
        throw std::system_error(std::error_code(errno, std::system_category()));
    }
    if (lock) {
        this->lock();
    }
}

tristan::IPC_Lock::~IPC_Lock() { this->unlock(); }

void tristan::IPC_Lock::lock() {
    sem_wait(m_mutex);
    m_locked = true;
}

void tristan::IPC_Lock::unlock() {
    if (m_locked) {
        sem_post(m_mutex);
        sem_close(m_mutex);
        m_locked = false;
    }
}

auto tristan::IPC_Lock::tryLock(ChronoDuration p_time_out) -> bool {
    if (std::holds_alternative< std::monostate >(p_time_out)) {
        auto lock_result = sem_trywait(m_mutex);
        if (lock_result == 0) {
            m_locked = true;
        }
    } else {
        std::chrono::nanoseconds time_out;
        std::visit(
            [&time_out](auto&& duration) -> void {
                using DurationType = std::decay_t< decltype(duration) >;
                if constexpr (std::is_same_v< DurationType, std::monostate >) {
                    assert(!"Unreachable code");
                } else {
                    time_out = std::chrono::duration_cast< std::chrono::nanoseconds >(duration);
                }
            },
            p_time_out);

        struct timespec timespec { };

        timespec.tv_sec = std::chrono::time_point_cast< std::chrono::seconds >(std::chrono::system_clock::now()).time_since_epoch().count()
                        + std::chrono::duration_cast< std::chrono::seconds >(time_out).count();
        int32_t lock_result;
        while ((lock_result = sem_timedwait(m_mutex, &timespec)) == -1 && errno == EINTR) { }
        if (lock_result == 0) {
            m_locked = true;
        }
    }
    return m_locked;
}

auto tristan::IPC_Lock::name() const -> const std::string& { return m_name; }
