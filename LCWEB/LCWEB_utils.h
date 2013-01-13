#ifndef LCWEB_UTILS_H_INCLUDED
#define LCWEB_UTILS_H_INCLUDED

void
LCWEB_socket_set_non_blocking (int socket_fd);

void
LCWEB_socket_create_and_bind (int *socket_fd, char *port);

void
LCWEB_socket_listen_nonblocking (int *socket_fd, char *port);

int
LCWEB_socket_read (int client_fd);

int
LCWEB_socket_accept (int epoll_fd, int listen_fd);

void
LCWEB_epoll_create (int *epoll_fd);

void
LCWEB_epoll_add_etin (int *epoll_fd, int *client_fd);

void
LCWEB_abort (void);

int
LCWEB_send_default_message(int fd);

#endif // LCWEB_UTILS_H_INCLUDED
