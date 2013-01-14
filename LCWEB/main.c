#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> //MW: sockets
#include <netdb.h> //MW: sockets address structs
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

    ///MW: EVENT LOOP
    while (1) {
        int n, i;
        n = epoll_wait (epoll_fd, events, MAXEVENTS, -1);

        for (i = 0; i < n; i++) {
            ///MW: CHECK EVENT VALIDITY AND ERRORS
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!((events[i].events & EPOLLIN) || (events[i].events & EPOLLOUT)))) {
                // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?) */
                fprintf (stderr, "epoll error\n");
                close (events[i].data.fd);
                continue;
            }

            ///MW: ACCEPT CONNECTIONS
            else if (listen_fd == events[i].data.fd) {
                // We have a notification on the listening socket, which means one or more incoming connections.
                while (1) {
                    if (LCWEB_socket_accept (epoll_fd, listen_fd) == -1) {
                        break;
                    }
                }
                continue;
            }

            ///MW: READ DATA
            else {
                /* We have data on the fd waiting to be read. Read and display it. We must read whatever data is available
                   completely, as we are running in edge-triggered mode and won't get a notification again for the same data. */
                int done = 0;
                done = LCWEB_socket_read (events[i].data.fd);
                if (done) {
                    printf ("Closed connection on descriptor %d\n",
                            events[i].data.fd);

                    /* Closing the descriptor will make epoll remove it
                       from the set of descriptors which are monitored. */
                    close (events[i].data.fd);
                }
            }

        }
    }

    free (events);

    close (listen_fd);

    return EXIT_SUCCESS;
}
