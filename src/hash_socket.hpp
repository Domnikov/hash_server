/**
 * @file hash_server.h
 * @author Domnikov Ivan
 * @brief File with class hash_socket.
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


/**
 * @brief The class proides interface woth with TCP sockets in blocking mode.
 * @details Methods create for creating and start listening, wait_new - waiting new connection and
 * @details kill for stopping server
 */
class tcp_soct_t
{
public:
    tcp_soct_t() = default;

    /**
      * @brief Object destructor. Before destroy object it will close descriptor
      */
    ~tcp_soct_t()
    {
        if (-1 != m_file_desc)
        {
            close(m_file_desc);
            m_file_desc = -1;
        }
    }

    // rule of five - delete all copy/move methods
    tcp_soct_t(const tcp_soct_t& ) = delete;
    tcp_soct_t(      tcp_soct_t&&) = delete;
    tcp_soct_t& operator=(const tcp_soct_t& ) = delete;
    tcp_soct_t& operator=(      tcp_soct_t&&) = delete;


    /**
     * @brief Close file descriptor and stop wait_new loop
     * @details After this method it can be used again after creating
     */
    void kill()
    {
        if (-1 == shutdown(m_file_desc, SHUT_RDWR))
        {
            fprintf(stderr, "Server shutdown failure!: %s\n", strerror(errno));
        }
        m_file_desc = -1;
    }


    /**
     * @brief Create server on this port and start listen it
     * @details If creation and starting listening success then function will finish normally.
     * @details Function will throw an exception if creating is failed
     * @param TCP port
     */
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


    /**
     * @brief Waiting new connection.
     * @details Method will throw exception in case of server critical error.
     * @details Method will keep trying accept new connection in case of non critical errors.
     * @param[out] New connection file descriptor
     * @return true if file descriptor have gotten and false if server sutted down.
     */
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

    /** Maximum connections queue size */
    constexpr static int max_events = 32;

    /** Opened socket file descriptor */
    int m_file_desc = -1;
};