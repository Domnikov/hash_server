/**
 * @file connection_pool.h
 * @author Domnikov Ivan
 * @brief File with connectin_pool_t class.
 *
 */
#pragma once

#include "event_manager.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <string_view>

namespace net
{

/**
* @brief connection_pool_t class
* @details Contains vector of threads. Each thread hash epoll event loop.
* @details Epoll event loop trigger every second even if there's no events to monitor m_rum status.
* @details To push new connection need to call method 'add_connection' with new file descriptor
* @details When connection is closed it will be deleted and buffer cleaned by method itself
* @details event_manager is manager for single connection. See event_manager_t for details
* @details must have following methods:
* @code static epoll_event create_event(int fd)
* @code static void delete_event(epoll_event* event)
* @code void process_data()
* @code bool is_eof()
*/
template <class event_manager>
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
        for (size_t  i = 0; i < m_thread_num; i++)
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
                            event_manager::delete_event(ev_arr[i]);
                        }
                        // New data available to read
                        else if (ev_arr[i].events & EPOLLIN)
                        {
                            auto manager = static_cast<event_manager*>(ev_arr[i].data.ptr);
                            manager->process_data();
                            if(manager->is_eof())
                            {
                                event_manager::delete_event(ev_arr[i]);
                            }
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



    /**
    * @brief Stop event pool and destroy all threads
    * @details Setup m_run flag to false and join all threads.
    * @details If epoll_wait will be changed to infinite timeout then despructor must be changed as well
    * @details After this method there's no way to start it again. Need to create new connection_pool
    */
    void stop()
    {
        // Finishing threads
        m_run = false;
        for (auto& thr : m_pool)
        {
            thr.thr.join();
        }
    }


    /**
    * @brief Function to add new connection to event loop
    * @details Function get connection file descroptor.
    * @details To manage server load used "peek next" algoritm.
    * @param[in] New file descriptor to open
    * @return Returns 0 in case of success, S-1 in case of error
    */
    int add_connection(int fd)
    {
        // Get next thread id
        static int counter = 0;
        int thread_id = counter++ % m_thread_num;

        auto event = event_manager::create_event(fd);

        // register connection to thread event loop
        return epoll_ctl(m_pool[thread_id].epollfd, EPOLL_CTL_ADD, fd, &event);
    }


private:

    /** how many maximum events to wait*/
    constexpr static int max_events = 32;

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

} // namespace net
