#include "hash_server.hpp"

#include <csignal>
#include <string.h>
#include <cstdio>

void (*system_handler)(int);
server_t<tcp_soct_t>* server_ptr = nullptr;

void sighandler(int sig)
{
    fprintf(stdout, "TCP server will be shutted down with signal %d: %s.\n", sig, strsignal(sig));
    if(server_ptr)
    {
        server_ptr->kill();
    }

    // if not succeed next time kill by system
    signal (SIGINT, system_handler);
}


int main(int argc, char **argv)
{
    // Read port number from command line
    char wrong_msg[] = "Port is not provided via command line parameters!\n\n\tUse: hash_server XXXX - where XXXX - port number\n";

    if (argc != 2)
    {
      fprintf(stderr, "%s", wrong_msg);
      return -1;
    }

    auto port = std::atoi(argv[1]);

    if(0 == port)
    {
        fprintf(stderr, "%s", wrong_msg);
        return -1;
    }

    // Reset system signalling and set it to sighandler function
    system_handler = signal (SIGINT, sighandler);

    // Read number of CPU
    auto thread_num = std::max(2u, 2*std::thread::hardware_concurrency());

    // Start server
    try
    {
        server_t<tcp_soct_t> server(thread_num);
        server_ptr = &server;
        server.run(port);
    }
    catch(std::runtime_error& err)
    {
        fprintf(stderr, "Hash Server Exception: %s!\n", err.what());
        return -1;
    }
    return 0;
}
