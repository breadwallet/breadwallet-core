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

extern void BRRandInit (void);

#include "test.h"

