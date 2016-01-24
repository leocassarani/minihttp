#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "server.h"

// TCP port
#define PORT "3490"

static void
sigchld_handler(int s)
{
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

static void
handle_sigchld(void (*handler)(int))
{
    struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags   = SA_RESTART,
    };

    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    handle_sigchld(sigchld_handler);

    int sockfd = server_bind(PORT);
    server_listen(sockfd);
    server_loop(sockfd);

    return 0;
}
