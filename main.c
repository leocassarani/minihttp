#include "server.h"

// TCP port
#define PORT "3490"

int
main(int argc, char *argv[])
{
    int sockfd = server_bind(PORT);
    server_listen(sockfd);
    server_loop(sockfd);
    return 0;
}
