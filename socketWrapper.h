/* general socket wrapper library
   Written for COMP2310 Assignment 2 by Peter Strazdins RSCS ANU 09/14
 */

// handle system call failure; status is a non-zer return code, 
// caller is name of caller, msg uis typically name of the failed system call 
void resourceError(int status, char *caller, char *);

// return an server listening socket for listening, 
// *servPort is the OS-assigned port number
int setupServerSocket(int backlog, int *servPort);

// return a new client socket for server listening on port serverPort
// caller is the name of the caller function
int newClientSock(int serverPort, char *caller);

// return a new server socket from accepting connection from socket serverSock
int newServerAcceptSock(int serverSock, char *fn) ;

