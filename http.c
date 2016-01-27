#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "http.h"

#define CRLF "\r\n"

// Max length of the "Content-Length" header value
#define MAX_CONTENT_LENGTH_STRLEN 10

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
    while (strncmp(buf, CRLF, 2) != 0)
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

static void
http_response_set_content_length(struct http_response *resp, size_t length)
{
    char value[MAX_CONTENT_LENGTH_STRLEN];
    snprintf(value, MAX_CONTENT_LENGTH_STRLEN, "%zu", length);
    resp->headers = http_headers_add(resp->headers, "Content-Length", value);
    resp->content_length = length;
}

void
http_response_set_body(struct http_response *resp, char *body, size_t length)
{
    resp->body = malloc(length);
    memcpy(resp->body, body, length);
    http_response_set_content_length(resp, length);
}

int
http_response_str(struct http_response *resp, char *buf, size_t length)
{
    // HTTP/1.1 200 OK\r\n
    int i = snprintf(buf, length, "%s %s%s", resp->proto, resp->status, CRLF);

    // Header: Value\r\n
    for (struct http_header *head = resp->headers; head != NULL; head = head->next)
        i += snprintf(buf + i, length - i, "%s: %s%s", head->name, head->value, CRLF);

    // \r\n
    i += snprintf(buf + i, length - i, CRLF);

    // Body
    if (length - i >= resp->content_length)
    {
        memcpy(buf + i, resp->body, resp->content_length);
        i += resp->content_length;
    }

    return i;
}

void
http_response_free(struct http_response *resp)
{
    free(resp->body);
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
