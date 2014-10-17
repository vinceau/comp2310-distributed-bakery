/* distBakery.c: distributed bakery simulation program

Name: Vincent Au
StudentId: u5388374

 ***Disclaimer***: (modify as appropriate)
 The work that I am submitting for this program is without significant
 contributions from others (excepting course staff).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>     /* fork(), pipe(), read(), write() */
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "bakeryParam.h"
#include "bakeryState.h"
#include "socketWrapper.h"

#define MAXBUF 32
#define COUNT_MAX 20

struct cust {
    int tix;
    int sid;
};

int main(int argc, char *argv[]) {
    bakeryParamInit(argc, argv);
    bakeryStateInit();

    int fd_cp[getNC()][2];
    int maxfd;
    fd_set fd_read_set;
    char buffer[MAXBUF];
    int i, counter, status;
    int sock_1, sock_2[getNC()];
    struct sockaddr_in client[getNC()], server;

    socklen_t namelen;

    /* ----Create TCP/IP socket---- */
    sock_1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_1 == -1) 
        resourceError(sock_1, "server", "socket");
    printf("Socket created successfully.\n");

    /* ----Address information for use with bind---- */
    server.sin_family = AF_INET;         /* it is an IP address */
    server.sin_port   = 0;               /* use O/S defined port number */
    server.sin_addr.s_addr = INADDR_ANY; /* use any interface on this host*/

    /* ----Bind socket to address and port---- */
    status = bind(sock_1, (struct sockaddr *) &server, sizeof(server)); 
    if (status != 0)
        resourceError(status, "server", "bind");

    /* ----Find out what port number was assigned---- */
    namelen = sizeof(server);
    status = getsockname(sock_1, (struct sockaddr *) &server, &namelen);
    if (status != 0)
        resourceError(status, "server", "getsockname - port number");
    printf("The assigned server port number is %d\n", ntohs(server.sin_port));

    status = listen(sock_1, getNC()); //create a queue of getNC() size
    if (status != 0)
        resourceError(status, "server", "listen");


    /* ---- Create the children ----- */
    for (i=0; i < getNC(); i++) {
        /* ----Create child i ---- */
        int pid = fork();
        if (pid < 0)
            resourceError(status, "selectsock", "fork");
        if (pid == 0) { /* ---- perform child i  ---- */
            int count;
            close(sock_1); /*Close the parent's listening socket */
            sock_1 = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_1 < 0)
                resourceError(sock_1, "selectsock child", "socket");
            printf("Client %d socket created\n", i);

            /* ---- Attempt to connect to the server; n.b. server struct set by 
             * parent ---- */
            status = connect(sock_1, (struct sockaddr *) &server, sizeof(server));
            if (status != 0)
                resourceError(status, "selectsock child", "connect");
            
            int msg;
            int nbytes;
            
            do {
                int buns = rand() % getNB() + 1;

                //take a ticket
                nbytes = write(sock_1, &buns, sizeof(buns));
                nbytes = read(sock_1, &msg, sizeof(msg));

                //sleep a bit
                sleepEvents();

                //wait to be called
                do {
                    nbytes = write(sock_1, &buns, sizeof(buns));
                    nbytes = read(sock_1, &msg, sizeof(msg));
                } while (msg == -1);

                //sleep a bit
                sleepEvents();

                //order some buns
                nbytes = write(sock_1, &buns, sizeof(buns));
                nbytes = read(sock_1, &msg, sizeof(msg));

                //sleep a bit
                sleepEvents();

                //receive the buns
                nbytes = write(sock_1, &buns, sizeof(buns));
                nbytes = read(sock_1, &msg, sizeof(msg));
                
                //sleep a bit
                sleepEvents();
            } while (nbytes > 0);
            printf("Child %d terminates\n", i);
            fflush(stdout);
            close(sock_1); close(sock_1);
            exit(0);
        }
    }


    /* ----Now for the parent---- */
    for (i=0; i < getNC(); i++) {
        namelen = sizeof(client[i]);
        sock_2[i] = accept(sock_1, (struct sockaddr *) &client[i], &namelen);
        if (sock_2[i] < 0)
            resourceError(status, "parent", "accept");
        printf("Server received connection from child %d %s\n", i, 
                inet_ntoa(client[i].sin_addr));
    }

    /* ----What is the maximum file descriptor for select---- */
    maxfd = fd_cp[0][0];
    for (i=1; i < getNC(); i++)
        if (sock_2[i] > maxfd)
            maxfd = sock_2[i];
    maxfd = maxfd + 1;

    counter = 0;

    // create the servers
    int servers[getNS()];
    // initialise them to 1 to signify ready
    for (i = 0; i < getNS(); i++) {
        servers[i] = 1;
    }
    // create the customers
    struct cust c[getNC()];
    c[i].tix = -1;
    c[i].sid = -1;

    int msg = 1;

    /* ----Clear the read set---- */
    FD_ZERO(&fd_read_set);
    while (1) {
        /* ----Set file descriptor set to getNC() read descriptors---- */
        for (i=0; i < getNC(); i++) {
            FD_SET(sock_2[i], &fd_read_set);
        }

        /* ----Wait in select until file descriptors change---- */
        select(maxfd, &fd_read_set, NULL, NULL, NULL);
        // check client connected (which bit was set from select)
        for (i=0; i < getNC(); i++) {
            /* ----Was it child i---- */
            if (FD_ISSET(sock_2[i], &fd_read_set)) {
                int buns;
                int result = -1;
                read(sock_2[i], &buns, sizeof(buns));

                switch (custState(i)) {
                    case TAKE:
                        c[i].tix = nextTktTake();
                        result = c[i].tix;
                        take(i, c[i].tix);
                        break;
                    case CALL:
                        if (serversAvail() > 0) {
                            for (int s = 0; s < getNS(); s++) {
                                if (servers[s]) {
                                    call(s, c[i].tix);
                                    c[i].sid = s;
                                    result = s;
                                    servers[s] = 0; //server is busy
                                    break;
                                }
                            }
                        }
                        break;
                    case PAY:
                        pay(i, c[i].sid, c[i].tix, buns);
                        result = serverPaid(c[i].sid);
                        break;
                    case BUN:
                        if (serverNBuns(c[i].sid) < buns) {
                            topup(c[i].sid);
                        }
                        bun(i, c[i].sid, buns);
                        result = buns;
                        servers[c[i].sid] = 1;
                        c[i].tix = -1;
                        c[i].sid = -1;
                        break;
                }
                write(sock_2[i], &result, sizeof(result));
            }
        }
    }

    /* ---- Request the children to terminate and wait ---- */
    counter = -1;
    for (i=0; i < getNC(); i++) {
        write(sock_2[i], &counter, sizeof(counter));
        close(sock_2[i]); close(sock_2[i]);  
    }
    for (i=0; i < getNC(); i++)
        wait(NULL);
    exit(0);
} //main()
