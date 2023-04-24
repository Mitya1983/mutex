#include "ipc_lock.hpp"

#include <fcntl.h>

#include <stdexcept>
#include <system_error>

tristan::IPC_Lock::IPC_Lock(std::string name, bool lock) :
    m_name(std::move(name)) {

    if (m_name.empty()){
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

void tristan::IPC_Lock::lock() { sem_wait(m_mutex); }

void tristan::IPC_Lock::unlock() {
    sem_post(m_mutex);
    sem_close(m_mutex);
}

auto tristan::IPC_Lock::name() const -> const std::string& { return m_name; }
