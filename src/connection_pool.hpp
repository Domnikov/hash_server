/**
 * @file connection_pool.h
 * @author Domnikov Ivan
 * @brief File with connectin_pool_t class.
 *
 */
#pragma once

#include "hash_calc.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <thread>
#include <vector>
#include <atomic>
#include <cstring>


/**
* @brief connection_pool_t class
* @details Multi function class (sorry OOP). Contains vector of threads. Each thread hash epoll event loop.
* @details Epoll event loop trigger every second even if there's no events to monitor m_rum status.
* @details To push new connection neet to call functin push with connection file descriptor and event object.
*/
class connection_pool_t
{
public:


    /**
    * @brief Connection_pool_t constructor.
    * @details Creates vector of threads with size thread_num. In each thread register epoll event loop.
    * @param[in] Number of threads.
    */
    connection_pool_t(size_t thread_num)
        :m_thread_num(thread_num), m_run(true)
    {
        // Reserve vector data
        m_pool.reserve(m_thread_num);

        // Creating threads for thread pool in loop
        for(size_t  i = 0; i < m_thread_num; i++)
        {
            // Creating new event loop
            auto epollfd = epoll_create1(0);
            if (-1 == epollfd)
            {
                fprintf(stderr, "[E] thread pool[%ld] epoll_create1 failed\n", i);
                continue;
            }

            // Creating thread
            thread_data_t pool_elem{epollfd, std::thread([this, epollfd, i]{
                std::array<struct epoll_event, max_events> ev_arr;

                // Event loop
                while (m_run)
                {
                    auto n = epoll_wait(epollfd, ev_arr.data(), max_events, 1000);
                    for (int i = 0; i < n; ++i)
                    {
                        //Close and clean if Error or disconnected
                        if (ev_arr[i].events & EPOLLERR || ev_arr[i].events & EPOLLHUP )
                        {
                            if(ev_arr[i].data.ptr)
                            {
                                auto hash = static_cast<hash_t*>(ev_arr[i].data.ptr);
                                close(hash->get_fd());
                                delete hash;
                                ev_arr[i].data.ptr = nullptr;
                            }
                        }
                        // New data available to read
                        else if (ev_arr[i].events & EPOLLIN)
                        {
                            while(read_data(&ev_arr[i])){}
                        }
                    }
                }
            })};

            m_pool.emplace_back(std::move(pool_elem));

        }
    }



    /**
    * @brief Connection_pool_t destructor
    * @details Setup m_run flag to false and join all threads.
    * @details If epoll_wait will be changed to infinite timeout then despructor must be changed as well
    */
    virtual ~connection_pool_t()
    {
        if (m_run)
        {
            stop();
        }
    }


    void stop()
    {
        // Finishing threads
        m_run = false;
        for(auto& thr : m_pool)
        {
            thr.thr.join();
        }
    }


    /**
    * @brief Function to add new connection to event loop
    * @details Function get connection file descroptor and event object.
    * @details To manage server load used "peek next" algoritm.
    * @details If event was not accepted it can be repeated.
    * @param[in] buffer with new data
    * @param[in] new data length
    * @return Returns 0 in case of success, -1 in case of error
    */
    int push(int fd, epoll_event* event)
    {
        // Get next thread id
        static int counter = 0;
        int thread_id = counter++ % m_thread_num;

        if(-1 == m_pool[thread_id].epollfd)
        {
            return -1;
        }

        // register connection to thread event loop
        return epoll_ctl(m_pool[thread_id].epollfd, EPOLL_CTL_ADD, fd, event);
    }


protected:

    /** how many maximum events to wait*/
    constexpr static int max_events = 32;


    /**
    * @brief Read data from triggered event
    * @details Function get triggered event and reading data.
    * @param[in] Triggered event
    * @return true if it has more data to read and false in opposite way.
    */
    static bool read_data(epoll_event* event)
    {
        if (!event->data.ptr)
        {
            return false;
        }

        const int BUF_SIZE = 8192;
        auto hash = static_cast<hash_t*>(event->data.ptr);
        char buf[BUF_SIZE];

        auto count = read(hash->get_fd(), buf, BUF_SIZE);

        if (-1 == count) // All data was read
        {
            return false;
        }
        else if (0 == count) // EOF - remote closed connection
        {
            close(hash->get_fd());
            delete hash;
            event->data.ptr = nullptr;
        }

        // New data available
        return process_data(hash, buf, count, false);
    }


    /**
    * @brief Scan buffer. if new line found write it down to socket
    * @param[in] Hash object
    * @param[in] Received buffer
    * @param[in] Buffer size
    */
    static bool process_data(hash_t* hash, const char* buf, int size, bool skip_socket_test)
    {
        const char* begin = buf;
        while (auto end = (char*)std::memchr(begin,'\n',size)) // Checking end of line. By task no need to check \r
        {
            auto len = end-begin;

            // Calculating hash
            hash->calc_hash(begin, len);

            // Output hash to
            char out[hash_t::HAST_STR_LEN];
            hash->get_hex_str(out);

            // write result to output buffer
            if (!write_data(hash->get_fd(), out, hash_t::HAST_STR_LEN, skip_socket_test))
            {
                return false;
            }

            // jump over \n
            begin = end+1;
            size -= len+1;
        }

        // Calc hash to the rest of buffer
        if(size)
        {
            hash->calc_hash(begin, size);
        }

        return true;
    }


    /**
    * @brief Write data to file rescriptor
    * @details Before write this functin check if socket is still available
    * @param[in] File descriptor
    * @param[in] Buffer to send
    * @param[in] Buffer length
    * @return true if succeed and false in opposite way.
    */
    static bool write_data(int fd, const char* buf, size_t len, bool skip_socket_test)
    {
        if(!skip_socket_test)
        {
            // Checking if socket is alive
            int error = 0;
            socklen_t optlen = sizeof (error);
            int retval = getsockopt (fd, SOL_SOCKET, SO_ERROR, &error, &optlen);

            if (0 != retval)
            {
                perror( "error getting socket error code: ");
                return false;
            }

            if (0 != error)
            {
                perror( "socket error: ");
                return false;
            }
        }
        // Write data to socket
        return (-1 != write(fd, buf, len));
    }

    /** thread data. socket file descriptor and thread object*/
    struct thread_data_t
    {
        int epollfd;
        std::thread thr;
    };


    /** Thread pool*/
    std::vector<thread_data_t> m_pool;

    /** How many threads*/
    size_t m_thread_num;

    /** Run flag. setup to true when object created and to false when object is destroying*/
    std::atomic_bool m_run;
};
