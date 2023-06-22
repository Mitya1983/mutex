#ifndef IPC_LOCK_IPC_LOCK_HPP
#define IPC_LOCK_IPC_LOCK_HPP

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
using Mutex = HANDLE;
#else
  #include <semaphore.h>
using Mutex = sem_t*;
#endif

#include <string>
#include <chrono>
#include <variant>

namespace tristan {

    using ChronoDuration = std::
        variant< std::monostate, std::chrono::minutes, std::chrono::seconds, std::chrono::milliseconds, std::chrono::microseconds, std::chrono::nanoseconds >;

    class IPC_Lock {
    public:
        /**
        * \brief Constructor which by default locks the mutex.
        * \param lock bool. Default is true
        * \throws std::invalid_argument if name is empty
        * \throws std::system_error holding the std::error_code of std::system_category
        */
        explicit IPC_Lock(std::string name, bool lock = true);

        IPC_Lock(const IPC_Lock& other) = delete;

        IPC_Lock(IPC_Lock&& other) = delete;

        IPC_Lock& operator=(const IPC_Lock& other) = delete;

        IPC_Lock& operator=(IPC_Lock&& other) = delete;

        /**
        * \brief Destructor which implicitly unlocks the mutex
        */
        ~IPC_Lock();

        void lock();

        void unlock();

        [[nodiscard]] auto tryLock(ChronoDuration p_time_out = std::monostate()) -> bool;

        [[nodiscard]] auto name() const -> const std::string&;

    protected:
    private:
        std::string m_name;

        Mutex m_mutex;

        bool m_locked;
    };
}  // namespace tristan

#endif  //IPC_LOCK_IPC_LOCK_HPP
