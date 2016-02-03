struct sockaddr_storage;

int server_bind(char *);
void server_listen(int);
void server_loop();
void server_handle(int, int, struct sockaddr_storage *);
