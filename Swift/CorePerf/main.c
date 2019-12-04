//
//  main.c
//  CorePerf
//
//  Created by Ed Gamble on 10/11/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <stdio.h>
#include <unistd.h>
#include "support/BRBIP39WordsEn.h"
#include "ethereum/BREthereum.h"
#include "ethereum/test.h"  // runSyncTest

extern BREthereumClient
runEWM_createClient (void);

extern void
runEWM_freeClient (BREthereumClient client);

static void
runSyncMany (BREthereumNetwork newtork,
             BRCryptoSyncMode mode,
             unsigned int durationInSeconds,
             unsigned int accounts) {

    BREthereumEWM ewms[accounts];
    BREthereumClient clients[accounts];
    BREthereumTimestamp timestamp = 1539330275; // ETHEREUM_TIMESTAMP_UNKNOWN;

    for (int i = 0; i < accounts; i++) {
        UInt128 entropy;
        arc4random_buf(entropy.u64, sizeof (entropy));

        size_t phraseLen = BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy.u8, sizeof(entropy));
        char phrase[phraseLen];

        assert (phraseLen == BRBIP39Encode(phrase, sizeof(phrase), BRBIP39WordsEn, entropy.u8, sizeof(entropy)));

        BREthereumAccount account = createAccount(phrase);

        char storagePath[100];
        sprintf (storagePath, "many%d", i);

        BREthereumEWM ewm;

        eth_log("TST", "SyncTest: PaperKey: %s", phrase);


//        alarmClockCreateIfNecessary (1);

        clients[i] = runEWM_createClient();

        ewm = ewmCreate (ethereumMainnet, account, timestamp, mode, clients[i], storagePath, 0, 6);
        ewms[i] = ewm;

        char *address = ewmGetAccountPrimaryAddress(ewm);
        printf ("ETH: TST:\nETH: TST: Address: %s\nETH: TST:\n", address);
        free (address);

        ewmUpdateTokens(ewm);
        ewmConnect(ewm);
    }

    unsigned int remaining = durationInSeconds;
    while (remaining) {
        printf ("ETH: TST:\nETH: TST: sleeping: %d\nETH: TST:\n", remaining);
        remaining = sleep(remaining);
    }

    for (size_t i = 0; i < accounts; i++) {
        ewmDisconnect(ewms[i]);
        ewmDestroy(ewms[i]);
        runEWM_freeClient(clients[i]);
    }
//    alarmClockDestroy(alarmClock);
}


int main(int argc, const char * argv[]) {
    BRCryptoSyncMode mode = CRYPTO_SYNC_MODE_API_WITH_P2P_SEND;

    const char *paperKey = (argc > 1 ? argv[1] : "0xa9de3dbd7d561e67527bc1ecb025c59d53b9f7ef");
    BREthereumAccount account = createAccount(paperKey);
    BREthereumTimestamp timestamp = 1539330275; // ETHEREUM_TIMESTAMP_UNKNOWN;
    const char *path = "core";


    runSyncTest (ethereumMainnet,  account, mode, timestamp,  5 * 60, path);

//    runSyncMany(ethereumMainnet, mode, 10 * 60, 1000);

    return 0;
}
