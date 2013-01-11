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

int
LCWEB_make_socket_non_blocking (int sfd) {
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1) {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1) {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

int
LCWEB_create_and_bind (char *port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, listen_fd;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo (NULL, port, &hints, &result);
    if (s != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1)
            continue;

        s = bind (listen_fd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {
            /* We managed to bind successfully! */
            break;
        }

        close (listen_fd);
    }

    if (rp == NULL) {
        fprintf (stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo (result);

    return listen_fd;
}

void
LCWEB_socket_listen_nonblocking (int *listen_fd, char *port) {
    int s;

    *listen_fd = LCWEB_create_and_bind (port);
    if (listen_fd == -1) {
        LCWEB_abort ();
    }

    s = LCWEB_make_socket_non_blocking (*listen_fd);
    if (s == -1) {
        LCWEB_abort ();
    }

    s = listen (*listen_fd, SOMAXCONN); //MW: SOMAXCONN defines the listening queue size
    if (s == -1) {
        perror ("listen");
        LCWEB_abort ();
    }

    printf ("LCWEB initialized");
}

void
LCWEB_abort (void) {
    abort ();
}

int
LCWEB_accept_connection (int epoll_fd, int listen_fd) {
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
    s = LCWEB_make_socket_non_blocking (client_fd);
    if (s == -1)
        LCWEB_abort ();

    LCWEB_epoll_add_etin (&epoll_fd, &client_fd);
    return client_fd;
}

int
LCWEB_read_client_data (int client_fd) {

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
    char *message="This is a message to send\n\r";
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




