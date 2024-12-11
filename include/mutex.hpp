// Created by Mitia Tristan on 24.10.24.
#ifndef MT_MUTEX_HPP
#define MT_MUTEX_HPP

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

namespace mt::utility::mutex {
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
        std::atomic_flag m_lock{false};
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
        std::atomic< std::thread::id > m_thread_id{std::thread::id{0}};
        std::atomic_uint8_t m_lock_counter{0};
        std::atomic_flag m_lock{false};
    };

    class SharedMutex {
      public:
        SharedMutex() = default;

        SharedMutex(const SharedMutex&) = delete;
        SharedMutex(SharedMutex&&) = delete;
        SharedMutex& operator=(const RecursiveMutex&) = delete;
        RecursiveMutex& operator=(SharedMutex&&) = delete;

        ~SharedMutex() = default;

        void lock();
        void unlock();
        void lock_shared();
        void unlock_shared();

        [[nodiscard]] auto try_lock() -> bool;
        [[nodiscard]] auto try_lock_shared() -> bool;

      private:
        std::atomic_uint8_t m_read_counter{0};
        std::atomic_flag m_lock{false};
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
        std::atomic_flag m_lock{false};
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

        ~IPCMutex() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock(ChronoDuration p_time_out = std::monostate()) const -> bool;
        [[nodiscard]] auto name() const -> const std::string&;
        [[nodiscard]] auto native_handle() const -> NativeHandle;

      private:
        std::string m_name;
        NativeHandle m_mutex{nullptr};
        std::atomic_bool m_locked{false};
    };
}  // namespace mt::utility::mutex
#endif  //MT_MUTEX_HPP
