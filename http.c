#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

// TCP port
#define PORT "3490"

// Number of pending connections
#define BACKLOG 10

static void
sigchld_handler(int s)
{
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

static void *
get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *) sa)->sin_addr);

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int
main(int argc, char *argv[])
{
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags    = AI_PASSIVE,
    }, *servinfo, *p;

    int err = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    int sockfd;
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("server: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            return 1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        // Found a socket we can bind on, exit the loop.
        break;
    }

    freeaddrinfo(servinfo);

    if (NULL == p)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 1;
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        return 1;
    }

    struct sigaction sa = {
        .sa_handler = sigchld_handler,
        .sa_flags   = SA_RESTART,
    };

    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    printf("server: waiting for connections...\n");

    struct sockaddr_storage their_addr;
    while (1)
    {
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        char s[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Is this the child process?
        if (!fork())
        {
            // Child doesn't need listener socket.
            close(sockfd);

            char *response = "HTTP/1.1 200 OK\nContent-Length: 40\nConnection: close\n\n<html><body>Hello, world!</body></html>\n";
            if (send(new_fd, response, strlen(response), 0) == -1)
                perror("send");

            close(new_fd);
            return 0;
        }

        // Parent doesn't need connected socket.
        close(new_fd);
    }

    return 0;
}
