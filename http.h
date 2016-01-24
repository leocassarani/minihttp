struct http_request {
    char *method, *path, *version;
};

void http_request_parse(char *, size_t, struct http_request *);
void http_request_free(struct http_request *);
