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
    class mutex {
      public:
        mutex() = default;

        mutex(const mutex&) = delete;
        mutex(mutex&&) = delete;
        mutex& operator=(const mutex&) = delete;
        mutex& operator=(mutex&&) = delete;

        ~mutex() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock() -> bool;

      private:
        std::atomic_flag m_lock;
    };

    class recursive_mutex {
      public:
        recursive_mutex() = default;

        recursive_mutex(const recursive_mutex&) = delete;
        recursive_mutex(recursive_mutex&&) = delete;
        recursive_mutex& operator=(const recursive_mutex&) = delete;
        recursive_mutex& operator=(recursive_mutex&&) = delete;

        ~recursive_mutex() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock() -> bool;

      private:
        std::atomic< std::thread::id > m_thread_id{std::thread::id{}};
        std::atomic_uint8_t m_lock_counter{0};
        std::atomic_flag m_lock;
    };

    class shared_mutex {
      public:
        shared_mutex() = default;

        shared_mutex(const shared_mutex&) = delete;
        shared_mutex(shared_mutex&&) = delete;
        shared_mutex& operator=(const shared_mutex&) = delete;
        shared_mutex& operator=(shared_mutex&&) = delete;

        ~shared_mutex() = default;

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

    class spinlock {
      public:
        spinlock() = default;

        spinlock(const spinlock&) = delete;
        spinlock(spinlock&&) = delete;
        spinlock& operator=(const spinlock&) = delete;
        spinlock& operator=(spinlock&&) = delete;

        ~spinlock() = default;

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock() -> bool;

      private:
        std::atomic_flag m_lock;
    };

#if defined(_WIN32) || defined(_WIN64)
    using native_handle = HANDLE;
#else
    using native_handle = sem_t *;
#endif

    using chrono_duration
        = std::variant< std::monostate, std::chrono::minutes, std::chrono::seconds, std::chrono::milliseconds, std::chrono::microseconds, std::chrono::nanoseconds >;

    class ipc_mutex {
      public:
        explicit ipc_mutex(std::string name);

        ipc_mutex(const ipc_mutex& other) = delete;
        ipc_mutex(ipc_mutex&& other) = delete;
        ipc_mutex& operator=(const ipc_mutex& other) = delete;
        ipc_mutex& operator=(ipc_mutex&& other) = delete;

        ~ipc_mutex();

        void lock();
        void unlock();

        [[nodiscard]] auto try_lock(chrono_duration p_time_out = std::monostate()) -> bool;
        [[nodiscard]] auto name() const -> const std::string&;
        [[nodiscard]] auto native_handle() const -> native_handle;

      private:
        std::string m_name;
        mt::mutex::native_handle m_mutex{nullptr};
        std::atomic_bool m_locked{false};
    };
}  // namespace mt::mutex
#endif  //MUTEX_INCLUDE_MUTEX_HPP
