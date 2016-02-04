#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "server.h"

// TCP port
#define PORT "3490"

static void
ignore_sigpipe()
{
    struct sigaction sa = {
        .sa_handler = SIG_IGN,
    };

    if (sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    ignore_sigpipe();

    int sockfd = server_bind(PORT);
    server_listen(sockfd);
    server_loop(sockfd);

    return 0;
}
