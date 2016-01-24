struct http_request {
    char *method;
    char *path;
    char *version;
    struct http_header *headers;
};

struct http_header {
    char *name;
    char *value;
    struct http_header *next; // Linked list
};

void http_request_parse(char *, size_t, struct http_request *);
void http_request_free(struct http_request *);
