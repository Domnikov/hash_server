#include "../src/connection_pool.hpp"
#include "../src/hash_calc.hpp"
#include "../src/event_manager.hpp"

#include <gtest/gtest.h>
#include <fcntl.h>


class hash_calc_test : public ::testing::Test 
{
    public:
        void SetUp   () {}
        void TearDown() {}

    protected:
};


class test_event_manager_t: public event_manager_t<hash_t, false>
{
    public:
        test_event_manager_t(int fd):event_manager_t(fd){}
        bool test_read_data  ()                       {return read_data  (      );}
        bool test_parse_lines(std::string_view buffer){return parse_lines(buffer);}
        bool test_write_data (std::string_view buffer){return write_data (buffer);}
};

std::string test_str = "1111111";
std::string etalon   = "7FA8282AD93047A4D6FE6111C93B308A\n";


TEST_F(hash_calc_test, GoogleTestTest) 
{
    ASSERT_EQ(1, 1) << "Google test doesn't work";
}


TEST_F(hash_calc_test, manual_hash) 
{

    hash_t hash;
    hash.process(test_str);
    auto result = hash.get_result();

    ASSERT_EQ(etalon, result) << "Hash calculation test failed";
}


TEST_F(hash_calc_test, create_delete_event)
{
    const int test_fd = 111;
    auto event = hash_ev_manager_t::create_event(test_fd);

    ASSERT_EQ(static_cast<hash_ev_manager_t*>(event.data.ptr)->get_fd(), test_fd) << "Event cannot be created";
    hash_ev_manager_t::delete_event(event);
    ASSERT_FALSE(event.data.ptr) << "Event cannot be deleted";
}


TEST_F(hash_calc_test, hash_one_line)
{
    int pipefd[2];

    ASSERT_EQ(pipe(pipefd), 0) << "Test pipe cannot be created ["<<strerror(errno)<<"]";

    test_event_manager_t manager(pipefd[1]);

    ASSERT_TRUE(manager.test_parse_lines(test_str + "\n")) << "Writing to buffer failure";

    auto buf = std::make_unique<char[]>(etalon.size());
    int count = read(pipefd[0], buf.get(), etalon.size());
    ASSERT_EQ(count, etalon.size()) << "Wrong read buffer size";

    ASSERT_EQ(strncmp(buf.get(), etalon.c_str(), etalon.size()), 0) << "Received hash doesn't match";

    close(pipefd[0]);
    close(pipefd[1]);
}


TEST_F(hash_calc_test, hash_multi_line)
{
    int pipefd[2];

    ASSERT_EQ(pipe(pipefd), 0) << "Test pipe cannot be created ["<<strerror(errno)<<"]";

    auto test_newline = test_str + "\n";
    test_newline += test_newline;
    test_newline += test_newline;

    std::string etalon_x_4 = etalon;
    etalon_x_4 += etalon_x_4;
    etalon_x_4 += etalon_x_4;

    test_event_manager_t manager(pipefd[1]);

    ASSERT_TRUE(manager.test_parse_lines(test_str + "\n")) << "Writing to buffer failure";
    ASSERT_TRUE(manager.test_parse_lines(test_str + "\n")) << "Writing to buffer failure";
    ASSERT_TRUE(manager.test_parse_lines(test_str + "\n")) << "Writing to buffer failure";
    ASSERT_TRUE(manager.test_parse_lines(test_str + "\n")) << "Writing to buffer failure";

    auto buf = std::make_unique<char[]>(etalon_x_4.size());
    int count = read(pipefd[0], buf.get(), etalon_x_4.size());
    ASSERT_EQ(count, etalon_x_4.size()) << "Wrong read buffer size";

    ASSERT_EQ(strncmp(buf.get(), etalon_x_4.c_str(), etalon_x_4.size()), 0) << "Received hash doesn't match";

    close(pipefd[0]);
    close(pipefd[1]);
}


TEST_F(hash_calc_test, hash_continues_writing)
{
    int pipefd[2];

    ASSERT_EQ(pipe(pipefd), 0) << "Test pipe cannot be created ["<<strerror(errno)<<"]";
    int retval = fcntl( pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
    ASSERT_EQ(retval, 0) << "Cannot make nonblocking pipe";

    test_event_manager_t manager(pipefd[1]);

    ASSERT_TRUE(manager.test_parse_lines(test_str)) << "Writing to buffer failure";

    auto buf = std::make_unique<char[]>(etalon.size());
    int count = read(pipefd[0], buf.get(), etalon.size());
    ASSERT_EQ(count, -1) << "Incompliete must return -1";

    ASSERT_TRUE(manager.test_parse_lines("\n")) << "Writing to buffer failure";
    count = read(pipefd[0], buf.get(), etalon.size());

    ASSERT_EQ(count, etalon.size()) << "Wrong read buffer size";
    ASSERT_EQ(strncmp(buf.get(), etalon.c_str(), etalon.size()), 0) << "Received hash doesn't match";

    close(pipefd[0]);
    close(pipefd[1]);
}

