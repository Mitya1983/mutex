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
#if defined(DEBUG_BUILD)
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
#if defined(DEBUG_BUILD)
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

mt::mutex::IPCMutex::IPCMutex(std::string name) :
    m_name{std::move(name)} {

    if (m_name.empty()) {
        throw std::invalid_argument("Name for IPC_Lock should be provided");
    }
#if defined(_WIN32) || defined(_WIN64)
    m_mutex = CreateSemaphoreA(nullptr, 0, 1, m_name.c_str());
    LPVOID lpMsgBuf;
    if (m_mutex == nullptr) {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast< LPSTR >(&lpMsgBuf), 0, nullptr);
        throw std::runtime_error("Failed to obtain semaphore handle " + std::string(static_cast< const char* >(lpMsgBuf)));
    }
#else
    m_mutex = sem_open(m_name.c_str(), O_CREAT, 0666, 1);
    if (m_mutex == SEM_FAILED) {
        throw std::system_error(std::error_code(errno, std::system_category()));
    }
#endif
}

mt::mutex::IPCMutex::~IPCMutex() {
#if defined(_WIN32) || defined(_WIN64)
    if (m_locked) {
        ReleaseSemaphore(m_mutex, 1, nullptr);
    }
    CloseHandle(m_mutex);
#else
    if (m_locked) {
        sem_post(m_mutex);
        sem_close(m_mutex);
    }
    sem_unlink(m_name.c_str());
    sem_destroy(m_mutex);
#endif
}

void mt::mutex::IPCMutex::lock() {
#if defined(_WIN32) || defined(_WIN64)
    WaitForSingleObject(m_mutex, INFINITE);
#else
    sem_wait(m_mutex);
#endif
    m_locked = true;
}

void mt::mutex::IPCMutex::unlock() {
    if (m_locked) {
#if defined(_WIN32) || defined(_WIN64)
        ReleaseSemaphore(m_mutex, 1, nullptr);
#else
        sem_post(m_mutex);
#endif
        m_locked = false;
    }
}

auto mt::mutex::IPCMutex::try_lock(ChronoDuration p_time_out) -> bool {
    if (std::holds_alternative< std::monostate >(p_time_out)) {
#if defined(_WIN32) || defined(_WIN64)
        if (const auto lock_result = WaitForSingleObject(m_mutex, 0); lock_result == WAIT_OBJECT_0) {
#else
        if (const auto lock_result = sem_trywait(m_mutex); lock_result == 0) {
#endif
            return true;
#if defined(_WIN32) || defined(_WIN64)
        } else if (lock_result == WAIT_TIMEOUT) {
#else
        } else if (lock_result == -1) {
            if (errno != EAGAIN) {
                throw std::system_error(std::error_code(errno, std::system_category()));
            }
#endif
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
        int32_t lock_result{0};
#if defined(_WIN32) || defined(_WIN64)
        WaitForSingleObject(m_mutex, std::chrono::duration_cast<std::chrono::milliseconds>(time_out).count());
        if (lock_result == WAIT_OBJECT_0) {
            return true;
        }
#else
        timespec timespec{};
        timespec.tv_sec = std::chrono::time_point_cast< std::chrono::seconds >(std::chrono::system_clock::now()).time_since_epoch().count() + std::chrono::duration_cast< std::chrono::seconds >(time_out).count();
        while ((lock_result = sem_timedwait(m_mutex, &timespec)) == -1 && errno == EINTR) { }
        if (lock_result == 0) {
            return true;
        }
#endif
    }
    return false;
}

auto mt::mutex::IPCMutex::name() const -> const std::string& { return m_name; }

auto mt::mutex::IPCMutex::native_handle() const -> NativeHandle { return m_mutex; }