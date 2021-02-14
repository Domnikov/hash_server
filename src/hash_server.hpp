/**
 * @file hash_server.h
 * @author Domnikov Ivan
 * @brief File with server class.
 *
 */
#pragma once

#include "hash_socket.hpp"
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

namespace net
{

/**
 * @brief Implementation of server.
 * @details Class itself just use Connection, connection_pool with Processor.
 * @details Function run use create and wait_new methods of Connection and if new connection then it put
 * @details new file descriptor into connection pool
 */
template <class Connection, class Processor>
class server_t
{
public:
    server_t(int thread_num):m_pool(thread_num){}
    virtual ~server_t() = default;


    /**
     * @brief Run server with connection loop.
     * @details Method will never return until server will be killed with 'void kill()' method.
     * @details If eny error during runnig server happaned then exception will be generated and it
     * @details must be caught.
     * @param[in] TCP port to listen
     */
    void run(int port)
    {
        m_conct.create(port);

        int new_fd = -1;
        while (m_conct.wait_new(new_fd))
        {
            // push connection event to thread pool
            if (-1 == m_pool.add_connection(new_fd))
            {
                perror("[E] epoll_ctl failed\n");
            }
        }
    }


    /**
     * @brief Killing server and connection_pool
     */
    void kill()
    {
        m_conct.kill();
    }

private:
    /** Instance for managing connections*/
    Connection m_conct;

    /** Connection pool object. Creating by conscructor*/
    connection_pool_t<Processor> m_pool;
};


/** Alias for hash_server_t*/
using hash_server_t = server_t<tcp_soct_t, hash_ev_manager_t>;

} // namespace net
