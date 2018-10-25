//
//  main.c
//  CorePerf
//
//  Created by Ed Gamble on 10/11/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include "BREthereum.h"

extern void runSyncTest (const char *paperKey,
                         BREthereumType type,
                         BREthereumSyncMode mode,
                         BREthereumTimestamp timestamp,
                         unsigned int durationInSeconds,
                         int restart);

int main(int argc, const char * argv[]) {
    BREthereumType type = EWM_USE_LES;
    BREthereumSyncMode mode = SYNC_MODE_FULL_BLOCKCHAIN;
    BREthereumTimestamp timestamp = 1539330275; // ETHEREUM_TIMESTAMP_UNKNOWN;

    runSyncTest (NULL, type, mode, timestamp,  2 * 60, 0);

    return 0;
}
