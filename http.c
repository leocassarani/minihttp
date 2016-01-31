#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "http.h"

#define CRLF "\r\n"

static char *
copy_until(char *buf, char *until, char **out)
{
    char *start = buf;
    size_t len = strlen(until);

    while(strncmp(buf, until, len) != 0)
        buf++;

    *out = strndup(start, buf - start);
    return buf + len;
}

static char *
parse_headers(char *buf, struct http_header **out)
{
    struct http_header *prev = NULL;

    // Keep parsing headers until we see a line containing only "\r\n".
    while (strncmp(buf, CRLF, strlen(CRLF)) != 0)
    {
        struct http_header *header = malloc(sizeof(struct http_header));
        header->next = NULL;

        buf = copy_until(buf, ": ", &header->name);
        buf = copy_until(buf, CRLF, &header->value);

        if (NULL == prev)
            *out = prev = header;
        else
        {
            prev->next = header;
            prev = header;
        }
    }

    return buf + strlen(CRLF);
}

void
http_request_parse(char *buf, size_t size, struct http_request *req)
{
    buf = copy_until(buf, " ", &req->method);
    buf = copy_until(buf, " ", &req->path);
    buf = copy_until(buf, CRLF, &req->proto);
    parse_headers(buf, &req->headers);
}

void
http_request_free(struct http_request *req)
{
    free(req->method);
    free(req->path);
    free(req->proto);
    http_headers_free(req->headers);
}

static char *
sizetoa(size_t size)
{
    // Find out how many bytes it will take to convert size into a string.
    // Add 1 for the terminating NULL byte.
    int len = 1 + snprintf(NULL, 0, "%zu", size);

    // Allocate just enough storage, then call snprintf for real.
    char *str = malloc(sizeof(char) * len);
    snprintf(str, len, "%zu", size);

    return str;
}

void
http_response_set_body(struct http_response *resp, char *body, size_t length)
{
    resp->body = body;
    resp->content_length = length;

    // Convert length to a string, then set it as the Content-Length header value.
    char *str = sizetoa(length);
    resp->headers = http_headers_add(resp->headers, "Content-Length", str);
    free(str);
}

int
http_response_status_line(struct http_response *resp, char *buf, size_t length)
{
    // HTTP/1.1 200 OK\r\n
    return snprintf(buf, length, "%s %s%s", resp->proto, resp->status, CRLF);
}

int
http_response_headers(struct http_response *resp, char *buf, size_t length)
{
    int i = 0;

    // Header: Value\r\n
    for (struct http_header *head = resp->headers; head != NULL; head = head->next)
        i += snprintf(buf + i, length - i, "%s: %s%s", head->name, head->value, CRLF);

    // \r\n
    i += snprintf(buf + i, length - i, CRLF);

    return i;
}

void
http_response_free(struct http_response *resp)
{
    http_headers_free(resp->headers);
}

struct http_header *
http_headers_add(struct http_header *headers, char *name, char *value)
{
    struct http_header *header = malloc(sizeof(struct http_header));
    header->name  = strdup(name);
    header->value = strdup(value);
    header->next  = headers;
    return header;
}

void
http_headers_free(struct http_header *header)
{
    struct http_header *next;
    while (header != NULL)
    {
        next = header->next;
        free(header->name);
        free(header->value);
        free(header);
        header = next;
    }
}
