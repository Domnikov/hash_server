/**
 * @file io manager.h
 * @author Domnikov Ivan
 * @brief File with event_manager_t class.
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
#include <string_view>


namespace net
{

/**
 * @brief Event manager provides interface for single connection
 * @details Functions of event_manager is: creating/deleting event, reading/writing data,
 * @details call processor to process each line and get its result.
 * @details After reading need to check is_eof and delete event_manager if eof is true.
 * @details Processor is any class with folliwing methods:
 * @code void process(std::string_view);
 * @code std::string_view get_result();
 * @details parameter IS_TCP  will choose write(fifo, pipe) or send(socket) method
 */
template <class Processor, bool IS_TCP>
class event_manager_t
{
public:
    /**
     * @brief Static method to create event for given file descriptor
     * @param[in] File descriptor
     * @result epoll_event object with event_manager as raw pointer in event.data.ptr
     */
    static epoll_event create_event(int fd)
    {
        // create event for accepted connection
        epoll_event event;
        event.data.ptr = new event_manager_t(fd);
        event.events = EPOLLIN | EPOLLET;

        return event;
    }

    /**
     * @brief Static deleter for event_manager from fived epoll_event and close socket
     * @details After executing this method raw data pointer in given event will be set and nullptr
     * @param epoll_event as raw poiner
     */
    static void delete_event(epoll_event& event)
    {
        if(event.data.ptr)
        {
            auto manager = static_cast<event_manager_t*>(event.data.ptr);
            delete manager;
            event.data.ptr = nullptr;
        }
    }

    /**
     * @brief Read all available data from file descriptor, process it and sent result to socket
     * @details After calling this method need to check is_eof is descriptor is closed a
     * @details And delete closed descriptors with calling  delete_event static method
     */
    void process_data()
    {
        while(read_data()){}
    }

    /**
     * @brief Return if end of file was reached or descriptor was closed.
     * @details This method only return eof flag. But the flag itself setup be void process_data()
     * @return
     */
    bool is_eof()
    {
        return m_eof;
    }

    /**
     * @brief Get event_manager file descriptor
     * @return file descriptor
     */
    int get_fd(){return m_fd;}

protected:
    event_manager_t(int fd) :m_fd(fd){}


    /**
     * @brief Class deleter. Before delete object it close file descriptor
     */
    ~event_manager_t()
    {
        if(m_fd != -1)
        {
            close(m_fd);
        }
    }


    // rule of five - delete all copy/move methods
    event_manager_t(const event_manager_t& ) = delete;
    event_manager_t(      event_manager_t&&) = delete;
    event_manager_t& operator=(const event_manager_t& ) = delete;
    event_manager_t& operator=(      event_manager_t&&) = delete;


    /**
     * @brief Read READ_BUF_SIZE data from file descriptor
     * @details After reading data will be parsed and processed line by line.
     * @details If some data will be without following newline symbol ('\n')
     * @details It will be processed but result will be read during next session when newline
     * @details Symbol will be given. If connection is closed and EOF is reached it will
     * @details setup EOF flag. After this method is_eof must be checked and event_manager must be
     * @details deleted with static deleter delete_event(epoll_event* event)
     * @return true if more data to read exist and false in opposite
     */
    bool read_data()
    {

        auto count = read(m_fd, rd_buf, READ_BUF_SIZE);

        if (-1 == count) // All data was read
        {
            return false;
        }
        else if (0 == count) // EOF - remote closed connection
        {
            m_eof = true;
            return false;
        }

        // New data available
        return parse_lines({rd_buf, static_cast<std::string_view::size_type>(count)});
    }


    /**
     * @brief Parsing data and process it line by line and send result to file descriptor
     * @details If some data will be without following newline symbol ('\n')
     * @details It will be processed but result will be read during next session when newline
     * @details Symbol will be given.
     * @param buffer as string_view
     * @return true is sending data was success or there was nothing to send. False is sending data failed
     */
    bool parse_lines(std::string_view buffer)
    {
        auto begin = buffer.data();
        auto size = buffer.size();
        while (auto end = (char*)std::memchr(begin,'\n',size)) // Checking end of line. By task no need to check \r
        {
            std::string_view::size_type len = end-begin;

            // Calculating hash
            m_processor.process({begin, len});

            auto result = m_processor.get_result();
            if (result.size() && !write_data(result))
            {
                fprintf(stderr, "%s\n", strerror(errno));
                return false;
            }

            begin = end+1;
            size -= len+1;
        }

        // Calc hash to the rest of buffer
        m_processor.process({begin, size});

        return true;
    }


    /**
     * @brief Send data to file descriptor
     * @param Buffer to send as string_view
     * @return True if sending succeed and false in otherwise
     */
    bool write_data(std::string_view buffer)
    {
        if constexpr (IS_TCP)
        {
            return (-1 != send(m_fd, buffer.data(), buffer.size(), MSG_NOSIGNAL));
        }
        else
        {
            return write(m_fd, buffer.data(), buffer.size());
        }

    }

    /** Saved file descriptor. Used for thread pool*/
    int m_fd;
    Processor m_processor;
    bool m_eof = false;

    /** Read buffer size*/
    static const int READ_BUF_SIZE = 8192;

    /** Read buffer*/
    char rd_buf[READ_BUF_SIZE];
};

/** Alias for using with hash_t as Processor*/
using hash_ev_manager_t = event_manager_t<processors::hash_t, true>;

} // namespace net

