//
//  testEwm.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include "ethereum/event/BREventAlarm.h"
#include "BREthereumEWMPrivate.h"

//
// EWM Tests
//
#define NODE_PAPER_KEY "ginger ..."
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

static const char *
getTokenBRDAddress (BREthereumNetwork network) {
    return (network == ethereumMainnet
            ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
            : "0x7108ca7c4718efa810457f228305c9c71390931a");
}

#if defined (BITCOIN_DEBUG)
static const char *
getTokenTSTAddress (BREthereumNetwork network) {
    return (network == ethereumMainnet
            ? "0x3efd578b271d034a69499e4a2d933c631d44b9ad"
            : "0x722dd3f80bac40c951b51bdd28dd19d435762180");
}
#endif

extern void
installTokensForTestOnNetwork (BREthereumNetwork network) {
    static int needInstall = 1;
    if (!needInstall) return;
    needInstall = 0;

    BREthereumGas defaultGasLimit = gasCreate(TOKEN_BRD_DEFAULT_GAS_LIMIT);
    BREthereumGasPrice defaultGasPrice = gasPriceCreate(etherCreateNumber(TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, WEI));
    tokenInstall (getTokenBRDAddress(network),
                  "BRD",
                  "BRD Token",
                  "",
                  18,
                  defaultGasLimit,
                  defaultGasPrice);
#if defined (BITCOIN_DEBUG)
    tokenInstall (getTokenTSTAddress(network),
                  "TST",
                  "Test Standard Token",
                  "TeST Standard Token (TST) for TeSTing (TST)",
                  18,
                  defaultGasLimit,
                  defaultGasPrice);
#endif
    tokenInstall ("0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
                  "EOS",
                  "EOS Token",
                  "",
                  18,
                  defaultGasLimit,
                  defaultGasPrice);

    tokenInstall ("0xdd974d5c2e2928dea5f71b9825b8b646686bd200",
                  "KNC",
                  "KNC token",
                  "",
                  18,
                  defaultGasLimit,
                  defaultGasPrice);
}

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
                  BREthereumWallet wid,
                  const char *address,
                  int rid) {
    ewmAnnounceWalletBalance(ewm, wid, "0x123f", rid);
}

static void
clientGetGasPrice (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumWallet wid,
                   int rid) {
    ewmAnnounceGasPrice(ewm, wid, "0xffc0", rid);
}

static void
clientEstimateGas (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumWallet wid,
                   BREthereumTransfer tid,
                   const char *from,
                   const char *to,
                   const char *amount,
                   const char *data,
                   int rid) {
    ewmAnnounceGasEstimate(ewm, wid, tid, "0x77", rid);
}

static void
clientSubmitTransaction (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         BREthereumWallet wid,
                         BREthereumTransfer tid,
                         const char *transaction,
                         int rid) {
    // The transaction hash
    ewmAnnounceSubmitTransfer(ewm, wid, tid, "0x123abc456def", -1, NULL, rid);
}

static void
clientGetTransactions (BREthereumClientContext context,
                       BREthereumEWM ewm,
                       const char *account,
                       uint64_t begBlockNumber,
                       uint64_t endBlockNumber,
                       int rid) {
    // Get all the transaction, then one by one call 'announce'
    char *address = ewmGetAccountPrimaryAddress(ewm);
    // Two transactions with an identical 'nonce' and the first one with a
    // target that is `address` - confirms a bugfix for CORE-71.
    if (begBlockNumber <= 1627184 && 1627184 <= endBlockNumber) {
        ewmAnnounceTransaction(ewm, rid,
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

        ewmAnnounceTransaction(ewm, rid,
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
    }
    ewmAnnounceTransactionComplete(ewm, rid, ETHEREUM_BOOLEAN_TRUE);
    free (address);
}

static void
clientGetLogs (BREthereumClientContext context,
               BREthereumEWM ewm,
               const char *contract,
               const char *addressIgnore,
               const char *event,
               uint64_t begBlockNumber,
               uint64_t endBlockNumber,
               int rid) {
    char *address = ewmGetAccountPrimaryAddress(ewm);
    const char *topics[] = {
        "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
        "0x0000000000000000000000000000000000000000000000000000000000000000",
        "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"
    };

    if (begBlockNumber <= 0x1e487e && 0x1e487e <= endBlockNumber)
        ewmAnnounceLog (ewm, rid,
                        "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220", // random hash...
                        getTokenBRDAddress(ewm->network),
                        3,
                        topics,
                        "0x0000000000000000000000000000000000000000000000000000000000002328",
                        "0xba43b7400",
                        "0xc64e",
                        "0x",
                        "0x1e487e",
                        "0x",
                        "0x59fa1ac9");
    ewmAnnounceLogComplete(ewm, rid, ETHEREUM_BOOLEAN_TRUE);

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

    if (0 == strcasecmp (address, "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")) {
        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_LOGS_AS_TARGET)) {
            array_add (blockNumbers, 5732521);
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_LOGS_AS_SOURCE)) {
            // ~8 blocks
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET)) {
            // ~5 blocks
        }

        if (syncInterestMatch(interests, CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE)) {
            // ~ 45 blocks
        }
    }
    else if (0 == strcasecmp (address, "0xb302B06FDB1348915599D21BD54A06832637E5E8")) {
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
    
    ewmAnnounceBlocks (ewm, rid, (int) array_count(blockNumbers), blockNumbers);
    array_free (blockNumbers);
}

static void
clientGetBlockNumber (BREthereumClientContext context,
                      BREthereumEWM ewm,
                      int rid) {
    ewmAnnounceBlockNumber(ewm, "0x2e487e", rid);
}

static void
clientGetNonce (BREthereumClientContext context,
                BREthereumEWM ewm,
                const char *address,
                int rid) {
    ewmAnnounceNonce(ewm, address, "0x4", rid);
}


static void
clientGetTokens (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 int rid) {
    ewmAnnounceToken(ewm,
                     getTokenBRDAddress(ewm->network),
                     "BRD",
                     "BRD Token",
                     "BRD Token Description",
                     18,
                     NULL,
                     NULL,
                     0);
#if defined (BITCOIN_DEBUG)
    ewmAnnounceToken(ewm,
                     getTokenTSTAddress(ewm->network),
                     "TST",
                     "Test Standard Token",
                     "TeST Standard Token (TST) for TeSTing (TST)",
                     18,
                     NULL,
                     NULL,
                     0);
#endif
    // For 0xb302B06FDB1348915599D21BD54A06832637E5E8
    ewmAnnounceToken(ewm,
                     "0x68e14bb5a45b9681327e16e528084b9d962c1a39",
                     "CAT",
                     "CAT Token",
                     "",
                     18,
                     NULL,
                     NULL,
                     0);
    
    ewmAnnounceToken(ewm,
                     "0x1234567461d3f8db7496581774bd869c83d51c93",
                     "bitclave",
                     "bitclave",
                     "",
                     18,
                     NULL,
                     NULL,
                     0);
    
    ewmAnnounceToken(ewm,
                     "0xb3bd49e28f8f832b8d1e246106991e546c323502",
                     "GMT",
                     "GMT",
                     "",
                     18,
                     NULL,
                     NULL,
                     0);
    
    ewmAnnounceToken(ewm,
                     "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
                     "EOS",
                     "EOS",
                     "",
                     18,
                     NULL,
                     NULL,
                     0);

    ewmAnnounceTokenComplete(ewm, ETHEREUM_BOOLEAN_TRUE, rid);
    
}

static void
clientEventWallet (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumWallet wid,
                   BREthereumWalletEvent event,
                   BREthereumStatus status,
                   const char *errorDescription) {
    fprintf (stdout, "ETH: TST: WalletEvent: wid=%p, ev=%d\n", wid, event);
    switch (event) {
        case WALLET_EVENT_BALANCE_UPDATED:
            signalBalance(context);
            break;
        default:
            break;
    }
}

static void
clientEventToken (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumToken token,
                   BREthereumTokenEvent event) {
    fprintf (stdout, "ETH: TST: TokenEvent: wid=%p, ev=%d\n", token, event);
}

#if defined (NEVER_DEFINED)
static void
clientEventBlock (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BREthereumBlockId bid,
                  BREthereumBlockEvent event,
                  BREthereumStatus status,
                  const char *errorDescription) {
    fprintf (stdout, "ETH: TST: BlockEvent: bid=%d, ev=%d\n", bid, event);
}
#endif

static void
clientEventTransfer (BREthereumClientContext context,
                     BREthereumEWM ewm,
                     BREthereumWallet wid,
                     BREthereumTransfer tid,
                     BREthereumTransferEvent event,
                     BREthereumStatus status,
                     const char *errorDescription) {
    fprintf (stdout, "ETH: TST: TransferEvent: tid=%p, ev=%d\n", tid, event);
}

static void
clientEventPeer (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 //BREthereumWallet wid,
                 //BREthereumTransactionId tid,
                 BREthereumPeerEvent event,
                 BREthereumStatus status,
                 const char *errorDescription) {
    fprintf (stdout, "ETH: TST: PeerEvent: ev=%d\n", event);
}

static void
clientEventEWM (BREthereumClientContext context,
                BREthereumEWM ewm,
                //BREthereumWallet wid,
                //BREthereumTransactionId tid,
                BREthereumEWMEvent event,
                BREthereumStatus status,
                const char *errorDescription) {
    fprintf (stdout, "ETH: TST: EWMEvent: ev=%d\n", event);
}

static void
clientRelease (void) {
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

    clientEventEWM,
    clientEventPeer,
    clientEventWallet,
    clientEventToken,
 //   clientEventBlock,
    clientEventTransfer
};

extern BREthereumClient
runEWM_createClient (void) {
    BREthereumClient newClient = client;
    newClient.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));
    return newClient;
}

extern void
runEWM_freeClient (BREthereumClient client) {
    free (client.context);
}

//
// Listener
//

//
//
//
static void
runEWM_CONNECT_test (const char *paperKey,
                     const char *storagePath) {
    printf ("====   JSON_RPC\n");
    
    BRCoreParseStatus status;
    client.context = testContextCreate();

    BREthereumAmount balance;
    BREthereumEther expectedBalance = etherCreate(createUInt256Parse("0x123f", 16, &status));
    assert (CORE_PARSE_OK == status);

    BREthereumEWM ewm = ewmCreateWithPaperKey (ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                               BRD_ONLY,
                                               client,
                                               storagePath);
    assert (NULL != ewm);

    BREthereumWallet wallet = ewmGetWallet(ewm);
    assert (NULL != wallet);

    balance = ewmWalletGetBalance (ewm, wallet);
    assert (AMOUNT_ETHER == balance.type);
    assert (ETHEREUM_BOOLEAN_TRUE == etherIsEQ (amountGetEther(balance), etherCreateZero()));

    // Immediately dispatches callbacks for WalletManager and Wallet events. Notable, wallet
    // create and a wallet update balance events.
    ewmConnect(ewm);
    
    printf ("====     Waiting for Balance\n");

    // First balance event, from wallet creation, will be 0.  But we cannot guarantee that we won't
    // have gotten an updated balance by now.
    waitForBalance(client.context);
    balance = ewmWalletGetBalance (ewm, wallet);
    assert (AMOUNT_ETHER == balance.type);
    assert (ETHEREUM_BOOLEAN_TRUE == etherIsEQ (amountGetEther(balance), etherCreateZero()) ||
            ETHEREUM_BOOLEAN_TRUE == etherIsEQ (amountGetEther(balance), expectedBalance));

    // the proper approach is to wait on a 'EWM' connected event.
    sleep (2);  // let connect 'take'

    // Second balance will be from the client.
    waitForBalance(client.context);

    // Callback to JSON_RPC for 'getBalanance'&
    //    ewmUpdateWalletBalance (ewm, wallet, &status);

    balance = ewmWalletGetBalance (ewm, wallet);
    assert (AMOUNT_ETHER == amountGetType(balance));
    assert (ETHEREUM_BOOLEAN_TRUE == etherIsEQ (expectedBalance, amountGetEther(balance)));

    //    ewmUpdateTransactions(ewm);

//    int count = ewmWalletGetTransferCount(ewm, wallet);
//    assert (2 == count);

    testContextRelease(client.context);
    clientRelease();
    ewmDisconnect(ewm);
    ewmDestroy(ewm);
}

//
//
//
void prepareTransaction (const char *paperKey,
                         const char *storagePath,
                         const char *recvAddr,
                         const uint64_t gasPrice,
                         const uint64_t gasLimit,
                         const uint64_t amount) {
    printf ("====   Prepare Transaction\n");
    
    // START - One Time Code Block
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));
    
    BREthereumEWM ewm = ewmCreateWithPaperKey (ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                               P2P_ONLY,
                                               client,
                                               storagePath);
    // A wallet amount Ether
    BREthereumWallet wallet = ewmGetWallet(ewm);
    // END - One Time Code Block
    
    // Optional - will provide listNodeWalletCreateTransactionDetailed.
    ewmWalletSetDefaultGasPrice(ewm, wallet, gasPriceCreate(etherCreateNumber(gasPrice, GWEI)));
    ewmWalletSetDefaultGasLimit(ewm, wallet, gasCreate(gasLimit));

    BREthereumAmount amountAmountInEther =
    ewmCreateEtherAmountUnit(ewm, amount, WEI);
    
    BREthereumTransfer tx1 = ewmWalletCreateTransfer
    (ewm,
     wallet,
     recvAddr,
     amountAmountInEther);
    
    ewmWalletSignTransferWithPaperKey (ewm, wallet, tx1, paperKey);
    
    const char *rawTransactionHexEncoded =
    ewmTransferGetRawDataHexEncoded(ewm, wallet, tx1, "0x");
    
    printf ("        Raw Transaction: %s\n", rawTransactionHexEncoded);
    
    char *fromAddr = ewmGetAccountPrimaryAddress(ewm);
    BREthereumTransfer *transfers = ewmWalletGetTransfers(ewm, wallet);
    assert (NULL != transfers && NULL != transfers[0]);
    
    BREthereumTransfer transfer = transfers[0];
    assert (0 == strcmp (fromAddr, addressGetEncodedString (ewmTransferGetSource(ewm, transfer), 1)) &&
            0 == strcmp (recvAddr, addressGetEncodedString (ewmTransferGetTarget(ewm, transfer), 1)));
    
    free (fromAddr);
    ewmDestroy(ewm);
    free (client.context);
}

// Local (PaperKey) -> LocalTest @ 5 GWEI gasPrice @ 21000 gasLimit & 0.0001/2 ETH
#define ACTUAL_RAW_TX "f86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260"
// eth.sendRawTran ('0xf86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260', function (err, hash) { if (!err) console.log(hash); });

extern void
testReallySend (const char *storagePath) {
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));
    
    char *paperKey = "boring ...";
    
    char *recvAddr = "0x49f4c50d9bcc7afdbcf77e0d6e364c29d5a660df";
    char *strAmount = "0.00004"; //ETH
    uint64_t gasPrice = 2; // GWEI
    uint64_t gasLimit = 21000;
    uint64_t nonce = 7;                  // Careful
    
    printf ("PaperKey: '%s'\nAddress: '%s'\nGasLimt: %" PRIu64 "\nGasPrice: %" PRIu64 " GWEI\n", paperKey, recvAddr, gasLimit, gasPrice);
    
    alarmClockCreateIfNecessary (1);
    BREthereumEWM ewm = ewmCreateWithPaperKey (ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                               P2P_ONLY,
                                               client,
                                               storagePath);
    BREthereumAccount account = ewmGetAccount(ewm);
    
    // A wallet amount Ether
    BREthereumWallet wallet = ewmGetWallet(ewm);
    BREthereumAddress address = accountGetPrimaryAddress (account);
    
    accountSetAddressNonce(account, address, nonce, ETHEREUM_BOOLEAN_TRUE);
    
    // Optional - will provide listNodeWalletCreateTransactionDetailed.
    ewmWalletSetDefaultGasPrice(ewm, wallet, gasPriceCreate(etherCreateNumber(gasPrice, GWEI)));
    ewmWalletSetDefaultGasLimit(ewm, wallet, gasCreate(gasLimit));
    
    BRCoreParseStatus status;
    BREthereumAmount amountAmountInEther =
    ewmCreateEtherAmountString(ewm, strAmount, ETHER, &status);
    
    BREthereumTransfer tx = ewmWalletCreateTransfer
    (ewm,
     wallet,
     recvAddr,
     amountAmountInEther);
    
    ewmWalletSignTransferWithPaperKey (ewm, wallet, tx, paperKey);
    
    ewmConnect(ewm);
    
#if 1 // only submit by explicit action.
    printf ("***\n***\n***\n*** WAITING TO SUBMIT\n***\n");
    sleep (10);
    printf ("***\n***\n***\n*** SUBMITING\n***\n");
    
    ewmWalletSubmitTransfer(ewm, wallet, tx);
#endif
    // 2 minutes ?? to confirm
    unsigned int remaining = 2 * 60;
    while (remaining) {
        printf ("***\n*** SLEEPING: %d\n", remaining);
        remaining = sleep(remaining);
    }
    
    ewmTransferDelete(ewm, tx);
    ewmDisconnect(ewm);
    ewmDestroy(ewm);
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
runEWM_TOKEN_test (const char *paperKey,
                   const char *storagePath) {
    printf ("====   TOKEN\n");
    
    BRCoreParseStatus status;
    
    BREthereumToken token = tokenLookup(getTokenBRDAddress(ethereumMainnet));
    BREthereumEWM ewm = ewmCreateWithPaperKey (ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                               P2P_ONLY,
                                               client,
                                               storagePath);
    BREthereumWallet wid = ewmGetWalletHoldingToken(ewm, token);
    
    BREthereumAmount amount = ewmCreateTokenAmountString(ewm, token,
                                                         TEST_TRANS3_DECIMAL_AMOUNT,
                                                         TOKEN_QUANTITY_TYPE_DECIMAL,
                                                         &status);
    BREthereumTransfer tid =
    ewmWalletCreateTransfer (ewm, wid,
                             TEST_TRANS3_TARGET_ADDRESS,
                             amount);
    
    const char *rawTxUnsigned = ewmTransferGetRawDataHexEncoded(ewm, wid, tid, "0x");
    printf ("        RawTx Unsigned: %s\n", rawTxUnsigned);
    // No match: nonce, gasLimit, gasPrice differ
    // assert (0 == strcasecmp(&rawTxUnsigned[2], TEST_TRANS3_UNSIGNED_TX));
    
    ewmWalletSignTransferWithPaperKey(ewm, wid, tid, paperKey);
    const char *rawTxSigned = ewmTransferGetRawDataHexEncoded(ewm, wid, tid, "0x");
    printf ("        RawTx  Signed: %s\n", rawTxSigned);
    
    ewmTransferDelete(ewm, tid);
    ewmDestroy(ewm);
    
}

static void
runEWM_PUBLIC_KEY_test (BREthereumNetwork network,
                        const char *paperKey,
                        const char *storagePath) {
    printf ("====   PUBLIC KEY\n");

    BREthereumEWM ewm1 = ewmCreateWithPaperKey (ethereumMainnet, paperKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                                P2P_ONLY,
                                                client,
                                                storagePath);
    BRKey publicKey = ewmGetAccountPrimaryAddressPublicKey (ewm1);
    char *addr1 = ewmGetAccountPrimaryAddress (ewm1);
    
    BREthereumEWM ewm2 = ewmCreateWithPublicKey (ethereumMainnet, publicKey, ETHEREUM_TIMESTAMP_UNKNOWN,
                                                 P2P_ONLY,
                                                 client,
                                                 storagePath);
    char *addr2 = ewmGetAccountPrimaryAddress (ewm2);
    
    
    assert (0 == strcmp (addr1, addr2));
    
    free (addr1);
    free (addr2);
    ewmDestroy(ewm1);
    ewmDestroy(ewm2);
}

extern void
runSyncTest (BREthereumNetwork network,
             BREthereumAccount account,
             BREthereumMode mode,
             BREthereumTimestamp accountTimestamp,
             unsigned int durationInSeconds,
             const char *storagePath) {
    BREthereumEWM ewm;

    eth_log("TST", "SyncTest%s", "");
    
    //    installTokensForTestOnNetwork(ethereumMainnet);
    
    client.context = (JsonRpcTestContext) calloc (1, sizeof (struct JsonRpcTestContextRecord));
    
    alarmClockCreateIfNecessary (1);

    ewm = ewmCreate (ethereumMainnet, account, accountTimestamp, mode, client, storagePath);

    
    char *address = ewmGetAccountPrimaryAddress(ewm);
    printf ("ETH: TST:\nETH: TST: Address: %s\nETH: TST:\n", address);
    free (address);

    ewmUpdateTokens(ewm);
    ewmConnect(ewm);
    
    unsigned int remaining = durationInSeconds;
    while (remaining) {
        printf ("ETH: TST:\nETH: TST: sleeping: %d\nETH: TST:\n", remaining);
        remaining = sleep(remaining);
    }
    
    ewmDisconnect(ewm);
    ewmDestroy(ewm);
    alarmClockDestroy(alarmClock);
    free (client.context);
    return;
}

extern void
runEWMTests (const char *paperKey,
             const char *storagePath) {
    installTokensForTestOnNetwork(ethereumMainnet);
    printf ("==== EWM\n");
    // prepareTransaction(NODE_PAPER_KEY, NODE_RECV_ADDR, TEST_TRANS2_GAS_PRICE_VALUE, GAS_LIMIT_DEFAULT, NODE_ETHER_AMOUNT);
    if (NULL == paperKey) paperKey = NODE_PAPER_KEY;

    runEWM_CONNECT_test(paperKey, storagePath);
    runEWM_TOKEN_test (paperKey, storagePath);
    runEWM_PUBLIC_KEY_test (ethereumMainnet, paperKey, storagePath);
}
