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
#include <sys/wait.h>

#include "bakeryParam.h"
#include "bakeryState.h"
#include "socketWrapper.h"

struct cust {
    int nbuns;
    int tix;
    int sid;
};

int main(int argc, char *argv[]) {
    bakeryParamInit(argc, argv);
    bakeryStateInit();

    int fd_cp[getNC()][2];
    int maxfd;
    fd_set fd_read_set;
    int status;
    int sock_1, sock_2[getNC()];
    struct sockaddr_in client[getNC()], server;

    socklen_t namelen;

    /* ----Create TCP/IP socket---- */
    sock_1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_1 == -1) {
        resourceError(sock_1, "server", "socket");
    }
    printf("Socket created successfully.\n");

    /* ----Address information for use with bind---- */
    server.sin_family = AF_INET;         /* it is an IP address */
    server.sin_port   = 0;               /* use O/S defined port number */
    server.sin_addr.s_addr = INADDR_ANY; /* use any interface on this host*/

    /* ----Bind socket to address and port---- */
    status = bind(sock_1, (struct sockaddr *) &server, sizeof(server)); 
    if (status != 0) {
        resourceError(status, "server", "bind");
    }

    /* ----Find out what port number was assigned---- */
    namelen = sizeof(server);
    status = getsockname(sock_1, (struct sockaddr *) &server, &namelen);
    if (status != 0) {
        resourceError(status, "server", "getsockname - port number");
    }
    printf("The assigned server port number is %d\n", ntohs(server.sin_port));

    status = listen(sock_1, getNC()); //create a queue of getNC() size
    if (status != 0) {
        resourceError(status, "server", "listen");
    }

    /* ---- Create the children ----- */
    for (int i=0; i < getNC(); i++) {
        /* ----Create child i ---- */
        switch(fork()) {
            case -1:
                resourceError(status, "selectsock", "fork");
                break;
            case 0: /* ---- perform child i  ---- */
                close(sock_1); /*Close the parent's listening socket */
                sock_1 = socket(AF_INET, SOCK_STREAM, 0);
                if (sock_1 < 0) {
                    resourceError(sock_1, "selectsock child", "socket");
                }
                printf("Client %d socket created\n", i);

                /* ---- Attempt to connect to the server; 
                 * n.b. server struct set by parent ---- */
                status = connect(sock_1, (struct sockaddr *) &server,
                        sizeof(server));
                if (status != 0) {
                    resourceError(status, "selectsock child", "connect");
                }
                
                int msg = 0;
                int nbytes;
                
                do {
                    //take a ticket
                    write(sock_1, &msg, sizeof(msg));
                    read(sock_1, &msg, sizeof(msg));

                    //sleep a bit
                    sleepEvents();

                    //wait to be called
                    do {
                        write(sock_1, &msg, sizeof(msg));
                        read(sock_1, &msg, sizeof(msg));
                    } while (msg == -1);

                    //sleep a bit
                    sleepEvents();

                    //order some buns
                    write(sock_1, &msg, sizeof(msg));
                    read(sock_1, &msg, sizeof(msg));

                    //sleep a bit
                    sleepEvents();

                    //receive the buns
                    write(sock_1, &msg, sizeof(msg));
                    nbytes = read(sock_1, &msg, sizeof(msg));
                    
                    //sleep a bit
                    sleepEvents();
                } while (nbytes > 0);
                close(sock_1); close(sock_1);
                exit(0);
                break;
            default:
                //do nothing
                break;
        }
    }


    /* ----Now for the parent---- */
    for (int i=0; i < getNC(); i++) {
        namelen = sizeof(client[i]);
        sock_2[i] = accept(sock_1, (struct sockaddr *) &client[i], &namelen);
        if (sock_2[i] < 0) {
            resourceError(status, "parent", "accept");
        }
        printf("Server received connection from child %d %s\n", i, 
                inet_ntoa(client[i].sin_addr));
    }

    /* ----What is the maximum file descriptor for select---- */
    maxfd = fd_cp[0][0];
    for (int i=1; i < getNC(); i++)
        if (sock_2[i] > maxfd) {
            maxfd = sock_2[i];
        }
    maxfd = maxfd + 1;

    // create the servers
    int servers[getNS()];
    // initialise them to 1 to signify ready
    for (int i = 0; i < getNS(); i++) {
        servers[i] = 1;
    }
    // initialise the customers
    struct cust c[getNC()];
    for (int i = 0; i < getNC(); i++) {
        c[i].nbuns = (rand() % getNB()) + 1;
        c[i].tix = -1;
        c[i].sid = -1;
    }

    /* ----Clear the read set---- */
    FD_ZERO(&fd_read_set);
    while (1) {
        /* ----Set file descriptor set to getNC() read descriptors---- */
        for (int i=0; i < getNC(); i++) {
            FD_SET(sock_2[i], &fd_read_set);
        }

        /* ----Wait in select until file descriptors change---- */
        select(maxfd, &fd_read_set, NULL, NULL, NULL);
        // check client connected (which bit was set from select)
        for (int i=0; i < getNC(); i++) {
            /* ----Was it child i---- */
            if (FD_ISSET(sock_2[i], &fd_read_set)) {
                int msg = -1;
                read(sock_2[i], &msg, sizeof(msg));

                switch (custState(i)) {
                    case TAKE:
                        c[i].tix = nextTktTake();
                        msg = c[i].tix;
                        take(i, c[i].tix);
                        break;
                    case CALL:
                        if (serversAvail() > 0) {
                            for (int s = 0; s < getNS(); s++) {
                                if (servers[s] && c[i].tix == nextTktCall()) {
                                    call(s, c[i].tix);
                                    c[i].sid = s;
                                    msg = s;
                                    servers[s] = 0; //server is busy
                                    break;
                                }
                            }
                        } else {
                            msg = -1;
                        }
                        break;
                    case PAY:
                        pay(i, c[i].sid, c[i].tix, c[i].nbuns);
                        msg = serverPaid(c[i].sid);
                        break;
                    case BUN:
                        if (serverNBuns(c[i].sid) < c[i].nbuns) {
                            topup(c[i].sid);
                        }
                        bun(i, c[i].sid, c[i].nbuns);
                        msg = c[i].nbuns;
                        
                        //reset values
                        servers[c[i].sid] = 1; //server is ready again
                        c[i].nbuns = rand() % getNB() + 1; //new no. of buns
                        c[i].tix = -1;
                        c[i].sid = -1;
                        break;
                }
                write(sock_2[i], &msg, sizeof(msg));
            }
        }
    }

    /* ---- Request the children to terminate and wait ---- */
    for (int i=0; i < getNC(); i++) {
        close(sock_2[i]); close(sock_2[i]);  
    }
    for (int i=0; i < getNC(); i++) {
        wait(NULL);
    }
    exit(0);
} //main()
