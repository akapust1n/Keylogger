#define LISTENQ 1024 /* 2nd argument to listen() */

#define MAXLINE 4096 /* max text line length */

#define MAXSOCKADDR 128 /* max socket address structure size */

#define BUFFSIZE 8192 /* buffer size for reads and writes */



/* Define some port number that can be used for client-servers */

#define SERV_PORT 9877 /* TCP and UDP client-servers */

#define SERV_PORT_STR "9877" /* TCP and UDP client-servers */

#define UNIXSTR_PATH "/tmp/unix.str" /* Unix domain stream cli-serv */

#define UNIXDG_PATH "/tmp/unix.dg" /* Unix domain datagram cli-serv */

#include <sys/types.h> /* basic system data types */

#include <sys/socket.h> /* basic socket definitions */

#include <sys/time.h> /* timeval{} for select() */

#include <time.h> /* timespec{} for pselect() */

#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */

#include <arpa/inet.h> /* inet(3) functions */

#include <errno.h>

#include <fcntl.h> /* for nonblocking */

#include <netdb.h>

#include <signal.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/stat.h> /* for S_xxx file mode constants */

#include <sys/uio.h> /* for iovec{} and readv/writev */

#include <unistd.h>

#include <sys/wait.h>

#include <sys/un.h> /* for Unix domain sockets */

static char buf[BUFSIZ]; /* Buffer for messages */


int main(int argc, char** argv)

{

    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE];
    ssize_t n;
    fd_set rset, allset;
    char line[MAXLINE];
    socklen_t clilen;

    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //any port

    servaddr.sin_port = htons(SERV_PORT);

    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);  //max number connections

    maxfd = listenfd; /* initialize */

    maxi = -1; /* index into client[] array */
    //printf("%d SIZE\n",FD_SETSIZE);

    for (i = 0; i < FD_SETSIZE; i++)

        client[i] = -1; /* -1 indicates available entry */

    FD_ZERO(&allset); //clear new set

    FD_SET(listenfd, &allset);

    for (;;) {

        rset = allset; /* structure assignment */

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) { /* new client connection */

            clilen = sizeof(cliaddr);

            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen); //create connection

            printf("new client connected with fd #%d\n", connfd);

            for (i = 0; i < FD_SETSIZE; i++)

                if (client[i] < 0) {

                    client[i] = connfd; /* save descriptor */

                    break;
                }

            if (i == FD_SETSIZE) {

                perror("too many clients");

                exit(4);

            } /* end if */

            FD_SET(connfd, &allset); /* add new descriptor to set */

            if (connfd > maxfd)

                maxfd = connfd; /* for select */

            if (i > maxi)

                maxi = i; /* max index in client[] array */

            if (--nready <= 0)

                continue; /* no more readable descriptors */
        }

        for (i = 0; i <= maxi; i++) { /* check all clients for data */

            if ((sockfd = client[i]) < 0)

                continue;

            if (FD_ISSET(sockfd, &rset)) {

                if ((n = read(sockfd, line, MAXLINE)) == 0) {

                    /*connection closed by client */

                    printf("connection by a client with fd #%d\n", sockfd);

                    close(sockfd);

                    FD_CLR(sockfd, &allset);

                    client[i] = -1;

                } else

                    printf("The following line echoed to client with fd #%d\n",

                        sockfd);

                write(sockfd, line, n);

                write(1 /*stdout*/, line, n);

                if (--nready <= 0)

                    break; /* no more readable descriptors */
            }
        }
    }
}
