#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "handler.h"
#include "http.h"
#include "server.h"

// Number of pending connections
#define BACKLOG 10

// Size of the recv buffer for each handler
#define BUFLEN 1024

int
server_bind(char *service)
{
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags    = AI_PASSIVE,
    }, *servinfo, *p;

    int err = getaddrinfo(NULL, service, &hints, &servinfo);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
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
            exit(1);
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
        exit(1);
    }

    return sockfd;
}

void
server_listen(int sockfd)
{
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");
}

struct worker_args {
    int count;
    int fd;
    struct sockaddr_storage *addr;
};

static struct worker_args *
worker_args_new(int count, int fd, struct sockaddr_storage *addr)
{
    struct worker_args *args = malloc(sizeof(struct worker_args));
    args->count = count;
    args->fd = fd;
    args->addr = addr;
    return args;
}

static void *
worker_start(void *args_)
{
    struct worker_args *args = (struct worker_args *) args_;
    server_handle(args->count, args->fd, args->addr);
    free(args_);
    return NULL;
}

#define handle_error_en(en, msg) \
    do { errno = en, perror(msg); return; } while(0)

static void
worker_spawn(int count, int fd, struct sockaddr_storage *addr)
{
    pthread_t thread;
    pthread_attr_t attr;
    int retval;

    retval = pthread_attr_init(&attr);
    if (retval != 0)
        handle_error_en(retval, "pthread_attr_init");

    retval = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (retval != 0)
        handle_error_en(retval, "pthread_attr_setdetachstate");

    // The thread will be responsible for calling free() on the memory for args.
    struct worker_args *args = worker_args_new(count, fd, addr);

    retval = pthread_create(&thread, &attr, worker_start, args);
    if (retval != 0)
    {
        free(args);
        handle_error_en(retval, "pthread_create");
    }

    retval = pthread_attr_destroy(&attr);
    if (retval != 0)
        handle_error_en(retval, "pthread_attr_destroy");
}

void
server_loop(int sockfd)
{
    int thread_count = 0;
    struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof their_addr;

    while (1)
    {
        int conn_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (conn_fd == -1)
        {
            perror("accept");
            continue;
        }

        worker_spawn(++thread_count, conn_fd, &their_addr);
    }
}

static void *
get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *) sa)->sin_addr);

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void
server_handle(int count, int fd, struct sockaddr_storage *addr)
{
    char ipaddr[INET6_ADDRSTRLEN];
    inet_ntop(addr->ss_family,
            get_in_addr((struct sockaddr *) addr), ipaddr, sizeof ipaddr);

    char in[BUFLEN];
    int inlen = recv(fd, in, BUFLEN, 0);
    if (inlen == -1)
    {
        perror("recv");
        close(fd);
        return;
    }

    struct http_request req;
    http_request_parse(in, inlen, &req);

    printf("handler[%d]: %s - %s %s\n", count, ipaddr, req.method, req.path);

    struct http_response resp;
    memset(&resp, 0, sizeof(struct http_response));

    handle_request(&req, &resp);

    char out[BUFLEN];
    int outlen = http_response_status_line(&resp, out, BUFLEN);
    outlen += http_response_headers(&resp, out + outlen, BUFLEN - outlen);

    if (send(fd, out, outlen, 0) == -1 ||
        send(fd, resp.body, resp.content_length, 0) == -1)
        perror("send");

    free(resp.body);
    http_response_free(&resp);
    http_request_free(&req);

    close(fd);
}
