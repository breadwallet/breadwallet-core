//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//
#include "BREthereum.h"

// Bitcoin
extern int BRRunSupTests (void);

extern int BRRunTests();

extern int BRRunTestsSync (const char *paperKey,
                           int isBTC,
                           int isMainnet);

extern int BRRunTestWalletManagerSync (const char *paperKey,
                                       const char *storagePath,
                                       int isBTC,
                                       int isMainnet);

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
extern void runEWMTests (const char *paperKey);

extern void runSyncTest (BREthereumNetwork network,
                         BREthereumAccount account,
                         BREthereumMode mode,
                         BREthereumTimestamp accountTimestamp,
                         unsigned int durationInSeconds,
                         const char *storagePath,
                         int restart);

// LES
extern void
runLESTests (const char *paperKey);

extern void
runNodeTests (void);

// Top-Level
extern void runTests (int reallySend);

extern void runPerfTestsCoder (int repeat, int many);

extern void BRRandInit (void);
