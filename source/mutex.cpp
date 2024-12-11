// Created by Mitia Tristan on 24.10.24.
#include "mutex.hpp"

#include <fcntl.h>

#include <stdexcept>
#include <system_error>
#include <cassert>

void mt::mutex::Mutex::lock() {
    while (m_lock.test_and_set(std::memory_order_consume)) {
        m_lock.wait(true, std::memory_order_consume);
    }
}

void mt::mutex::Mutex::unlock() {
    m_lock.clear(std::memory_order_release);
    m_lock.notify_all();
}

auto mt::mutex::Mutex::try_lock() -> bool {
    if (m_lock.test_and_set(std::memory_order_consume)) {
        return false;
    }
    return true;
}

void mt::mutex::RecursiveMutex::lock() {
    if (m_lock_counter.load(std::memory_order::relaxed) > 0) {
        if (m_thread_id.load(std::memory_order::relaxed) == std::this_thread::get_id()) {
            ++m_lock_counter;
            return;
        }
    }
    while (m_lock.test_and_set(std::memory_order_consume)) {
        m_lock.wait(true, std::memory_order_relaxed);
    }
    m_thread_id.store(std::this_thread::get_id(), std::memory_order_relaxed);
    ++m_lock_counter;
}

void mt::mutex::RecursiveMutex::unlock() {
    if (m_lock_counter.load(std::memory_order_relaxed) == 0) {
#ifdef MT_DEBUG
        throw std::runtime_error("unlock is called without corresponding lock call");
#else
        return;
#endif
    }
    --m_lock_counter;
    if (m_lock_counter.load(std::memory_order_relaxed) == 0) {
        m_lock.clear(std::memory_order_release);
        m_lock.notify_all();
    }
}

auto mt::mutex::RecursiveMutex::try_lock() -> bool {
    if (m_lock_counter.load(std::memory_order::relaxed) > 0) {
        if (m_thread_id.load(std::memory_order::relaxed) == std::this_thread::get_id()) {
            ++m_lock_counter;
            return true;
        }
        return false;
    }
    if (m_lock.test_and_set(std::memory_order_consume)) {
        return false;
    }
    m_thread_id.store(std::this_thread::get_id(), std::memory_order_relaxed);
    ++m_lock_counter;
    return true;
}

void mt::mutex::SharedMutex::lock() {
    while (m_lock.test_and_set(std::memory_order_consume)) {
        m_lock.wait(true, std::memory_order_consume);
    }
}

void mt::mutex::SharedMutex::unlock() {
    m_lock.clear(std::memory_order_release);
    m_lock.notify_all();
}

void mt::mutex::SharedMutex::lock_shared() {
    if (m_read_counter.load(std::memory_order_relaxed) > 0) {
        ++m_read_counter;
        return;
    }
    while (m_lock.test_and_set(std::memory_order_consume)) {
        m_lock.wait(true, std::memory_order_consume);
    }
    ++m_read_counter;
}

void mt::mutex::SharedMutex::unlock_shared() {
    if (m_read_counter.load(std::memory_order_relaxed) == 0) {
#ifdef MT_DEBUG
        throw std::runtime_error("unlock_shared is called without corresponding lock_shared call");
#else
        return;
#endif
    }
    --m_read_counter;
    if (m_read_counter.load(std::memory_order_relaxed) == 0) {
        m_lock.clear(std::memory_order_release);
        m_lock.notify_all();
    }
}

auto mt::mutex::SharedMutex::try_lock() -> bool {
    if (m_lock.test_and_set(std::memory_order_consume)) {
        return false;
    }
    return true;
}

auto mt::mutex::SharedMutex::try_lock_shared() -> bool {
    if (m_read_counter.load(std::memory_order_relaxed) > 0) {
        ++m_read_counter;
        return true;
    }
    if (m_lock.test_and_set(std::memory_order_consume)) {
        return false;
    }
    ++m_read_counter;
    return true;
}

void mt::mutex::Spinlock::lock() {
    while (m_lock.test_and_set(std::memory_order_consume)) { }
}

void mt::mutex::Spinlock::unlock() { m_lock.clear(std::memory_order_release); }

auto mt::mutex::Spinlock::try_lock() -> bool {
    if (m_lock.test_and_set(std::memory_order_consume)) {
        return false;
    }
    return true;
}

//TODO: Windows implementation

mt::mutex::IPCMutex::IPCMutex(std::string name) :
    m_name{std::move(name)} {

    if (m_name.empty()) {
        throw std::invalid_argument("Name for IPC_Lock should be provided");
    }

    m_mutex = sem_open(m_name.c_str(), O_CREAT, 0666, 1);
    if (m_mutex == SEM_FAILED) {
        throw std::system_error(std::error_code(errno, std::system_category()));
    }
}

void mt::mutex::IPCMutex::lock() {
    sem_wait(m_mutex);
    m_locked.store(true, std::memory_order_relaxed);
}

void mt::mutex::IPCMutex::unlock() {
    if (m_locked.load(std::memory_order_relaxed)) {
        sem_post(m_mutex);
        sem_close(m_mutex);
        m_locked.store(false, std::memory_order_relaxed);
    }
}

auto mt::mutex::IPCMutex::try_lock(ChronoDuration p_time_out) const -> bool {
    if (std::holds_alternative< std::monostate >(p_time_out)) {
        if (const auto lock_result = sem_trywait(m_mutex); lock_result == 0) {
            return true;
        } else if (lock_result == -1) {
            if (errno != EAGAIN) {
                throw std::system_error(std::error_code(errno, std::system_category()));
            }
            return false;
        }
    } else {
        std::chrono::nanoseconds time_out;
        std::visit(
            [&time_out]< typename Duration >(Duration&& duration) -> void {
                using DurationType = std::decay_t< Duration >;
                if constexpr (std::is_same_v< DurationType, std::monostate >) {
                    assert(!"Unreachable code");
                } else {
                    time_out = std::chrono::duration_cast< std::chrono::nanoseconds >(duration);
                }
            },
            p_time_out);

        timespec timespec{};

        timespec.tv_sec = std::chrono::time_point_cast< std::chrono::seconds >(std::chrono::system_clock::now()).time_since_epoch().count()
                        + std::chrono::duration_cast< std::chrono::seconds >(time_out).count();
        int32_t lock_result;
        while ((lock_result = sem_timedwait(m_mutex, &timespec)) == -1 && errno == EINTR) { }
        if (lock_result == 0) {
            return true;
        }
    }
    return false;
}

auto mt::mutex::IPCMutex::name() const -> const std::string& { return m_name; }

auto mt::mutex::IPCMutex::native_handle() const -> NativeHandle { return m_mutex; }