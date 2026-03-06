// Created by Mitia Tristan on 24.10.24.
#ifndef MUTEX_INCLUDE_MUTEX_HPP
#define MUTEX_INCLUDE_MUTEX_HPP

#include <atomic>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
  #define NOMINMAX
  #include <windows.h>
#else
#include <semaphore.h>
#endif

#include <chrono>
#include <string>
#include <variant>

namespace mt::mutex {
    class Mutex {
      public:
        Mutex() = default;

        Mutex(const Mutex&) = delete;
        Mutex(Mutex&&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        Mutex& operator=(Mutex&&) = delete;

        ~Mutex() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock() -> bool;

      private:
        std::atomic_flag m_lock;
    };

    class RecursiveMutex {
      public:
        RecursiveMutex() = default;

        RecursiveMutex(const RecursiveMutex&) = delete;
        RecursiveMutex(RecursiveMutex&&) = delete;
        RecursiveMutex& operator=(const RecursiveMutex&) = delete;
        RecursiveMutex& operator=(RecursiveMutex&&) = delete;

        ~RecursiveMutex() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock() -> bool;

      private:
        std::atomic< std::thread::id > m_thread_id{std::thread::id{}};
        std::atomic_uint8_t m_lock_counter{0};
        std::atomic_flag m_lock;
    };

    class SharedMutex {
      public:
        SharedMutex() = default;

        SharedMutex(const SharedMutex&) = delete;
        SharedMutex(SharedMutex&&) = delete;
        SharedMutex& operator=(const SharedMutex&) = delete;
        SharedMutex& operator=(SharedMutex&&) = delete;

        ~SharedMutex() = default;

        void lock();
        void unlock();
        void lock_shared();
        void unlock_shared();

        [[nodiscard]] auto try_lock() -> bool;
        [[nodiscard]] auto try_lock_shared() -> bool;

      private:
        // 0 = unlocked, -1 = write-locked, >0 = number of active readers
        std::atomic< int32_t > m_state{0};
    };

    class Spinlock {
      public:
        Spinlock() = default;

        Spinlock(const Spinlock&) = delete;
        Spinlock(Spinlock&&) = delete;
        Spinlock& operator=(const Spinlock&) = delete;
        Spinlock& operator=(Spinlock&&) = delete;

        ~Spinlock() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock() -> bool;

      private:
        std::atomic_flag m_lock;
    };

#if defined(_WIN32) || defined(_WIN64)
    using NativeHandle = HANDLE;
#else
    using NativeHandle = sem_t *;
#endif

    using ChronoDuration
        = std::variant< std::monostate, std::chrono::minutes, std::chrono::seconds, std::chrono::milliseconds, std::chrono::microseconds, std::chrono::nanoseconds >;

    class IPCMutex {
      public:
        explicit IPCMutex(std::string name);

        IPCMutex(const IPCMutex& other) = delete;
        IPCMutex(IPCMutex&& other) = delete;
        IPCMutex& operator=(const IPCMutex& other) = delete;
        IPCMutex& operator=(IPCMutex&& other) = delete;

        ~IPCMutex();

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock(ChronoDuration p_time_out = std::monostate()) -> bool;
        [[nodiscard]] auto name() const -> const std::string&;
        [[nodiscard]] auto native_handle() const -> NativeHandle;

      private:
        std::string m_name;
        NativeHandle m_mutex{nullptr};
        std::atomic_bool m_locked{false};
    };
}  // namespace mt::mutex
#endif  //MUTEX_INCLUDE_MUTEX_HPP
