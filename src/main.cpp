#include "hash_server.hpp"

#include <csignal>
void (*prev_handler)(int);

void sighandler(int sig)
{
    fprintf(stdout, "TCP server will be shutted down with signal %d: %s.\n", sig, strsignal(sig));
    hash_server_t::kill();

    // if not succeed next time kill by system
    signal (SIGINT, prev_handler);
}


int main()
{
    /* Reset system signalling and set it to sighandler function */
    prev_handler = signal (SIGINT, sighandler);

    auto thread_num = std::max(2u, 2*std::thread::hardware_concurrency());
    return hash_server_t::run("5555", thread_num, false);
}
