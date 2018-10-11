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

extern void runSyncTest (BREthereumType type,
                         BREthereumSyncMode mode,
                         unsigned int durationInSeconds,
                         int restart);

int main(int argc, const char * argv[]) {
    BREthereumType type = EWM_USE_LES;
    BREthereumSyncMode mode = SYNC_MODE_PRIME_WITH_ENDPOINT;

    runSyncTest (type, mode, 5 * 60, 0);

    return 0;
}
