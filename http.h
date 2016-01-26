struct http_request {
    char *method;
    char *path;
    char *proto;
    struct http_header *headers;
};

struct http_response {
    char *proto;
    char *status;
    char *body;
    size_t length;
    struct http_header *headers;
};

struct http_header {
    char *name;
    char *value;
    struct http_header *next; // Linked list
};

void http_request_parse(char *, size_t, struct http_request *);
void http_request_free(struct http_request *);

void http_response_set_body(struct http_response *, char *, size_t);
void http_response_str(struct http_response *, char *, size_t);

struct http_header *http_headers_add(struct http_header *, char *, char *);
void http_headers_free(struct http_header *);
