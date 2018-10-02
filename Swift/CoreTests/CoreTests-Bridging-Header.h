//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//
#include "BREthereum.h"

// Bitcoin
extern int BRRunTests();
extern int BRRunTestsSync (int randomKey);

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

extern void runSyncTest (BREthereumType type,
                         BREthereumSyncMode mode,
                         unsigned int durationInSeconds,
                         int restart);

// LES
extern void
runLEStests (void);

extern void
runNodeTests (void);

// Top-Level
extern void runTests (int reallySend);

extern void runPerfTestsCoder (int repeat, int many);
