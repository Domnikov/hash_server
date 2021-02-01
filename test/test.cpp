#include "../src/connection_pool.hpp"
#include "../src/hash_calc.hpp"

#include <gtest/gtest.h>
#include <fcntl.h>


class hash_calc_test : public ::testing::Test 
{
    public:
        void SetUp   () {}
        void TearDown() {}

    protected:
};


class pool_test: public connection_pool_t
{
    public:
	static bool test_buffer_read(hash_t* hash, const char* buf, int size)
	{
	    return process_data(hash, buf, size, true);
	}
};

char etalon[] = "7FA8282AD93047A4D6FE6111C93B308A\n";


TEST_F(hash_calc_test, GoogleTestTest) 
{
    ASSERT_EQ(1, 1) << "Google test doesn't work";
}


TEST_F(hash_calc_test, manual_hash) 
{
    char str   [] = "1111111";
    char buf[hash_t::HAST_STR_LEN];

    hash_t hash(0);
    hash.calc_hash(str, strlen(str));
    hash.get_hex_str(buf);

    ASSERT_EQ(strncmp(buf, etalon, hash_t::HAST_STR_LEN), 0) << "Hash calculation test failed";
}

TEST_F(hash_calc_test, hash_one_line) 
{
    int pipefd[2];

    ASSERT_EQ(pipe(pipefd), 0) << "Test pipe cannot be created";

    char str   [] = "1111111\n";
    char buf[hash_t::HAST_STR_LEN];
    
    hash_t hash(pipefd[1]);

    pool_test::test_buffer_read(&hash, str, strlen(str));

    int count = read(pipefd[0], buf, hash_t::HAST_STR_LEN);
    
    ASSERT_EQ(count, hash_t::HAST_STR_LEN) << "Wrong read buffer size";

    ASSERT_EQ(strncmp(buf, etalon, hash_t::HAST_STR_LEN), 0) << "Received hash doesn't match";
    
    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(hash_calc_test, hash_multi_line) 
{
    int pipefd[2];

    ASSERT_EQ(pipe(pipefd), 0) << "Test pipe cannot be created";

    char str   [] = "1111111\n1111111\n1111111\n1111111\n";
    std::string etalon_x_4;
    
    etalon_x_4 += etalon;
    etalon_x_4 += etalon;
    etalon_x_4 += etalon;
    etalon_x_4 += etalon;
    
    char buf[hash_t::HAST_STR_LEN*4];
    
    hash_t hash(pipefd[1]);

    pool_test::test_buffer_read(&hash, str, strlen(str));

    int count = read(pipefd[0], buf, hash_t::HAST_STR_LEN*4);
    
    ASSERT_EQ(count, hash_t::HAST_STR_LEN*4) << "Wrong read buffer size";

    ASSERT_EQ(strncmp(buf, etalon_x_4.c_str(), hash_t::HAST_STR_LEN*4), 0) << "Received hash doesn't match";
    
    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(hash_calc_test, hash_incomplete_line) 
{
    int pipefd[2];

    ASSERT_EQ(pipe(pipefd), 0) << "Test pipe cannot be created";

    int retval = fcntl( pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);

    ASSERT_EQ(retval, 0) << "Cannot make nonblocking pipe";

    char str   [] = "1111111";
    
    char buf[hash_t::HAST_STR_LEN];
    
    hash_t hash(pipefd[1]);

    pool_test::test_buffer_read(&hash, str, strlen(str));

    int count = read(pipefd[0], buf, hash_t::HAST_STR_LEN);


    
    ASSERT_EQ(count, -1) << "String incomplete. Result must be EOF(-1)";

    close(pipefd[0]);
    close(pipefd[1]);
}


