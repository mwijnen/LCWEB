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
    int s;
    int epoll_fd;
    int listen_fd;
    char *port = PORT;
    struct epoll_event event;
    struct epoll_event *events;

    if (argc == 2) {
        port = argv[1];
    }

    listen_fd = LCWEB_initialize_server (port);

    epoll_fd = epoll_create1 (0);
    if (epoll_fd == -1) {
        perror ("epoll_create");
        abort ();
    }

    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    if (s == -1) {
        perror ("epoll_ctl");
        abort ();
    }

    events = calloc (MAXEVENTS, sizeof event);
    int fd_array[3] = {-1, -1, -1};
    int index = 0;
    while (1) {
        int n, i;
        n = epoll_wait (epoll_fd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if (index == 2) {
                LCWEB_send_default_message (fd_array[2]);
            }
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!((events[i].events & EPOLLIN) || (events[i].events & EPOLLOUT)))) {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */
                fprintf (stderr, "epoll error\n");
                close (events[i].data.fd);
                continue;
            }

            else if (listen_fd == events[i].data.fd) {
                /* We have a notification on the listening socket, which
                   means one or more incoming connections. */
                while (1) {
                    //struct sockaddr in_addr;
                    //socklen_t in_len;
                    int client_id;
                    client_id = LCWEB_accept_connection (listen_fd);
                    if (client_id == -1){
                        break;
                    }

                    event.data.fd = client_id;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, client_id, &event);
                    if (s == -1) {
                        perror ("epoll_ctl");
                        abort ();
                    }
                }
                continue;
            } else {
                /* We have data on the fd waiting to be read. Read and
                   display it. We must read whatever data is available
                   completely, as we are running in edge-triggered mode
                   and won't get a notification again for the same
                   data. */
                int done = 0;

                fd_array[index] = events[i].data.fd;
                index++;

                while (1) {
                    ssize_t count;
                    char buf[512];

                    count = read (events[i].data.fd, buf, sizeof buf);
                    if (count == -1) {
                        /* If errno == EAGAIN, that means we have read all
                           data. So go back to the main loop. */
                        if (errno != EAGAIN) {
                            perror ("read");
                            done = 1;
                        }
                        break;
                    } else if (count == 0) {
                        /* End of file. The remote has closed the
                           connection. */
                        done = 1;
                        break;
                    }

                    /* Write the buffer to standard output */
                    s = write (1, buf, count);

                    if (s == -1) {
                        perror ("write");
                        abort ();
                    }
                }

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
