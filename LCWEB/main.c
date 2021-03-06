#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include "LCWEB_utils.h"

#define MAXEVENTS 64
#define PORT "2409"

int
main (int argc, char *argv[]) {
    int epoll_fd;
    int listen_fd;
    char *port = PORT;
    struct epoll_event event;
    struct epoll_event *events;

    if (argc == 2) {
        port = argv[1];
    }

    LCWEB_socket_listen_nonblocking (&listen_fd, port);

    LCWEB_epoll_create (&epoll_fd);

    LCWEB_epoll_add_etin (&epoll_fd, &listen_fd);

    events = calloc (MAXEVENTS, sizeof event);

    while (1) {
        int n, i;
        n = epoll_wait (epoll_fd, events, MAXEVENTS, -1);

        for (i = 0; i < n; i++) {

            if ( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || !(events[i].events & EPOLLIN) ) {
                fprintf (stderr, "epoll error\n");
                close (events[i].data.fd);
                continue;
            }

            else if (listen_fd == events[i].data.fd) {
                while (1) {
                    if (LCWEB_socket_accept (epoll_fd, listen_fd) == -1) {
                        break;
                    }
                }
                continue;
            }

            else {
                LCWEB_socket_handle_request (events[i].data.fd);
            }

        }
    }

    free (events);

    close (listen_fd);

    return EXIT_SUCCESS;
}
