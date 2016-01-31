#include <string.h>

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
handle_get(struct http_request *req, struct http_response *resp)
{
    resp->status = "200 OK";
    resp->headers = http_headers_add(resp->headers, "Content-Type", "text/html");

    char *body = strdup("<html><body>Hello, world!</body></html>\n");
    http_response_set_body(resp, body, strlen(body));
}

void
handle_request(struct http_request *req, struct http_response *resp)
{
    resp->proto   = "HTTP/1.1";
    resp->headers = http_headers_add(resp->headers, "Connection", "close");

    if (strcmp(req->method, "GET") == 0)
        handle_get(req, resp);
    else
        handle_not_implemented(resp);
}
