//
//  testEwm.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "BREthereumEWMPrivate.h"
#include "../event/BREventAlarm.h"

//
// EWM Tests
//
#define NODE_PAPER_KEY "ginger settle marine tissue robot crane night number ramp coast roast critic"
#define NODE_NONCE              TEST_TRANS2_NONCE // 1
#define NODE_GAS_PRICE_VALUE    TEST_TRANS2_GAS_PRICE_VALUE // 20 GWEI
#define NODE_GAS_PRICE_UNIT     TEST_TRANS2_GAS_PRICE_UNIT // WEI
#define NODE_GAS_LIMIT          TEST_TRANS2_GAS_LIMIT
#define NODE_RECV_ADDR          TEST_TRANS2_TARGET_ADDRESS
#define NODE_ETHER_AMOUNT_UNIT  TEST_TRANS2_ETHER_AMOUNT_UNIT
#define NODE_ETHER_AMOUNT       TEST_TRANS2_ETHER_AMOUNT

#define GAS_PRICE_20_GWEI       2000000000
#define GAS_PRICE_10_GWEI       1000000000
#define GAS_PRICE_5_GWEI         500000000

#define GAS_LIMIT_DEFAULT 21000

extern const char *tokenBRDAddress;

extern void
installTokensForTest (void);

//
// EWM CONNECT
//
typedef struct JsonRpcTestContextRecord {
    pthread_cond_t  cond;
    pthread_mutex_t lock;
} *JsonRpcTestContext;

static JsonRpcTestContext
testContextCreate (void) {
    JsonRpcTestContext context = malloc (sizeof (struct JsonRpcTestContextRecord));
    // Create the PTHREAD CONDition variable
    {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_cond_init(&context->cond, &attr);
        pthread_condattr_destroy(&attr);
    }

    // Create the PTHREAD LOCK variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&context->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    pthread_mutex_lock(&context->lock);

    return context;
}

static void
testContextRelease (JsonRpcTestContext context) {
    pthread_cond_destroy(&context->cond);
    pthread_mutex_destroy(&context->lock);
    free (context);
}

static void
signalBalance (JsonRpcTestContext context) {
    pthread_cond_signal(&context->cond);

}

static void
waitForBalance (JsonRpcTestContext context) {
    pthread_cond_wait(&context->cond, &context->lock);
}

// Stubbed Callbacks - should actually construct JSON, invoke an Etherum JSON_RPC method,
// get the response and return the result.
static void
clientGetBalance (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BREthereumWalletId wid,
                  const char *address,
                  int rid) {
    ethereumClientAnnounceBalance(ewm, wid, "0x123f", rid);
}

static void
clientGetGasPrice (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumWalletId wid,
                   int rid) {
    ethereumClientAnnounceGasPrice(ewm, wid, "0xffc0", rid);
}

static void
clientEstimateGas (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumWalletId wid,
                   BREthereumTransferId tid,
                   const char *to,
                   const char *amount,
                   const char *data,
                   int rid) {
    ethereumClientAnnounceGasEstimate(ewm, wid, tid, "0x77", rid);
}

static void
clientSubmitTransaction (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         BREthereumWalletId wid,
                         BREthereumTransferId tid,
                         const char *transaction,
                         int rid) {
    // The transaction hash
    ethereumClientAnnounceSubmitTransfer(ewm, wid, tid, "0x123abc456def", rid);
}

static void
clientGetTransactions (BREthereumClientContext context,
                       BREthereumEWM ewm,
                       const char *account,
                       int id) {
    // Get all the transaction, then one by one call 'announce'
    char *address = ethereumGetAccountPrimaryAddress(ewm);
    // Two transactions with an identical 'nonce' and the first one with a
    // target that is `address` - confirms a bugfix for CORE-71.
    ethereumClientAnnounceTransaction(ewm, id,
                                      "0x5f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                                      "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                                      address,   // required
                                      "",
                                      "11113000000000",
                                      "21000",
                                      "21000000000",
                                      "",
                                      "118",
                                      "21000",
                                      "1627184",
                                      "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                                      "339050",
                                      "3",
                                      "1516477482",
                                      "0");
    ethereumClientAnnounceTransaction(ewm, id,
                                      "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                                      address,   // required
                                      "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                                      "",
                                      "11113000000000",
                                      "21000",
                                      "21000000000",
                                      "",
                                      "118",
                                      "21000",
                                      "1627184",
                                      "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                                      "339050",
                                      "3",
                                      "1516477482",
                                      "0");


    free (address);
}

static void
clientGetLogs (BREthereumClientContext context,
               BREthereumEWM ewm,
               const char *contract,
               const char *addressIgnore,
               const char *event,
               int rid) {
    char *address = ethereumGetAccountPrimaryAddress(ewm);
    const char *topics[] = {
        "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
        "0x0000000000000000000000000000000000000000000000000000000000000000",
        "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"
    };
    ethereumClientAnnounceLog (ewm, rid,
                               "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220", // random hash...
                               "0x722dd3f80bac40c951b51bdd28dd19d435762180",
                               3,
                               topics,
                               "0x0000000000000000000000000000000000000000000000000000000000002328",
                               "0xba43b7400",
                               "0xc64e",
                               "0x",
                               "0x1e487e",
                               "0x",
                               "0x59fa1ac9");
    free (address);
}

static void
clientGetBlocks (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 const char *address,
                 BREthereumSyncInterestSet interests,
                 uint64_t blockNumberStart,
                 uint64_t blockNumberStop,
                 int rid) {
    BRArrayOf(uint64_t) blockNumbers;
    array_new (blockNumbers, 10);

    if (0 == strcasecmp (address, "0xb302B06FDB1348915599D21BD54A06832637E5E8")) {
        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_LOGS_AS_TARGET)) {
            array_add (blockNumbers, 4847049);
            array_add (blockNumbers, 4847152);
            array_add (blockNumbers, 4894677);
            array_add (blockNumbers, 4965538);
            array_add (blockNumbers, 4999850);
            array_add (blockNumbers, 5029844);
            // array_add (blockNumbers, 5705175);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_LOGS_AS_SOURCE)) {
            array_add (blockNumbers, 5705175);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET)) {
            array_add (blockNumbers, 4894027);
            array_add (blockNumbers, 4908682);
            array_add (blockNumbers, 4991227);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE)) {
            array_add (blockNumbers, 4894330);
            array_add (blockNumbers, 4894641);
            array_add (blockNumbers, 4894677);

            array_add (blockNumbers, 4903993);
            array_add (blockNumbers, 4906377);
            array_add (blockNumbers, 4997449);

            array_add (blockNumbers, 4999850);
            array_add (blockNumbers, 4999875);
            array_add (blockNumbers, 5000000);

            // TODO: When arrives at BCS - don't request unless the node is up-to-date!
            array_add (blockNumbers, 5705175);
        }
    }

    else if (0 == strcasecmp (address, "0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef")) {
        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_LOGS_AS_TARGET)) {
            array_add (blockNumbers, 5506607);
            array_add (blockNumbers, 5877545);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_LOGS_AS_SOURCE)) {
            array_add (blockNumbers, 5509990);
            array_add (blockNumbers, 5509990);
            array_add (blockNumbers, 5511681);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET)) {
            array_add (blockNumbers, 5506602);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE)) {
            array_add (blockNumbers, 5506764);
            array_add (blockNumbers, 5509990);
            array_add (blockNumbers, 5511681);
            array_add (blockNumbers, 5539808);
        }
    }

    else {
        array_add (blockNumbers, blockNumberStart);
        array_add (blockNumbers, (blockNumberStart + blockNumberStop) / 2);
        array_add (blockNumbers, blockNumberStop);
    }

    // Remote block numbers out of range.
    for (size_t index = array_count(blockNumbers); index > 0; index--)
        if (blockNumbers[index - 1] < blockNumberStart ||
            blockNumbers[index - 1] > blockNumberStop)
            array_rm (blockNumbers, index - 1);

    ethereumClientAnnounceBlocks (ewm, rid, (int) array_count(blockNumbers), blockNumbers);
    array_free (blockNumbers);
}

static void
clientGetBlockNumber (BREthereumClientContext context,
                      BREthereumEWM ewm,
                      int rid) {
    ethereumClientAnnounceBlockNumber(ewm, "0x1e487e", rid);
}

static void
clientGetNonce (BREthereumClientContext context,
                BREthereumEWM ewm,
                const char *address,
                int rid) {
    ethereumClientAnnounceNonce(ewm, address, "0x4", rid);
}


static void
clientGetTokens (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 int rid) {
    ethereumClientAnnounceToken(ewm,
                                "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                                "BRD",
                                "BRD Token",
                                "BRD Token Description",
                                18,
                                NULL,
                                NULL,
                                0);

    // For 0xb302B06FDB1348915599D21BD54A06832637E5E8
    ethereumClientAnnounceToken(ewm,
                                "0x68e14bb5a45b9681327e16e528084b9d962c1a39",
                                "CAT",
                                "CAT Token",
                                "",
                                18,
                                NULL,
                                NULL,
                                0);

    ethereumClientAnnounceToken(ewm,
                                "0x1234567461d3f8db7496581774bd869c83d51c93",
                                "bitclave",
                                "bitclave",
                                "",
                                18,
                                NULL,
                                NULL,
                                0);

    ethereumClientAnnounceToken(ewm,
                                "0xb3bd49e28f8f832b8d1e246106991e546c323502",
                                "GMT",
                                "GMT",
                                "",
                                18,
                                NULL,
                                NULL,
                                0);

    ethereumClientAnnounceToken(ewm,
                                "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
                                "EOS",
                                "EOS",
                                "",
                                18,
                                NULL,
                                NULL,
                                0);

}

//
// Save Blocks
//
BRSetOf(BREthereumHashDataPair) savedBlocks = NULL;

static void
clientSaveBlocks (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BRSetOf(BREthereumHashDataPair) blocksToSave) {
    static int count = 0;
    static int total = 0;
    total += BRSetCount(blocksToSave);
    eth_log("TST", "Save Blocks (%d): %d", (++count), total);

    if (NULL == savedBlocks)
        savedBlocks = hashDataPairSetCreateEmpty (BRSetCount(blocksToSave));

    BRSetUnion (savedBlocks, blocksToSave);
    BRSetFree (blocksToSave);
}

//
// Save Peers
//
BRSetOf(BREthereumHashDataPair) savedNodes = NULL;

static void
clientSaveNodes (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 OwnershipGiven BRSetOf(BREthereumHashDataPair) nodesToSave) {
    if (NULL == savedNodes)
        savedNodes = hashDataPairSetCreateEmpty (BRSetCount(nodesToSave));

    BRSetUnion (savedNodes, nodesToSave);
    BRSetFree (nodesToSave);
}

//
// Update Transactions
//
BRSetOf(BREthereumHashDataPair) savedTransactions = NULL;

static void
clientUpdateTransaction (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         BREthereumClientChangeType type,
                         OwnershipGiven BREthereumHashDataPair new) {
    fprintf (stdout, "ETH: TST: UpdateTransaction: ev=%s @ %p\n",
             CLIENT_CHANGE_TYPE_NAME(type),
             hashDataPairGetData(new).bytes);

    if (NULL == savedTransactions)
        savedTransactions = hashDataPairSetCreateEmpty (100);

    BREthereumHashDataPair old = BRSetRemove (savedTransactions, new);
    if (NULL != old) hashDataPairRelease(old);

    BRSetAdd(savedTransactions, new);
}

//
// Update Log
//
BRSetOf(BREthereumHashDataPair) savedLogs = NULL;

static void
clientUpdateLog (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 BREthereumClientChangeType type,
                 OwnershipGiven BREthereumHashDataPair new) {
    fprintf (stdout, "ETH: TST: UpdateLog: ev=%s @ %p\n",
             CLIENT_CHANGE_TYPE_NAME(type),
             hashDataPairGetData(new).bytes);

    if (NULL == savedLogs)
        savedLogs = hashDataPairSetCreateEmpty (100);

    BREthereumHashDataPair old = BRSetRemove (savedLogs, new);
    if (NULL != old) hashDataPairRelease(old);

    BRSetAdd (savedLogs, new);
}

static void
clientEventWallet (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumWalletId wid,
                   BREthereumWalletEvent event,
                   BREthereumStatus status,
                   const char *errorDescription) {
    fprintf (stdout, "ETH: TST: WalletEvent: wid=%d, ev=%d\n", wid, event);
    switch (event) {
        case WALLET_EVENT_BALANCE_UPDATED:
            signalBalance(context);
            break;
        default:
            break;
    }
}

static void
clientEventBlock (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BREthereumBlockId bid,
                  BREthereumBlockEvent event,
                  BREthereumStatus status,
                  const char *errorDescription) {
    fprintf (stdout, "ETH: TST: BlockEvent: bid=%d, ev=%d\n", bid, event);

}

static void
clientEventTransfer (BREthereumClientContext context,
                     BREthereumEWM ewm,
                     BREthereumWalletId wid,
                     BREthereumTransferId tid,
                     BREthereumTransferEvent event,
                     BREthereumStatus status,
                     const char *errorDescription) {
    fprintf (stdout, "ETH: TST: TransferEvent: tid=%d, ev=%d\n", tid, event);
}

static void
clientEventPeer (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 //BREthereumWalletId wid,
                 //BREthereumTransactionId tid,
                 BREthereumPeerEvent event,
                 BREthereumStatus status,
                 const char *errorDescription) {
    fprintf (stdout, "ETH: TST: PeerEvent: ev=%d\n", event);
}

static void
clientEventEWM (BREthereumClientContext context,
                BREthereumEWM ewm,
                //BREthereumWalletId wid,
                //BREthereumTransactionId tid,
                BREthereumEWMEvent event,
                BREthereumStatus status,
                const char *errorDescription) {
    fprintf (stdout, "ETH: TST: EWMEvent: ev=%d\n", event);
}

static void
clientRelease (void) {
    BRSetApply(savedBlocks, NULL, hashDataPairReleaseForSet);
    BRSetFree(savedBlocks);

    BRSetApply(savedNodes, NULL, hashDataPairReleaseForSet);
    BRSetFree(savedNodes);

    BRSetApply(savedTransactions, NULL, hashDataPairReleaseForSet);
    BRSetFree(savedTransactions);

    BRSetApply(savedLogs, NULL, hashDataPairReleaseForSet);
    BRSetFree(savedLogs);
}

static BREthereumClient client = {
    NULL,

    clientGetBalance,
    clientGetGasPrice,
    clientEstimateGas,
    clientSubmitTransaction,
    clientGetTransactions,
    clientGetLogs,
    clientGetBlocks,
    clientGetTokens,
    clientGetBlockNumber,
    clientGetNonce,

    clientSaveNodes,
    clientSaveBlocks,
    clientUpdateTransaction,
    clientUpdateLog,

    clientEventEWM,
    clientEventPeer,
    clientEventWallet,
    clientEventBlock,
    clientEventTransfer
};

//
// Listener
//

//
//
//
static void
runEWM_CONNECT_test (const char *paperKey) {
    printf ("     JSON_RCP\n");

    BRCoreParseStatus status;
    client.context = testContextCreate();

    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                       EWM_USE_BRD, SYNC_MODE_FULL_BLOCKCHAIN,
                                       client, NULL, NULL, NULL, NULL);

    BREthereumWalletId wallet = ethereumGetWallet(ewm);

    ethereumConnect(ewm);

    printf ("       Waiting for Balance\n");
    waitForBalance(client.context);
    //    sleep (20);  // let connect 'take'

    // Callback to JSON_RPC for 'getBalanance'&
    //    ewmUpdateWalletBalance (ewm, wallet, &status);
    BREthereumAmount balance = ethereumWalletGetBalance (ewm, wallet);
    BREthereumEther expectedBalance = etherCreate(createUInt256Parse("0x123f", 16, &status));
    assert (CORE_PARSE_OK == status
            && AMOUNT_ETHER == amountGetType(balance)
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (expectedBalance, amountGetEther(balance)));

    int count = ethereumWalletGetTransferCount(ewm, wallet);
    assert (2 == count);

    //    ewmUpdateTransactions(ewm);
    testContextRelease(client.context);
    clientRelease();
    ethereumDisconnect(ewm);
    ethereumDestroy(ewm);
}

//
//
//
void prepareTransaction (const char *paperKey, const char *recvAddr, const uint64_t gasPrice, const uint64_t gasLimit, const uint64_t amount) {
    printf ("     Prepare Transaction\n");

    // START - One Time Code Block
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));

    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                       EWM_USE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                       client, NULL, NULL, NULL, NULL);
    // A wallet amount Ether
    BREthereumWalletId wallet = ethereumGetWallet(ewm);
    // END - One Time Code Block

    // Optional - will provide listNodeWalletCreateTransactionDetailed.
    ethereumWalletSetDefaultGasPrice(ewm, wallet, WEI, gasPrice);
    ethereumWalletSetDefaultGasLimit(ewm, wallet, gasLimit);

    BREthereumAmount amountAmountInEther =
    ethereumCreateEtherAmountUnit(ewm, amount, WEI);

    BREthereumTransferId tx1 =
    ethereumWalletCreateTransfer
    (ewm,
     wallet,
     recvAddr,
     amountAmountInEther);

    ethereumWalletSignTransfer (ewm, wallet, tx1, paperKey);

    const char *rawTransactionHexEncoded =
    ethereumTransferGetRawDataHexEncoded(ewm, wallet, tx1, "0x");

    printf ("        Raw Transaction: %s\n", rawTransactionHexEncoded);

    char *fromAddr = ethereumGetAccountPrimaryAddress(ewm);
    BREthereumTransferId *transfers = ethereumWalletGetTransfers(ewm, wallet);
    assert (NULL != transfers && -1 != transfers[0]);

    BREthereumTransferId transfer = transfers[0];
    assert (0 == strcmp (fromAddr, ethereumTransferGetSendAddress(ewm, transfer)) &&
            0 == strcmp (recvAddr, ethereumTransferGetRecvAddress(ewm, transfer)));

    free (fromAddr);
    ethereumDestroy(ewm);
    free (client.context);
}

// Local (PaperKey) -> LocalTest @ 5 GWEI gasPrice @ 21000 gasLimit & 0.0001/2 ETH
#define ACTUAL_RAW_TX "f86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260"
// eth.sendRawTran ('0xf86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260', function (err, hash) { if (!err) console.log(hash); });

extern void
testReallySend (void) {
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));

    char *paperKey = "boring head harsh green empty clip fatal typical found crane dinner timber";

    char *recvAddr = "0x49f4c50d9bcc7afdbcf77e0d6e364c29d5a660df";
    char *strAmount = "0.00004"; //ETH
    uint64_t gasPrice = 2; // GWEI
    uint64_t gasLimit = 21000;
    uint64_t nonce = 7;                  // Careful

    printf ("PaperKey: '%s'\nAddress: '%s'\nGasLimt: %llu\nGasPrice: %llu GWEI\n", paperKey, recvAddr, gasLimit, gasPrice);

    alarmClockCreateIfNecessary (1);
    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                       EWM_USE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                       client, NULL, NULL, NULL, NULL);

    // A wallet amount Ether
    BREthereumWalletId wallet = ethereumGetWallet(ewm);
    BREthereumAccount account = ewmGetAccount (ewm);
    BREthereumAddress address = accountGetPrimaryAddress (account);

    accountSetAddressNonce(account, address, nonce, ETHEREUM_BOOLEAN_TRUE);

    // Optional - will provide listNodeWalletCreateTransactionDetailed.
    ethereumWalletSetDefaultGasPrice(ewm, wallet, GWEI, gasPrice);
    ethereumWalletSetDefaultGasLimit(ewm, wallet, gasLimit);

    BRCoreParseStatus status;
    BREthereumAmount amountAmountInEther =
    ethereumCreateEtherAmountString(ewm, strAmount, ETHER, &status);

    BREthereumTransferId tx =
    ethereumWalletCreateTransfer
    (ewm,
     wallet,
     recvAddr,
     amountAmountInEther);

    ethereumWalletSignTransfer (ewm, wallet, tx, paperKey);

    ethereumConnect(ewm);

#if 1 // only submit by explicit action.
    printf ("***\n***\n***\n*** WAITING TO SUBMIT\n***\n");
    sleep (10);
    printf ("***\n***\n***\n*** SUBMITING\n***\n");

    ethereumWalletSubmitTransfer(ewm, wallet, tx);
#endif
    // 2 minutes ?? to confirm
    unsigned int remaining = 2 * 60;
    while (remaining) {
        printf ("***\n*** SLEEPING: %d\n", remaining);
        remaining = sleep(remaining);
    }

    ewmDeleteTransfer(ewm, tx);
    ethereumDisconnect(ewm);
    ethereumDestroy(ewm);
    alarmClockDestroy(alarmClock);
    free (client.context);
    return;
}

//
//
//
#define TEST_TRANS3_TARGET_ADDRESS "0x932a27e1bc84f5b74c29af3d888926b1307f4a5c"
#define TEST_TRANS3_GAS_PRICE_VALUE 50 // 20 GWEI
#define TEST_TRANS3_GAS_PRICE_UNIT  GWEI
#define TEST_TRANS3_GAS_LIMIT 74858
#define TEST_TRANS3_NONCE 423490
#define TEST_TRANS3_DECIMAL_AMOUNT "5968.77"


// Unsigned Result: 0xf864010082c35094558ec3152e2eb2174905cd19aea4e34a23de9ad680b844a9059cbb000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c0000000000000000000000000000000000000000000000000000000000000000018080
//   Signed Result: 0xf8a4010082c35094558ec3152e2eb2174905cd19aea4e34a23de9ad680b844a9059cbb000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c000000000000000000000000000000000000000000000000000000000000000025a0b729de661448a377bee9ef3f49f8ec51f6c5810cf177a8162d31e9611a940a18a030b44adbe0253fe6176ccd8b585745e60f411b009ec73815f201fff0f540fc4d
static void
runEWM_TOKEN_test (const char *paperKey) {
    printf ("     TOKEN\n");

    BRCoreParseStatus status;

    BREthereumToken token = tokenLookup(tokenBRDAddress);
    BREthereumEWM ewm = ethereumCreate (ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                        EWM_USE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                        client, NULL, NULL, NULL, NULL);
    BREthereumWalletId wid = ethereumGetWalletHoldingToken(ewm, token);

    BREthereumAmount amount = ethereumCreateTokenAmountString(ewm, token,
                                                              TEST_TRANS3_DECIMAL_AMOUNT,
                                                              TOKEN_QUANTITY_TYPE_DECIMAL,
                                                              &status);
    BREthereumTransferId tid =
    ethereumWalletCreateTransfer (ewm, wid,
                                     TEST_TRANS3_TARGET_ADDRESS,
                                     amount);

    const char *rawTxUnsigned = ethereumTransferGetRawDataHexEncoded(ewm, wid, tid, "0x");
    printf ("        RawTx Unsigned: %s\n", rawTxUnsigned);
    // No match: nonce, gasLimit, gasPrice differ
    // assert (0 == strcasecmp(&rawTxUnsigned[2], TEST_TRANS3_UNSIGNED_TX));

    ethereumWalletSignTransfer(ewm, wid, tid, paperKey);
    const char *rawTxSigned = ethereumTransferGetRawDataHexEncoded(ewm, wid, tid, "0x");
    printf ("        RawTx  Signed: %s\n", rawTxSigned);

    ewmDeleteTransfer(ewm, tid);
    ethereumDestroy(ewm);

}

static void
runEWM_PUBLIC_KEY_test (BREthereumNetwork network, const char *paperKey) {
    printf ("     PUBLIC KEY\n");

    BREthereumEWM ewm1 = ethereumCreate (network, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                         EWM_USE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                         client, NULL, NULL, NULL, NULL);
    char *addr1 = ethereumGetAccountPrimaryAddress (ewm1);

    BRKey publicKey = ethereumGetAccountPrimaryAddressPublicKey (ewm1);
    BREthereumEWM ewm2 = ethereumCreateWithPublicKey (network, publicKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                                      EWM_USE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                                      client, NULL, NULL, NULL, NULL);
    char *addr2 = ethereumGetAccountPrimaryAddress (ewm2);


    assert (0 == strcmp (addr1, addr2));

    free (addr1);
    free (addr2);
    ethereumDestroy(ewm1);
    ethereumDestroy(ewm2);
}

extern void
runSyncTest (BREthereumType type,
             BREthereumSyncMode mode,
             BREthereumTimestamp accountTimestamp,
             unsigned int durationInSeconds,
             int restart) {
    eth_log("TST", "SyncTest%s", "");

    installTokensForTest();
    
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));

#if ! defined (DEBUG)
    char *paperKey = "boring head harsh green empty clip fatal typical found crane dinner timber";
#else
    char *paperKey = "0xa9de3dbd7d561e67527bc1ecb025c59d53b9f7ef";
//    char *paperKey = "0x8975dbc1b8f25ec994815626d070899dda896511";
//    char *paperKey = "0xb302B06FDB1348915599D21BD54A06832637E5E8";
#endif
    alarmClockCreateIfNecessary (1);

    BRSetOf(BREthereumHashDataPair) blocks = (restart ? savedBlocks : NULL);
    BRSetOf(BREthereumHashDataPair) nodes = (restart ? savedNodes : NULL);
    BRSetOf(BREthereumHashDataPair) transactions = NULL;
    BRSetOf(BREthereumHashDataPair) logs = NULL;

    if (restart) {
        if (NULL != savedTransactions) {
            transactions = BRSetNew (transactionHashValue, transactionHashEqual, BRSetCount (savedTransactions));
            BRSetUnion (transactions, savedTransactions); // copy
        }

        if (NULL != savedLogs) {
            logs = BRSetNew (logHashValue, logHashEqual, BRSetCount(savedLogs));
            BRSetUnion (logs, savedLogs); // copy
        }
    }

    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, accountTimestamp,
                                       type, mode, client,
                                       nodes,
                                       blocks,
                                       transactions,
                                       logs);

    char *address = ethereumGetAccountPrimaryAddress(ewm);
    printf ("ETH: TST:\nETH: TST: Address: %s\nETH: TST:\n", address);
    free (address);

    // We passed on { node, block, etc } - we no longer own the memory.  Thus:
    savedBlocks = NULL;
    savedNodes  = NULL;

    ethereumConnect(ewm);

    unsigned int remaining = durationInSeconds;
    while (remaining) {
        printf ("ETH: TST:\nETH: TST: sleeping: %d\nETH: TST:\n", remaining);
        remaining = sleep(remaining);
    }

    ethereumDisconnect(ewm);
    ethereumDestroy(ewm);
    alarmClockDestroy(alarmClock);
    free (client.context);
    return;
}

extern void
runEWMTests (void) {
    installTokensForTest();
    printf ("==== EWM\n");
    // prepareTransaction(NODE_PAPER_KEY, NODE_RECV_ADDR, TEST_TRANS2_GAS_PRICE_VALUE, GAS_LIMIT_DEFAULT, NODE_ETHER_AMOUNT);
    runEWM_CONNECT_test(NODE_PAPER_KEY);
    runEWM_TOKEN_test (NODE_PAPER_KEY);
    runEWM_PUBLIC_KEY_test (ethereumMainnet, NODE_PAPER_KEY);
    runEWM_PUBLIC_KEY_test (ethereumTestnet, "ocean robust idle system close inject bronze mutual occur scale blast year");
}
