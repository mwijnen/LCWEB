#ifndef LCWEB_UTILS_H_INCLUDED
#define LCWEB_UTILS_H_INCLUDED

int
LCWEB_make_socket_non_blocking (int sfd);

int
LCWEB_create_and_bind (char *port);

void
LCWEB_socket_listen_nonblocking (int *listen_fd, char *port);

void
LCWEB_abort (void);

int
LCWEB_send_default_message(int fd);

int
LCWEB_accept_connection (int epoll_fd, int listen_fd);

int
LCWEB_read_client_data (int client_fd);

void
LCWEB_epoll_create (int *epoll_fd);

void
LCWEB_epoll_add_etin (int *epoll_fd, int *client_fd);

#endif // LCWEB_UTILS_H_INCLUDED
