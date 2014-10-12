/* Bakery Parameter library
   This class provides parameters of the Bakery simulation from the 
   command line and environment variables.
   Written or COMP2310 Assignment 1 by Peter Strazdins RSCS ANU 08/14
 */

extern const int MaxEventDef; // max number of events 
extern const char MaxEventName[];
extern const char SeedName[];
extern const char SleepFactorName[];
extern const char NoGuiName[];

void bakeryParamInit(int argc, char* args[]);

// returns corresponding simulation parameter
int getNC();
int getNS();
int getNB();
int useCook();
int fairCook(); // not needed for assingment 2

long getRandSeed();    // returns a random seed for the simulation
int getSleepFactor();  // returns BakerySleepMax

// return random number in 0..max-1
int getRandVal(int max);
// return max number of events for simulation
int getMaxEvents();

void sleepEvents(); //sleep for some random time in 0..BakerySleepMax-1 ms

