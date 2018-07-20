//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

extern int BRRunTests();
extern void runTests (int reallySend);
extern void runSyncTest (unsigned int durationInSeconds,
                         int restart);
extern void runLEStests(void);
