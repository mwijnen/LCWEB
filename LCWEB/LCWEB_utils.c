#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "LCWEB_utils.h"
#include "LCWEB_http.h"

void
LCWEB_socket_set_non_blocking (int socket_fd) {
    int flags, s;

    flags = fcntl (socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror ("fcntl");
        LCWEB_abort();
    }

    flags |= O_NONBLOCK;
    s = fcntl (socket_fd, F_SETFL, flags);
    if (s == -1) {
        perror ("fcntl");
        LCWEB_abort();
    }
}

void
LCWEB_socket_create_and_bind (int *socket_fd, char *port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo (NULL, port, &hints, &result);
    if (s != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        LCWEB_abort();
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        *socket_fd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (*socket_fd == -1)
            continue;

        s = bind (*socket_fd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {
            /* We managed to bind successfully! */
            break;
        }
        close (*socket_fd);
    }

    if (rp == NULL) {
        fprintf (stderr, "Could not bind\n");
        LCWEB_abort();
    }
    freeaddrinfo (result);
}

void
LCWEB_socket_listen_nonblocking (int *socket_fd, char *port) {
    int s;

    LCWEB_socket_create_and_bind (socket_fd, port);

    LCWEB_socket_set_non_blocking (*socket_fd);

    s = listen (*socket_fd, SOMAXCONN); //MW: SOMAXCONN defines the listening queue size
    if (s == -1) {
        perror ("listen");
        LCWEB_abort ();
    }

    printf ("LCWEB initialized\n");
}

int
LCWEB_socket_accept (int epoll_fd, int listen_fd) {
    struct sockaddr in_addr;
    socklen_t in_len;
    int client_fd, s;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    in_len = sizeof in_addr;
    client_fd = accept (listen_fd, &in_addr, &in_len);
    if (client_fd == -1) {
        if ((errno == EAGAIN) ||
                (errno == EWOULDBLOCK)) { /// We have processed all incoming connections.
            return -1;
        } else {
            perror ("accept");
            return -1;
        }
    }
    s = getnameinfo (&in_addr, in_len,
                     hbuf, sizeof hbuf,
                     sbuf, sizeof sbuf,
                     NI_NUMERICHOST | NI_NUMERICSERV);

    LCWEB_socket_set_non_blocking (client_fd);
    LCWEB_epoll_add_etin (&epoll_fd, &client_fd);
    return client_fd;
}

int
LCWEB_socket_handle_request (int socket_fd) {
    int done;
    while (1) {
        int s;
        ssize_t count;
        char buf[512];
        done = 0;
        count = read (socket_fd, buf, sizeof buf);
        if (count == -1) { ///If errno == EAGAIN, that means we have read all data. So go back to the main loop.
            if (errno != EAGAIN) {
                perror ("read");
                done = 1;
            }
            break;
        } else if (count == 0) { /// End of file. The remote has closed the connection.
            done = 1;
            break;
        }

        //s = write (1, buf, count);
        //printf ("\n\n\n");
        //if (s == -1) {
        //    perror ("write");
        //    LCWEB_abort ();
        //}
    }

    /// done signifies whether the connection is still alive
    /// since we don't keep the connection alive the connection is closed in all cases.
    LCWEB_socket_send_default_message(socket_fd);
    close (socket_fd);
    return done;
}

int
LCWEB_socket_send_default_message (int fd) {
    int ret;
    char *message = LCWEB_http_html_login ();

    ret = send(fd,message,strlen(message),0);
    if (ret == 0) {
        printf ("error code: %i\n", ret);
        printf ("returned value %i", ret);
        abort ();
    } else if (ret < 0) {
        printf ("error code: %i\n", ret);
        printf("send() failed");
        abort ();
    }
    return ret;
}

void
LCWEB_abort (void) {
    abort ();
}

void
LCWEB_epoll_create (int *epoll_fd) {
    *epoll_fd = epoll_create1 (0);
    if (*epoll_fd == -1) {
        perror ("epoll_create");
        abort ();
    }
}

void
LCWEB_epoll_add_etin (int *epoll_fd, int *client_fd) {
    int s;
    struct epoll_event event;
    event.data.fd = *client_fd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (*epoll_fd, EPOLL_CTL_ADD, *client_fd, &event);
    if (s == -1) {
        perror ("epoll_ctl");
        LCWEB_abort ();
    }
}






