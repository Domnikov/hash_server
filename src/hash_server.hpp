/**
 * @file hash_server.h
 * @author Domnikov Ivan
 * @brief File with hash server class.
 *
 */
#pragma once

#include "hash_calc.hpp"
#include "connection_pool.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <cerrno>



class tcp_soct_t
{
    constexpr static int max_events = 32;

public:
    tcp_soct_t() = default;

    ~tcp_soct_t()
    {
        if (-1 != m_file_desc)
        {
            close(m_file_desc);
            m_file_desc = -1;
        }
    }

    tcp_soct_t(const tcp_soct_t& ) = delete;
    tcp_soct_t(      tcp_soct_t&&) = delete;
    tcp_soct_t& operator=(const tcp_soct_t& ) = delete;
    tcp_soct_t& operator=(      tcp_soct_t&&) = delete;


    void kill()
    {
        if (-1 == shutdown(m_file_desc, SHUT_RDWR))
        {
            fprintf(stderr, "Server shutdown failure!: %s\n", strerror(errno));
        }
        m_file_desc = -1;
    }


    void create(uint16_t port)
    {
        // Creating socket descriptor
        m_file_desc = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if (-1 == m_file_desc)
        {
            throw std::runtime_error("Socket cannot be created!");
        }

        // Setup socket option REUSE ADDRESS
        int enable = 1;
        if (-1 == setsockopt( m_file_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) ))
        {
            fprintf(stderr, "Reuse address option cannot be used\n");
        }

        // Binding socket to file descriptor
        sockaddr_in addr;
        memset( &addr , 0, sizeof(addr) );
        addr.sin_family = AF_INET;
        addr.sin_port = htons (port);
        addr.sin_addr.s_addr = INADDR_ANY ;
        if (-1 == bind( m_file_desc, (sockaddr*) &addr, sizeof(addr) ))
        {
            close(m_file_desc);
            m_file_desc = -1;
            throw std::runtime_error(std::string("Socket binding error[") + strerror(errno) + "]");
        }


        // Start listening socket
        if (-1 == listen( m_file_desc, max_events))
        {
            close(m_file_desc);
            m_file_desc = -1;
            throw std::runtime_error(std::string("Socket start listen error[") + strerror(errno) + "]");
        }
    }


    bool wait_new(int& file_descr)
    {
        while ( (file_descr = accept( m_file_desc, NULL, 0)) < 0)
        {
            if (errno == EWOULDBLOCK  || errno == EAGAIN      || errno == ENONET     ||
                errno == EPROTO       || errno == ENOPROTOOPT || errno == EOPNOTSUPP ||
                errno == ENETDOWN     || errno == ENETUNREACH || errno == EHOSTDOWN  ||
                errno == EHOSTUNREACH || errno == ECONNABORTED)
            {
                fprintf(stderr, "Connection accept error: %s\n", strerror(errno));
            }
            else if (EINVAL == errno)
            {
                // Server shutdown
                return false;
            }
            else
            {
                close(m_file_desc);
                m_file_desc = -1;
                throw std::runtime_error(std::string("Socket Listening error[") + strerror(errno) + "]");
            }
        }
        return true;
    }

private:
    int m_file_desc = -1;
};


template <class Connection_t>
class server_t
{
public:
    server_t(int thread_num):m_pool(thread_num){}


    virtual ~server_t() = default;


    void run(int port)
    {
        m_conct.create(port);

        int new_fd = -1;
        while (m_conct.wait_new(new_fd))
        {
            // create event for accepted connection
            auto event = new epoll_event;
            event->data.ptr = new hash_t(new_fd);
            event->events = EPOLLIN | EPOLLET;

            // push connection event to thread pool
            if (-1 == m_pool.push(new_fd, event))
            {
                perror("[E] epoll_ctl failed\n");
            }
        }
    }


    void kill()
    {
        m_conct.kill();
        m_pool.stop();
    }

private:
    /** Instance for managing connections*/
    Connection_t m_conct;

    /** Connection pool object. Creating by conscructor*/
    connection_pool_t m_pool;
};
