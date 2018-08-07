//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

// Bitcoin
extern int BRRunTests();

// Util
extern void runUtilTests (void);

// RLP
extern void runRlpTests (void);

// Event
extern void runEventTests (void);

// Block Chain
extern void runBcTests (void);

// Contract
extern void runContractTests (void);

// EWM
extern void runEWMTests (void);
extern void runSyncTest (unsigned int durationInSeconds,
                         int restart);

// LES
extern void runLEStests(void);

// Top-Level
extern void runTests (int reallySend);
