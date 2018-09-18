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

#include "BREthereumPrivate.h"
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
                               address,
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
    ethereumClientAnnounceToken(ewm, rid,
                                "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                                "BRD",
                                "BRD Token",
                                "BRD Token Description",
                                18);
}

//
// Save Blocks
//
BRArrayOf(BREthereumPersistData) savedBlocks = NULL;

static void
clientSaveBlocks (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BRArrayOf(BREthereumPersistData) blocksToSave) {
    static int count = 0;
    static int total = 0;
    total += array_count(blocksToSave);
    eth_log("TST", "Save Blocks (%d): %d", (++count), total);

    if (NULL != savedBlocks) {
        for (size_t item = 0; item < array_count(savedBlocks); item++)
            rlpDataRelease(savedBlocks[item].blob);
        array_free(savedBlocks);
        savedBlocks = NULL;
    }

    array_new (savedBlocks, array_count(blocksToSave));
    for (size_t index = 0; index < array_count(blocksToSave); index++)
        array_add (savedBlocks, blocksToSave[index]);

    array_free (blocksToSave);
}

//
// Save Peers
//
BRArrayOf(BREthereumPersistData) savedNodes = NULL;

static void
clientSaveNodes (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 BRArrayOf(BREthereumPersistData) nodesToSave) {
    if (NULL != savedNodes) {
        for (size_t item = 0; item < array_count(savedNodes); item++)
            rlpDataRelease(savedNodes[item].blob);
        array_free(savedNodes);
        savedNodes = NULL;
    }
    savedNodes = nodesToSave;

}

//
// Update Transactions
//
BRSetOf(BREthereumPersistData) savedTransactions = NULL;

static void
clientUpdateTransaction (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         BREthereumClientChangeType type,
                         BREthereumPersistData transactionPersistData) {
    fprintf (stdout, "ETH: TST: UpdateTransaction: ev=%s @ %p\n", CLIENT_CHANGE_TYPE_NAME(type), transactionPersistData.blob.bytes);

    if (NULL == savedTransactions)
        savedTransactions = BRSetNew(persistDataHashValue, persistDataHashEqual, 100);

    BREthereumPersistData *data = malloc (sizeof (BREthereumPersistData));
    memcpy (data, &transactionPersistData, sizeof (BREthereumPersistData));

    switch (type) {
        case CLIENT_CHANGE_ADD:
            BRSetAdd(savedTransactions, data);
            break;

        case CLIENT_CHANGE_REM:
            data = BRSetRemove(savedTransactions, data);
            if (NULL != data) { rlpDataRelease(data->blob); free (data); }
            break;

        case CLIENT_CHANGE_UPD:
            data = BRSetAdd(savedTransactions, data);
            if (NULL != data) { rlpDataRelease(data->blob); free (data); }
            break;
    }
}

//
// Update Log
//
BRSetOf(BREthereumPersistData) savedLogs = NULL;

static void
clientUpdateLog (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 BREthereumClientChangeType type,
                 BREthereumPersistData logPersistData) {
    fprintf (stdout, "ETH: TST: UpdateLog: ev=%s @ %p\n", CLIENT_CHANGE_TYPE_NAME(type), logPersistData.blob.bytes);

    if (NULL == savedLogs)
        savedLogs = BRSetNew(persistDataHashValue, persistDataHashEqual, 100);

    BREthereumPersistData *data = malloc (sizeof (BREthereumPersistData));
    memcpy (data, &logPersistData, sizeof (BREthereumPersistData));

    switch (type) {
        case CLIENT_CHANGE_ADD:
            BRSetAdd(savedLogs, data);
            break;

        case CLIENT_CHANGE_REM:
            data = BRSetRemove(savedLogs, data);
            if (NULL != data) { rlpDataRelease(data->blob); free (data); }
            break;

        case CLIENT_CHANGE_UPD:
            data = BRSetAdd(savedLogs, data);
            if (NULL != data) { rlpDataRelease(data->blob); free (data); }
            break;
    }
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

static BREthereumClient client = {
    NULL,

    clientGetBalance,
    clientGetGasPrice,
    clientEstimateGas,
    clientSubmitTransaction,
    clientGetTransactions,
    clientGetLogs,
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
#if 0
static void
runEWM_CONNECT_test (const char *paperKey) {
    printf ("     JSON_RCP\n");

    BRCoreParseStatus status;
    client.context = testContextCreate();

    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);

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
    ethereumDisconnect(ewm);
    ethereumDestroy(ewm);
}
#endif
//
//
//
void prepareTransaction (const char *paperKey, const char *recvAddr, const uint64_t gasPrice, const uint64_t gasLimit, const uint64_t amount) {
    printf ("     Prepare Transaction\n");

    // START - One Time Code Block
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));

    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);
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
    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);

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

    BREthereumToken token = tokenGet(0);
    BREthereumEWM ewm = ethereumCreate (ethereumMainnet, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);
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

    BREthereumEWM ewm1 = ethereumCreate (network, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);
    char *addr1 = ethereumGetAccountPrimaryAddress (ewm1);

    BRKey publicKey = ethereumGetAccountPrimaryAddressPublicKey (ewm1);
    BREthereumEWM ewm2 = ethereumCreateWithPublicKey (network, publicKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);
    char *addr2 = ethereumGetAccountPrimaryAddress (ewm2);


    assert (0 == strcmp (addr1, addr2));

    free (addr1);
    free (addr2);
    ethereumDestroy(ewm1);
    ethereumDestroy(ewm2);
}

extern void
runSyncTest (unsigned int durationInSeconds,
             int restart) {
    eth_log("TST", "SyncTest%s", "");

    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));

    char *paperKey = "0x8975dbc1b8f25ec994815626d070899dda896511"; // "boring head harsh green empty clip fatal typical found crane dinner timber";
    alarmClockCreateIfNecessary (1);

    BRArrayOf(BREthereumPersistData) blocks = (restart ? savedBlocks : NULL);
    BRArrayOf(BREthereumPersistData) nodes = (restart ? savedNodes : NULL);
    BRArrayOf(BREthereumPersistData) transactions = NULL;
    BRArrayOf(BREthereumPersistData) logs = NULL;

    if (restart) {
        if (NULL != savedTransactions) {
            size_t transactionsCount = BRSetCount(savedTransactions);
            array_new(transactions, transactionsCount);
            for (BREthereumPersistData *data = BRSetIterate(savedTransactions, NULL);
                 NULL != data;
                 data = BRSetIterate(savedTransactions, data))
                array_add (transactions, *data);
        }

        if (NULL != savedLogs) {
            size_t logsCount = BRSetCount(savedLogs);
            array_new(logs, logsCount);
            for (BREthereumPersistData *data = BRSetIterate(savedLogs, NULL);
                 NULL != data;
                 data = BRSetIterate(savedLogs, data))
                array_add (logs, *data);
        }
    }

    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client,
                                       nodes,
                                       blocks,
                                       transactions,
                                       logs);

    char *address = ethereumGetAccountPrimaryAddress(ewm);
    printf ("***\n*** Address: %s\n", address);
    free (address);

    // We passed on { node, block, etc } - we no longer own the memory.  Thus:
    savedBlocks = NULL;
    savedNodes  = NULL;

    ethereumConnect(ewm);

    unsigned int remaining = durationInSeconds;
    while (remaining) {
        printf ("***\n*** SLEEPING: %d\n", remaining);
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
    printf ("==== EWM\n");
    // prepareTransaction(NODE_PAPER_KEY, NODE_RECV_ADDR, TEST_TRANS2_GAS_PRICE_VALUE, GAS_LIMIT_DEFAULT, NODE_ETHER_AMOUNT);
    // runEWM_CONNECT_test(NODE_PAPER_KEY);
    runEWM_TOKEN_test (NODE_PAPER_KEY);
    runEWM_PUBLIC_KEY_test (ethereumMainnet, NODE_PAPER_KEY);
    runEWM_PUBLIC_KEY_test (ethereumTestnet, "ocean robust idle system close inject bronze mutual occur scale blast year");
}
