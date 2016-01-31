#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

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

void
server_loop(int sockfd)
{
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

        if (!fork())
        {
            // Child doesn't need listener socket.
            close(sockfd);
            server_handle(conn_fd, &their_addr);
            exit(0);
        }

        // Parent doesn't need the connection socket.
        close(conn_fd);
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
server_handle(int fd, struct sockaddr_storage *addr)
{
    char ipaddr[INET6_ADDRSTRLEN];
    inet_ntop(addr->ss_family,
            get_in_addr((struct sockaddr *) addr), ipaddr, sizeof ipaddr);

    char in[BUFLEN];
    struct http_request req;

    int inlen = recv(fd, in, BUFLEN - 1, 0);
    if (inlen == -1)
    {
        perror("recv");
        close(fd);
        return;
    }

    http_request_parse(in, inlen, &req);
    printf("handler: %s - %s %s\n", ipaddr, req.method, req.path);

    struct http_response resp = {
        .proto  = "HTTP/1.1",
        .status = "200 OK",
    };

    resp.headers = http_headers_add(resp.headers, "Connection", "close");
    resp.headers = http_headers_add(resp.headers, "Content-Type", "text/html");

    char *body = "<html><body>Hello, world!</body></html>\n";
    http_response_set_body(&resp, body, strlen(body));

    char out[BUFLEN];

    int outlen = http_response_status_line(&resp, out, BUFLEN);
    outlen += http_response_headers(&resp, out + outlen, BUFLEN - outlen);

    if (send(fd, out, outlen, 0) == -1 ||
        send(fd, resp.body, resp.content_length, 0) == -1)
        perror("send");

    http_response_free(&resp);
    http_request_free(&req);

    close(fd);
}
