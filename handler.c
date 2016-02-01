#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "handler.h"
#include "http.h"

static void
handle_not_implemented(struct http_response *resp)
{
    resp->status = "501 Not Implemented";
    resp->headers = http_headers_add(resp->headers, "Content-Type", "text/html");

    char *body = strdup("<html><body>Not Implemented</body></html>\n");
    http_response_set_body(resp, body, strlen(body));
}

static void
handle_not_found(struct http_response *resp)
{
    resp->status = "404 Not Found";
    resp->headers = http_headers_add(resp->headers, "Content-Type", "text/html");

    char *body = strdup("<html><body>Not Found</body></html>\n");
    http_response_set_body(resp, body, strlen(body));
}

static char *
file_path_join(char *relpath)
{
    // NULL, 0 asks getcwd to malloc() the right amount of memory
    char *cwd = getcwd(NULL, 0);
    size_t len = strlen(cwd) + strlen(relpath) + 1; // Add 1 for the NULL byte
    char *fullpath = realloc(cwd, len * sizeof(char));
    return strcat(fullpath, relpath);
}

static void
handle_get(struct http_request *req, struct http_response *resp)
{
    char *fullpath = file_path_join(req->path);
    struct stat filestat;

    // Return 404 Not Found if the file doesn't exist or is a directory.
    if (lstat(fullpath, &filestat) == -1 || !S_ISREG(filestat.st_mode))
    {
        handle_not_found(resp);
        free(fullpath);
        return;
    }

    FILE *file = fopen(fullpath, "r");
    free(fullpath);

    if (NULL == file)
    {
        perror("fopen");
        handle_not_found(resp);
        return;
    }

    size_t len = filestat.st_size;
    char *buf = malloc(len);

    if (fread(buf, len, 1, file) == 0)
        perror("fread");

    if (fclose(file) == EOF)
        perror("fclose");

    resp->status = "200 OK";
    resp->headers = http_headers_add(resp->headers, "Content-Type", "text/plain");
    http_response_set_body(resp, buf, len);
}

void
handle_request(struct http_request *req, struct http_response *resp)
{
    resp->proto = "HTTP/1.1";
    resp->headers = http_headers_add(resp->headers, "Connection", "close");

    if (strcmp(req->method, "GET") == 0)
        handle_get(req, resp);
    else
        handle_not_implemented(resp);
}
