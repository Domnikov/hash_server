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

struct fd_helper_t
{
    public:
        fd_helper_t(int fd) : m_fd(fd) {}
        fd_helper_t(std::nullptr_t) : m_fd(-1) {}
        fd_helper_t() : fd_helper_t(nullptr) {}

        operator int() const {return m_fd;}

        bool operator !=(std::nullptr_t) const {return m_fd != -1;}
    private:
        int m_fd;
};


struct fd_deleter_t
{
    using pointer = fd_helper_t; // Internal type is a pointer

    void operator()( int fd )
    {
        fprintf(stderr, "deleter:%d\n", fd);
        close(fd);
    }
};


