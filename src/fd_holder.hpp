/**
 * @file file_descr.h
 * @author Domnikov Ivan
 * @brief RAII class for prevent file descriptor leaking
 *
 */
#pragma once

#include <cstddef>

#include <unistd.h>
#include <cstdio>

struct fd_holder_t
{
    public:
        fd_holder_t(int fd) : m_fd(fd) {}
        fd_holder_t(std::nullptr_t) : m_fd(-1) {}
        fd_holder_t() : fd_holder_t(nullptr) {}

        operator int() const {return m_fd;}

        bool operator !=(std::nullptr_t) const {return m_fd != -1;}
    private:
        int m_fd;
};


struct fd_deleter_t
{
    using pointer = fd_holder_t;

    void operator()( int fd )
    {
        close(fd);
    }
};


