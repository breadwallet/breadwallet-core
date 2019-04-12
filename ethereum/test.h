//
//  test.h
//  CoreTests
//
//  Created by Ed Gamble on 2/14/19.
//  Copyright © 2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Test_H
#define BR_Ethereum_Test_H

#ifdef __cplusplus
extern "C" {
#endif

// Util
extern void runUtilTests (void);

// RLP
extern void runRlpTests (void);

// Event
extern void runEventTests (void);

// Base
extern void runBaseTests (void);

// Block Chain
extern void runBcTests (void);

// Contract
extern void runContractTests (void);

// LES
extern void runLESTests(const char *paperKey);

extern void
runNodeTests (void);

// EWM
extern void
runEWMTests (const char *paperKey,
             const char *storagePath);
    
extern void
runSyncTest (BREthereumNetwork network,
             BREthereumAccount account,
             BREthereumMode mode,
             BREthereumTimestamp accountTimestamp,
             unsigned int durationInSeconds,
             const char *storagePath);

//
extern void
installTokensForTest (void);

extern void
runTests (int reallySend);

extern void
runPerfTestsCoder (int repeat, int many);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Test_H */
