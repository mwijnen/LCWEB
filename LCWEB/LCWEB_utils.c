#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> //MW: error messages
#include <fcntl.h> //MW: nonblocking sockets
#include <sys/socket.h> //MW: sockets
#include <netdb.h> //MW: sockets address structs
#include <unistd.h> //MW: geta ddress info
#include <sys/epoll.h>

#include "LCWEB_utils.h"

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

void
LCWEB_abort (void) {
    abort ();
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
                (errno == EWOULDBLOCK)) {
            // We have processed all incoming connections.
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
    if (s == 0) {
        printf("Accepted connection on descriptor %d "
               "(host=%s, port=%s)\n", client_fd, hbuf, sbuf);
    }

    /* Make the incoming socket non-blocking and add it to the
       list of fds to monitor. */
    LCWEB_socket_set_non_blocking (client_fd);

    LCWEB_epoll_add_etin (&epoll_fd, &client_fd);
    return client_fd;
}

int
LCWEB_socket_read (int socket_fd) {
    int done;
    while (1) {
        int s;
        ssize_t count;
        char buf[512];
        done = 0;
        count = read (socket_fd, buf, sizeof buf);
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
            LCWEB_abort ();
        }
    }
    //MW:
    //if (done == 0) {
        printf ("\nwrite to: %i\n\n\n", socket_fd );
        if (done == 0) {
            LCWEB_send_default_message(socket_fd);
            done = 1;
        }
        //    *newest_fd = *newest_fd + 1;
        //    fd_buffer[*newest_fd];
        //    printf ("\nAdded file descriptor: %i\n", socket_fd);
        //    printf ("At position: %i\n", *newest_fd);
    //}
    return done;
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

int
LCWEB_send_default_message(int fd) {
    printf("attempt to send data\n");
    int ret;
    char *message=
    "HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\n\n\nHello World!";
    //"Hello World!";

    ret = send(fd,message,strlen(message),0);
    /*MW: add a test for EAGAIN and add to burrer in case send has failed */
    if (ret == -1) {
        if (errno == EWOULDBLOCK) {
            printf ("EWOULDBLOCK");
        }
        if (errno == EAGAIN) {
            printf ("EAGAIN");
        }
        printf ("\nERROR NUMBER: %i\n", errno);
    }
    if (ret == 0) {
        printf ("error code: %i\n", ret);
        printf ("returned value %i", ret);
        abort ();
    } else if (ret < 0) {
        printf ("error code: %i\n", ret);
        printf("send() failed");
        abort ();
    }
    printf("Send %d bytes\n", ret);
    return ret;
}




