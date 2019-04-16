//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//
#include "ethereum/BREthereum.h"

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

// testCrypto.c
extern void runCryptoTests (void);

// Ripple
extern void
runRippleTest (void /* ... */);

#include "test.h"

