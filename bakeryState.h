// BakeryState library: 
// provide access to and update function of the state of a Bakery simulation.
// Written for CDS Assignment 2 by Peter Strazdins RSCS ANU 09/14
// Version 1.0 17/09/14

enum CustState { 
  TAKE, // ready to take ticket
  CALL, // has taken ticket but ticket has not yet been called
  PAY,  // ticket has been called 
  BUN,  // has paid and is ready to receive buns
};

#define NONE -1  /*used to signify a value is undefined*/

// returns the next ticket number to be called 
// (it has been taken only if its < nextTktTake())
int nextTktCall(); 

// returns the next ticket number to be taken 
int nextTktTake();
 
// pre: 0 <= cid < getNC()
// return server id, ticket, state currently associated with customer cid
// (returns NONE if no value is associated)
int custServer(int cid);
int custTkt(int cid);
enum CustState custState(int cid);

// pre: 0 <= sid < getNS()
// return customer id, ticket number, amount paid, no. of buns available
// currently associated with server sid (NONE if no value is associated)
int serverTkt(int sid);    // NONE before CALL or after PAY
int serverCustId(int sid); // NONE before PAY or after BUN
int serverPaid(int sid);   // ditto
int serverNBuns(int sid);

// returns number of servers available to call a ticket
int serversAvail(); // = sum of serverCustId(sid)==NONE, 0 <= sid < NS

// initialize bakery state according to BakeryParam library
void bakeryStateInit();

// perform state updates; also perform logging of the respective actions.
// These will abort if bakery state is not immediately ready 
// to perform the corresponding update. 
void take(int cid, int tkt); 
void call(int sid, int tkt); 
void pay(int cid, int sid, int tkt, int nbuns); 
void bun(int cid, int sid, int nbuns); 
void topup(int sid); 
