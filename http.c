#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"

char *
copy_until(char *buf, char until, char **out)
{
    size_t i = 0;
    for (char c = buf[i]; c != until; c = buf[++i]);
    *out = strndup(buf, i++); // Increment i so as to go beyond 'until'
    return buf + i;
}

void
http_request_parse(char *buf, size_t size, struct http_request *req)
{
    buf = copy_until(buf, ' ', &req->method);
    buf = copy_until(buf, ' ', &req->path);
    copy_until(buf, '\n', &req->version);
}

void
http_request_free(struct http_request *req)
{
    free(req->method);
    free(req->path);
    free(req->version);
}
