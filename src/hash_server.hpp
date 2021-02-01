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



/**
 * @brief hash_server_t class
 * @details Implemented as singlton without access to created object. Can be only ran and killed.
 */
class hash_server_t
{
public:


    /**
     * @brief Static function to run hash_server
     * @details Server will be ran on port and function will finished only after server will be stopped
     * @param[in] which port to open
     * @param[in] number of threads for connection pool
     * @param[in] start with debuf mode
     * @return 0 if success -1 if failed
     */
    static bool run(const char* port, size_t thread_num, bool debug = false)
    {
        if (m_inst)
        {
            perror("hash_server was already created");
            return -1;
        }


        // Create instane of hash_serer object
        hash_server_t server{port, thread_num, debug};

        // Call function to create socket. Error messages will be shown by function
        auto socketfd = server.create_socket(port);
        if (-1 == socketfd)
        {
            return -1;
        }


        // Start listening
        if (-1 == listen(socketfd, SOMAXCONN))
        {
            perror("[E] listen failed\n");
            return -1;
        }

        // Creating epoll for connection
        int epollfd = epoll_create1(0);
        if (-1 == epollfd)
        {
            perror("[E] epoll_create1 failed\n");
            return -1;
        }

        // Creating single epoll event for connection
        epoll_event event;
        event.data.ptr = nullptr;
        event.data.fd = socketfd;
        event.events = EPOLLIN | EPOLLET;

        // Register event to epoll event loop
        if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event))
        {
            perror("[E] epoll_ctl failed\n");
            return -1;
        }

        // Array for storing events
        std::array<struct epoll_event, max_events> ev_arr;

        // Infinite loop until server will be killed
        while (server.m_run)
        {
            // Waiting connection events
            auto n = epoll_wait(epollfd, ev_arr.data(), max_events, -1);
            for (int i = 0; i < n; ++i)
            {
                int fd     = ev_arr[i].data.fd;

                // manage error case
                if (ev_arr[i].events & EPOLLERR)
                {
                    perror("Socket error\n"); close(fd);
                }

                // manage connection case
                else if (ev_arr[i].events & EPOLLIN && socketfd == fd)
                {
                    while (server.accept_connection(socketfd)) {}
                }
            }
        }

        // close socket
        close(socketfd);

        return 0;
    }


    /**
     * @brief Kill hash_server
     * @details Function only setup run flag. Server will be shutdown after terminating  all threads
     */
    static void kill()
    {
        // Set up run flag to false
        m_inst->m_run = false;
    }


private:

    /**
     * @brief Constructor for hash_server_t.
     * @details Only setup m_inst to 'this'. Other work will be done by run function.
     * @param TCT port to open
     * @param number of threads
     * @param debug mode flag
     */
    hash_server_t(const char* port, size_t thread_num, bool debug)
        :m_run(true), m_pool(thread_num), m_debug(debug)
    {
        // setup instance to current object
        m_inst = this;
    }


    /**
     * @brief Destructor for hash_server_t.
     * @details Only setup m_inst to nullptr.
     */
    virtual ~hash_server_t()
    {
        // recet instance
        m_inst = nullptr;
    }


    /**
     * @brief Create_socket function just creating socket
     * @return Open socket file descriptor
     */
    int create_socket(const char* port)
    {
        // setup ip address parameters
        addrinfo hints;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
        hints.ai_socktype = SOCK_STREAM; /* TCP */
        hints.ai_flags = AI_PASSIVE; /* All interfaces */
        addrinfo* result;
        int sockt = getaddrinfo(nullptr, port, &hints, &result);
        if (0 != sockt)
        {
            perror("[E] getaddrinfo failed");
            return -1;
        }

        addrinfo* rp = nullptr;
        int socketfd;
        for (rp = result; rp != nullptr; rp = rp->ai_next)
        {
            // create socket
            socketfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (-1 == socketfd)
            {
                continue;
            }

            // binding socket
            sockt = bind(socketfd, rp->ai_addr, rp->ai_addrlen);
            if (0 == sockt)
            {
                break;
            }

            close(socketfd);
        }

        if (nullptr == rp)
        {
            perror("[E] bind failed\n");
            return -1;
        }

        // clear result list
        freeaddrinfo(result);

        // setup nonblocking mode
        if (!make_nonblocking(socketfd))
        {
            return -1;
        }

        return socketfd;
    }


    /**
     * @brief Setup socket flags to non blocking mode
     * @param Open TCT socket file descriptor
     * @return true if succeed and false if failed
     */
    static bool make_nonblocking(int socketfd)
    {
        int flags = fcntl(socketfd, F_GETFL, 0);
        if (-1 == flags)
        {
            perror("[E] fcntl failed (F_GETFL)\n");
            return false;
        }

        flags |= O_NONBLOCK;

        if (-1 == fcntl(socketfd, F_SETFL, flags))
        {
            perror("[E] fcntl failed (F_SETFL)\n");
            return false;
        }

        return true;
    }


    /**
     * @brief Accept new connection for socket and registed it into connection pool
     * @param new TCP connection file descriptor.
     * @return true if succeed and false failed
     */
    bool accept_connection(int socketfd)
    {
        struct sockaddr in_addr;
        socklen_t in_len = sizeof(in_addr);
        int infd = accept(socketfd, &in_addr, &in_len);
        if (-1 == infd)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) // Done processing incoming connections
            {
                return false;
            }
            else
            {
                perror("[E] accept failed\n");
                return false;
            }
        }


        // print debug information
        if(m_debug)
        {
            char hbuf[NI_MAXHOST]{'\0'};
            char sbuf[NI_MAXSERV]{'\0'};
            if (getnameinfo(&in_addr, in_len, hbuf, NI_MAXHOST, sbuf, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV) == 0)
            {
                printf("[I] Accepted connection on descriptor %d(host=%s, port=%s)\n", infd, hbuf, sbuf);
            }
        }

        // setup nonblocking mode
        if (!make_nonblocking(infd))
        {
            perror("[E] make_socket_nonblocking failed\n");
            return false;
        }

        // create event for accepted connection
        auto event = new epoll_event;
        event->data.ptr = new hash_t(infd);
        event->events = EPOLLIN | EPOLLET;

        // push connection event to thread pool
        if (-1 == m_pool.push(infd, event))
        {
            perror("[E] epoll_ctl failed\n");
            return false;
        }

        return true;
    }



    /** How many event can be read in one time*/
    constexpr static int max_events = 32;

    /** hash_server instance*/
    static hash_server_t* m_inst;

    /** m_run flag. Setup by constructor and clear by kill function*/
    bool m_run;

    /** connection pool object. Creating by conscructor*/
    connection_pool_t m_pool;

    /** debug mode flag*/
    bool m_debug;
};
