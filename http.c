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

    return buf + 2; // Add 2 for the final "\r\n"
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

void
http_response_set_body(struct http_response *resp, char *body, size_t length)
{
    resp->body = body;
    resp->length = length;

    char str[10];
    snprintf(str, 10, "%zu", length);
    resp->headers = http_headers_add(resp->headers, "Content-Length", str);
}

void
http_response_str(struct http_response *resp, char *buf, size_t length)
{
    strcat(buf, resp->proto);
    strcat(buf, " ");
    strcat(buf, resp->status);
    strcat(buf, CRLF);

    for (struct http_header *head = resp->headers; head != NULL; head = head->next)
    {
        strcat(buf, head->name);
        strcat(buf, ": ");
        strcat(buf, head->value);
        strcat(buf, CRLF);
    }

    strcat(buf, CRLF);
    strcat(buf, resp->body);
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
