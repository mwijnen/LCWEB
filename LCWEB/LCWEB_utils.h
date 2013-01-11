#ifndef LCWEB_UTILS_H_INCLUDED
#define LCWEB_UTILS_H_INCLUDED

int
LCWEB_make_socket_non_blocking (int sfd);

int
LCWEB_create_and_bind (char *port);

int
LCWEB_initialize_server (char *port);

void
LCWEB_abort (void);

int
LCWEB_send_default_message(int fd);

int
LCWEB_accept_connection (int listen_fd);

int
LCWEB_add_client_to_epoll (int epoll_fd, int client_fd);

#endif // LCWEB_UTILS_H_INCLUDED
