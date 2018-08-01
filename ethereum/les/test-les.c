//
//  test-les.c
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/16/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <regex.h>
#include "test-les.h"
#include "BRInt.h"
#include "../base/BREthereumHash.h"
#include "BREthereumLES.h"
#include "BREthereumNode.h"
#include "BREthereumNodeEventHandler.h"
#include "BREthereumNodeManager.h"
#include "../blockchain/BREthereumNetwork.h"
#include "BREthereumNodeDiscovery.h"
#include "BREthereumRandom.h"
#include "BRCrypto.h"
#include "BREthereum.h"
#include "BREthereumHandshake.h"
#include "../ewm/BREthereumEWM.h"

// pthread locks/conditions and wait and signal functions
static pthread_mutex_t _testLock;
static pthread_cond_t _testCond;
static int _testComplete;
static int _numOfTests;

static void _initTestState() {

    // Initializes the pthread variables
    pthread_mutex_init(&_testLock, NULL);
    pthread_cond_init(&_testCond, NULL);
    _testComplete = 0;
}

static void _initTest(int numOfTests) {
    _testComplete = 0;
    _numOfTests = numOfTests;
}
static void _waitForTests() {
    //Wait for a little bit to get a reply back from the server.
    pthread_mutex_lock(&_testLock);
    //Wait until all the tests are complete
    while(_testComplete < _numOfTests){
        pthread_cond_wait(&_testCond, &_testLock);
    }
    pthread_mutex_unlock(&_testLock);
}
static void _signalTestComplete() {

    //Signal to the testing thread that this test is complete
    pthread_mutex_lock(&_testLock);
    _testComplete++;
    pthread_mutex_unlock(&_testLock);
    
    pthread_cond_signal(&_testCond);
}

// LES Tests
void _announceCallback (BREthereumLESAnnounceContext context,
                        BREthereumHash headHash,
                        uint64_t headNumber,
                        UInt256 headTotalDifficulty,
                        uint64_t reorgDepth) {
    
    eth_log("announcCallback_test", "%s", "received an announcement of a new chain");
}


//
//  Testing SendTx and SendTxV2 message
//
#define GAS_PRICE_20_GWEI       2000000000
#define GAS_PRICE_10_GWEI       1000000000
#define GAS_PRICE_5_GWEI         500000000
#define GAS_LIMIT_DEFAULT 21000

static void _transactionStatus(BREthereumLESTransactionStatusContext context,
                               BREthereumHash transaction,
                               BREthereumTransactionStatus status){
    
    printf("RECEIVED AN TRANSACTION STATUS");
}
void prepareLESTransaction (BREthereumLES les, const char *paperKey, const char *recvAddr, const uint64_t gasPrice, const uint64_t gasLimit, const uint64_t amount) {
    printf ("     Prepare Transaction\n");

    BREthereumClient client = {};
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
    BREthereumTransferId *tids = ethereumWalletGetTransfers(ewm, wallet);
    
    assert (NULL != tids && -1 != tids[0]);
    
    BREthereumTransferId tid = tids[0];
    assert (0 == strcmp (fromAddr, ethereumTransferGetSendAddress(ewm, tid)) &&
            0 == strcmp (recvAddr, ethereumTransferGetRecvAddress(ewm, tid)));
    
    BREthereumTransfer actualTransfer = ewmLookupTransfer(ewm, tid);
    BREthereumTransaction actualTransaction = transferGetOriginatingTransaction(actualTransfer);
    
    //Initilize testing state
    _initTest(1);
    
    assert(lesSubmitTransaction(les, NULL, _transactionStatus, actualTransaction) == LES_SUCCESS);
    
    sleep(600);
    
    free (fromAddr);
    ethereumDestroy(ewm);
}

extern void
reallySendLESTransaction(BREthereumLES les) {
    
    char paperKey[] = "biology just ridge kidney random donkey master employ poverty remind panel twenty";
    char recvAddress[] = "0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF";
    /*
     fputs("PaperKey: ", stdout);
     fgets (paperKey, 1024, stdin);
     paperKey[strlen(paperKey) - 1] = '\0';
     
     fputs("Address: ", stdout);
     fgets (recvAddress, 1024, stdin);
     recvAddress[strlen(recvAddress) - 1] = '\0';
     */
    printf ("PaperKey: '%s'\nAddress: '%s'\n", paperKey, recvAddress);
    
    // 0.001/2 ETH
    prepareLESTransaction(les, paperKey, recvAddress, GAS_PRICE_5_GWEI, GAS_LIMIT_DEFAULT, 1000000000000000000 / 1000 / 2);
}

//
//  Testing BlockHeaders message
//
static int _GetBlockHeaders_Context1 = 0;
static int _GetBlockHeaders_Context2 = 0;
static int _GetBlockHeaders_Context3 = 0;
static int _GetBlockHeaders_Context4 = 0;

typedef struct {
    BREthereumHash hash;
    uint64_t blockNum;
    UInt256 difficulty;
    uint64_t gasUsed;
    BREthereumHash parent;
    size_t transactionCount;
    size_t ommersCount;
}BlockHeaderTestData;
static BlockHeaderTestData _blockHeaderTestData[5];
#define BLOCK_4732522_IDX 2

static void _initBlockHeaderTestData(void){
    
    //Block Number: 4732524
    _blockHeaderTestData[0].hash = hashCreate("0x3a510c07862ebce419a14bfcd95620f924d188a935654c5ad0f4d5d7ee429193");
    _blockHeaderTestData[0].blockNum = 4732524;
    _blockHeaderTestData[0].difficulty = createUInt256(1645417372907632);
    _blockHeaderTestData[0].gasUsed = 7996865;
    _blockHeaderTestData[0].parent = hashCreate("0x5463afdad9eb343096a6a6561d4fed4b478380d02721cdd8fab97fda058f9fa2");
    _blockHeaderTestData[0].transactionCount = 331;
    _blockHeaderTestData[0].ommersCount = 0;
    
    //Block Number: 4732523
    _blockHeaderTestData[1].hash = hashCreate("0x5463afdad9eb343096a6a6561d4fed4b478380d02721cdd8fab97fda058f9fa2");
    _blockHeaderTestData[1].blockNum = 4732523;
    _blockHeaderTestData[1].difficulty = createUInt256(1645417372874864);
    _blockHeaderTestData[1].gasUsed = 7998505;
    _blockHeaderTestData[1].parent = hashCreate("0xb812a7b4a96c87a3d7d572847b3dee352b395cc9cfe3b6f0d163bc54e7d8a78e");
    _blockHeaderTestData[1].transactionCount = 193;
    _blockHeaderTestData[1].ommersCount = 0;
    
    //Block Number: 4732522
    _blockHeaderTestData[2].hash = hashCreate("0xb812a7b4a96c87a3d7d572847b3dee352b395cc9cfe3b6f0d163bc54e7d8a78e");
    _blockHeaderTestData[2].blockNum = 4732522;
    _blockHeaderTestData[2].difficulty = createUInt256(1646221191783396);
    _blockHeaderTestData[2].gasUsed = 8003540;
    _blockHeaderTestData[2].parent = hashCreate("0x4b29fb30276713be22786a9bdd548d787e9a2ea10248669f189b3f57f86ebaf8");
    _blockHeaderTestData[2].transactionCount = 186;
    _blockHeaderTestData[2].ommersCount = 0;
    
    
    //Block Number: 4732521
    _blockHeaderTestData[3].hash = hashCreate("0x4b29fb30276713be22786a9bdd548d787e9a2ea10248669f189b3f57f86ebaf8");
    _blockHeaderTestData[3].blockNum = 4732521;
    _blockHeaderTestData[3].difficulty = createUInt256(1647025403373368);
    _blockHeaderTestData[3].gasUsed = 7996801;
    _blockHeaderTestData[3].parent = hashCreate("0x4abb508954ec5f827184fb0d8bc74b104094d4060a06cc2dd743e4bfeaf1d8af");
    _blockHeaderTestData[3].transactionCount = 316;
    _blockHeaderTestData[3].ommersCount = 0;
    
    
    //Block Number: 4732520
    _blockHeaderTestData[4].hash = hashCreate("0x4abb508954ec5f827184fb0d8bc74b104094d4060a06cc2dd743e4bfeaf1d8af");
    _blockHeaderTestData[4].blockNum = 4732520;
    _blockHeaderTestData[4].difficulty = createUInt256(1647830007836613);
    _blockHeaderTestData[4].gasUsed = 7986707;
    _blockHeaderTestData[4].parent = hashCreate("0xe8f5d7cd81ad8ae3a677f6df6d87438ee5c98ead11f8df1b90b788f059a7deab");
    _blockHeaderTestData[4].transactionCount = 169;
    _blockHeaderTestData[4].ommersCount = 0;
}

static BREthereumBoolean _checkBlockHeader(BREthereumBlockHeader header,
                                           BREthereumHash expectedHash,
                                           uint64_t expectedBlockNumber,
                                           UInt256 expectedDifficulty,
                                           uint64_t expectedGasUsed,
                                           BREthereumHash expectedParenthash) {
    
    BREthereumHash gotHash = blockHeaderGetHash(header);
    uint64_t gotBlockNumber = blockHeaderGetNumber(header);
    uint64_t gotGasUsed = blockHeaderGetGasUsed(header);
    BREthereumHash gotParentHash = blockHeaderGetParentHash(header);
    
    
    return gotBlockNumber == expectedBlockNumber &&
    hashSetEqual(&gotHash, &expectedHash)  &&
    gotGasUsed == expectedGasUsed &&
    hashSetEqual(&gotParentHash, &expectedParenthash)
    ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
    
}
void _GetBlockHeaders_Calllback_Test4  (BREthereumLESBlockHeadersContext context,
                                        BREthereumBlockHeader header) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context4); //Check to make sure the context is correct
    
    BREthereumHash expectedHash = _blockHeaderTestData[_GetBlockHeaders_Context4].hash;
    uint64_t expectedBlockNumber =  _blockHeaderTestData[_GetBlockHeaders_Context4].blockNum;
    UInt256 expectedDifficulty = _blockHeaderTestData[_GetBlockHeaders_Context4].difficulty;
    uint64_t expectedGasUsed = _blockHeaderTestData[_GetBlockHeaders_Context4].gasUsed;
    BREthereumHash expectedParenthash = _blockHeaderTestData[_GetBlockHeaders_Context4].parent;
    
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_checkBlockHeader(header, expectedHash, expectedBlockNumber, expectedDifficulty, expectedGasUsed, expectedParenthash)));
    _GetBlockHeaders_Context4 += 2;
    
    if(_GetBlockHeaders_Context4 == 6){
        //Signal to the testing thread that this test completed successfully
        _signalTestComplete();
    }
}
void _GetBlockHeaders_Calllback_Test3  (BREthereumLESBlockHeadersContext context,
                                        BREthereumBlockHeader header) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context3); //Check to make sure the context is correct
    
    BREthereumHash expectedHash = _blockHeaderTestData[_GetBlockHeaders_Context3].hash;
    uint64_t expectedBlockNumber =  _blockHeaderTestData[_GetBlockHeaders_Context3].blockNum;
    UInt256 expectedDifficulty = _blockHeaderTestData[_GetBlockHeaders_Context3].difficulty;
    uint64_t expectedGasUsed = _blockHeaderTestData[_GetBlockHeaders_Context3].gasUsed;
    BREthereumHash expectedParenthash = _blockHeaderTestData[_GetBlockHeaders_Context3].parent;
    
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_checkBlockHeader(header, expectedHash, expectedBlockNumber, expectedDifficulty, expectedGasUsed, expectedParenthash)));
    _GetBlockHeaders_Context3 -= 2;
    
    if(_GetBlockHeaders_Context3 == -2){
        //Signal to the testing thread that this test completed successfully
        _signalTestComplete();
    }
}
void _GetBlockHeaders_Calllback_Test2  (BREthereumLESBlockHeadersContext context,
                                        BREthereumBlockHeader header) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context2); //Check to make sure the context is correct
    
    BREthereumHash expectedHash = _blockHeaderTestData[_GetBlockHeaders_Context2].hash;
    uint64_t expectedBlockNumber =  _blockHeaderTestData[_GetBlockHeaders_Context2].blockNum;
    UInt256 expectedDifficulty = _blockHeaderTestData[_GetBlockHeaders_Context2].difficulty;
    uint64_t expectedGasUsed = _blockHeaderTestData[_GetBlockHeaders_Context2].gasUsed;
    BREthereumHash expectedParenthash = _blockHeaderTestData[_GetBlockHeaders_Context2].parent;
    
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_checkBlockHeader(header, expectedHash, expectedBlockNumber, expectedDifficulty, expectedGasUsed, expectedParenthash)));
    _GetBlockHeaders_Context2++;
    
    if(_GetBlockHeaders_Context2 == 5){
        //Signal to the testing thread that this test completed successfully
        _signalTestComplete();
    }

}
void _GetBlockHeaders_Calllback_Test1  (BREthereumLESBlockHeadersContext context,
                                        BREthereumBlockHeader header) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context1); //Check to make sure the context is correct
    
    BREthereumHash expectedHash = _blockHeaderTestData[_GetBlockHeaders_Context1].hash;
    uint64_t expectedBlockNumber =  _blockHeaderTestData[_GetBlockHeaders_Context1].blockNum;
    UInt256 expectedDifficulty = _blockHeaderTestData[_GetBlockHeaders_Context1].difficulty;
    uint64_t expectedGasUsed = _blockHeaderTestData[_GetBlockHeaders_Context1].gasUsed;
    BREthereumHash expectedParenthash = _blockHeaderTestData[_GetBlockHeaders_Context1].parent;
    
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_checkBlockHeader(header, expectedHash, expectedBlockNumber, expectedDifficulty, expectedGasUsed, expectedParenthash)));
    _GetBlockHeaders_Context1--;
    
    if(_GetBlockHeaders_Context1 == -1){
        //Signal to the testing thread that this test completed successfully
        _signalTestComplete();
    }
}
static void run_GetBlockHeaders_Tests(BREthereumLES les){
    
     //Initialze test
     _initTest(4);
    
    //Request block headers 4732522, 4732523, 4732524
    _GetBlockHeaders_Context1 = BLOCK_4732522_IDX;
    assert(lesGetBlockHeaders(les, (void*)&_GetBlockHeaders_Context1, _GetBlockHeaders_Calllback_Test1, _blockHeaderTestData[BLOCK_4732522_IDX].blockNum, 3, 0, ETHEREUM_BOOLEAN_FALSE) == LES_SUCCESS);
    
    //Request block headers 4732522, 4732521, 4732520
    _GetBlockHeaders_Context2 = BLOCK_4732522_IDX;
    assert(lesGetBlockHeaders(les, (void*)&_GetBlockHeaders_Context2, _GetBlockHeaders_Calllback_Test2, _blockHeaderTestData[BLOCK_4732522_IDX].blockNum, 3, 0, ETHEREUM_BOOLEAN_TRUE) == LES_SUCCESS);
    
    //Request block headers 4732522, 4732524
    _GetBlockHeaders_Context3 = BLOCK_4732522_IDX;
    assert(lesGetBlockHeaders(les, (void*)&_GetBlockHeaders_Context3, _GetBlockHeaders_Calllback_Test3, _blockHeaderTestData[BLOCK_4732522_IDX].blockNum, 2, 1, ETHEREUM_BOOLEAN_FALSE) == LES_SUCCESS);
    
    //Request block headers 4732522, 4732520
    _GetBlockHeaders_Context4 = BLOCK_4732522_IDX;
    assert(lesGetBlockHeaders(les, (void*)&_GetBlockHeaders_Context4, _GetBlockHeaders_Calllback_Test4, _blockHeaderTestData[BLOCK_4732522_IDX].blockNum, 2, 1, ETHEREUM_BOOLEAN_TRUE) == LES_SUCCESS);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log("run_GetBlockHeaders_Tests", "%s", "Tests Successful");
}

//
// Testing GetTxtStatus message
//
static const int _GetTxStatus_Context1 = 1;
static const int _GetTxStatus_Context2 = 2;
static void _GetTxStatus_Test2_Callback(BREthereumLESTransactionStatusContext context,
                                        BREthereumHash transaction,
                                        BREthereumTransactionStatus status){
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetTxStatus_Context2); //Check to make sure the context is correct
    
    //Check to make sure we get back the right transaction
    BREthereumHash expectedTransactionHash1 = hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c");
    BREthereumHash expectedTransactionHash2 = hashCreate("0x78453edd2955e6ef6b200f5f9b98b3940d0d3f1528f902e7e855df56bf934cc5");
    
    assert(hashSetEqual(&transaction, &expectedTransactionHash1) ||
           hashSetEqual(&transaction, &expectedTransactionHash2));
    
    
    //Check to make sure the status is INCLUDED and the blockHash, blockNumber and txIndex is correct.
    // RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure.
    assert(status.type == TRANSACTION_STATUS_INCLUDED);
    
    BREthereumHash expectedBlockHash1 = hashCreate("0xf16becb908162df51c3789fab0e6ba52568fa7ee7d0127eb51bfaa0bcd40fb1b");
    BREthereumHash expectedBlockHash2 = hashCreate("0x0a4b16bac21b6dfeb51ccb522d8c34840844ae78ed0bc177670c501c18d35ff2");
    
    assert(hashSetEqual(&status.u.included.blockHash, &expectedBlockHash1) ||
           hashSetEqual(&status.u.included.blockHash, &expectedBlockHash2));
    
    uint64_t expectedBlockNumber1 = 5202375;
    uint64_t expectedBlockNumber2 = 5766700;
    
    assert(status.u.included.blockNumber == expectedBlockNumber1 ||
           status.u.included.blockNumber == expectedBlockNumber2 );
    
    uint64_t expectedTransactionIndex1 = 39;
    uint64_t expectedTransactionIndex2 = 36;
    
    assert(status.u.included.transactionIndex == expectedTransactionIndex1 ||
           status.u.included.transactionIndex == expectedTransactionIndex2 );
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}
static void _GetTxStatus_Test1_Callback(BREthereumLESTransactionStatusContext context,
                                        BREthereumHash transaction,
                                        BREthereumTransactionStatus status){
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetTxStatus_Context1); //Check to make sure the context is correct
    
    //Check to make sure we get back the right transaction
    BREthereumHash expectedTransactionHash = hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c");
    assert(hashSetEqual(&transaction, &expectedTransactionHash));
    
    
    //Check to make sure the status is INCLUDED and the blockHash, blockNumber and txIndex is correct.
    // RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure.
    assert(status.type == TRANSACTION_STATUS_INCLUDED);
    
    BREthereumHash expectedBlockHash = hashCreate("0xf16becb908162df51c3789fab0e6ba52568fa7ee7d0127eb51bfaa0bcd40fb1b");
    assert(hashSetEqual(&status.u.included.blockHash, &expectedBlockHash));
    
    uint64_t expectedBlockNumber = 5202375;
    assert(status.u.included.blockNumber == expectedBlockNumber);
    
    uint64_t expectedTransactionIndex = 39;
    assert(status.u.included.transactionIndex == expectedTransactionIndex);
    

    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}
static void run_GetTxStatus_Tests(BREthereumLES les){
    
    // Prepare values to be given to a send tranactions status message
    BREthereumHash transaction1Hash = hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c");
    BREthereumHash transaction2Hash = hashCreate("0x78453edd2955e6ef6b200f5f9b98b3940d0d3f1528f902e7e855df56bf934cc5");
    
    //Initilize testing state
    _initTest(2);
    
    assert(lesGetTransactionStatusOne(les, (void *)&_GetTxStatus_Context1, _GetTxStatus_Test1_Callback, transaction1Hash) == LES_SUCCESS);
    
    BREthereumHash* transactions;
    array_new(transactions, 2);
    array_add(transactions, transaction1Hash);
    array_add(transactions, transaction2Hash);
    
    assert(lesGetTransactionStatus(les, (void *)&_GetTxStatus_Context2, _GetTxStatus_Test2_Callback, transactions) == LES_SUCCESS);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log("run_GetTxStatus_Tests", "%s", "Tests Successful");
}
//
//  Testing BlockBodies message
//
static int _GetBlockBodies_Context1 = 0;
static int _GetBlockBodies_Context2 = 0;

static void _GetBlockBodies_Callback_Test1(BREthereumLESBlockBodiesContext context,
                                           BREthereumHash block,
                                           BREthereumTransaction transactions[],
                                           BREthereumBlockHeader ommers[]){
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetBlockBodies_Context1); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&block, &_blockHeaderTestData[_GetBlockBodies_Context1].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(transactions) == _blockHeaderTestData[_GetBlockBodies_Context1].transactionCount);
    assert(array_count(ommers) == _blockHeaderTestData[_GetBlockBodies_Context1].ommersCount);
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}
static void _GetBlockBodies_Callback_Test2(BREthereumLESBlockBodiesContext context,
                                           BREthereumHash block,
                                           BREthereumTransaction transactions[],
                                           BREthereumBlockHeader ommers[]){
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetBlockBodies_Context2); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&block, &_blockHeaderTestData[_GetBlockBodies_Context2].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(transactions) == _blockHeaderTestData[_GetBlockBodies_Context2].transactionCount);
    assert(array_count(ommers) == _blockHeaderTestData[_GetBlockBodies_Context2].ommersCount);
    
    _GetBlockBodies_Context2++;

    if(_GetBlockBodies_Context2 == 4) {
       //Signal to the testing thread that this test completed successfully
        _signalTestComplete();
    }
}

static void run_GetBlockBodies_Tests(BREthereumLES les){
    
    //Initilize testing state
    _initTest(2);
    
    //Request block bodies 4732522
    _GetBlockBodies_Context1 = BLOCK_4732522_IDX;
    assert(lesGetBlockBodiesOne(les, (void *)&_GetBlockBodies_Context1, _GetBlockBodies_Callback_Test1, _blockHeaderTestData[BLOCK_4732522_IDX].hash) == LES_SUCCESS);
    
    //Request block bodies 4732522, 4732521
    _GetBlockBodies_Context2 = BLOCK_4732522_IDX;
    BREthereumHash* blockHeaders;
    array_new(blockHeaders, 2);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX + 1].hash);
    
    assert(lesGetBlockBodies(les, (void *)&_GetBlockBodies_Context2, _GetBlockBodies_Callback_Test2, blockHeaders) == LES_SUCCESS);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log("run_GetBlockBlodies_Tests", "%s", "Tests Successful");
}

//
// Testing GetReceipts
//
static int _GetReceipts_Context1 = 0;
static int _GetReceipts_Context2 = 0;

static void _GetReceipts_Callback_Test1(BREthereumLESBlockBodiesContext context,
                                        BREthereumHash block,
                                        BREthereumTransactionReceipt receipts[]) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetReceipts_Context1); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&block, &_blockHeaderTestData[_GetReceipts_Context1].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(receipts) == _blockHeaderTestData[_GetReceipts_Context1].transactionCount);
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}
static void _GetReceipts_Callback_Test2(BREthereumLESBlockBodiesContext context,
                                        BREthereumHash block,
                                        BREthereumTransactionReceipt receipts[]) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetReceipts_Context2); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&block, &_blockHeaderTestData[_GetReceipts_Context2].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(receipts) == _blockHeaderTestData[_GetReceipts_Context2].transactionCount);
    
    _GetReceipts_Context2++;
    if(_GetReceipts_Context2 == 4){
        //Signal to the testing thread that this test completed successfully
        _signalTestComplete();
    }
}

static void run_GetReceipts_Tests(BREthereumLES les){
    
    //Initilize testing state
    _initTest(2);
    
    //Request receipts for block 4732522
    _GetReceipts_Context1 = BLOCK_4732522_IDX;
    assert(lesGetReceiptsOne(les, (void *)&_GetReceipts_Context1, _GetReceipts_Callback_Test1, _blockHeaderTestData[BLOCK_4732522_IDX].hash) == LES_SUCCESS);
    
    //Request receipts for block 4732522, 4732521
    _GetReceipts_Context2 = BLOCK_4732522_IDX;
    BREthereumHash* blockHeaders;
    array_new(blockHeaders, 2);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX + 1].hash);
    
    assert(lesGetReceipts(les, (void *)&_GetReceipts_Context2, _GetReceipts_Callback_Test2, blockHeaders) == LES_SUCCESS);
    
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log("run_GetReceipts_Tests", "%s", "Tests Successful");
    
}

// Test GetProofsV2
//
static int _GetProofsV2_Context1 = 0;

static void _GetProofs_Callback_Test1(BREthereumLESProofsV2Context context,
                                      BREthereumHash blockHash,
                                      BREthereumHash key,
                                      BREthereumHash key2) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetProofsV2_Context1); //Check to make sure the context is correct
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete(); 
}

static void run_GetProofsV2_Tests(BREthereumLES les){
    
    //Initilize testing state
    _initTest(1);
    
    //Address to get proofs from
    BREthereumAddress address = addressCreate("0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF");
    
    //Request the proofs for block 5503921 for address above
    BREthereumHash block_5503921 = hashCreate("0x089a6c0b4b960261287d30ee40b1eea2da2972e7189bd381137f55540d492b2c");
    BREthereumHash key, key2;
    BRKeccak256(key.bytes, address.bytes, sizeof(address.bytes));
    
    memset(key.bytes, 0, 32);
    memset(key2.bytes, 0, 32);
    
    assert(lesGetGetProofsV2One(les, (void *)&_GetProofsV2_Context1, _GetProofs_Callback_Test1, block_5503921, key, key2, 0) == LES_SUCCESS);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log("run_GetProofsV2_Tests", "%s", "Tests Successful");
}

//
// Test GetAccountState
//
static void _GetAccountState_Callback_Test1 (BREthereumLESAccountStateContext context,
                                             BREthereumLESAccountStateResult result) {
    assert (ACCOUNT_STATE_SUCCCESS == result.status);
    assert (result.u.success.accountState.nonce >= 0);
    _signalTestComplete();
}

static void run_GetAccountState_Tests (BREthereumLES les){
    //Initilize testing state
    _initTest(1);
    

    BREthereumAddress address = addressCreate("0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF");
    BREthereumHash block_5503921 = hashCreate("0x089a6c0b4b960261287d30ee40b1eea2da2972e7189bd381137f55540d492b2c");
    BREthereumLESAccountStateCallback context = NULL;

    lesGetAccountState(les, context, _GetAccountState_Callback_Test1, 5503921, block_5503921, address);

    _waitForTests();
    eth_log("run_GetAccopuntState_Tests", "%s", "Tests Successful");
}

void runLEStests(void) {
    
    //Prepare values to be given to a LES context
    BREthereumHash headHash;
    char headHashStr[] = "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3";
    assert(32 == (strlen(headHashStr)/2));
    decodeHex(headHash.bytes, 32, headHashStr, strlen(headHashStr));
    
    uint64_t headNumber = 0;
    UInt256 headTD = createUInt256 (0x400000000);
    
    BREthereumHash genesisHash;
    decodeHex(genesisHash.bytes, 32, headHashStr, strlen(headHashStr));
    
    // Create an LES context
    BREthereumLES les = lesCreate(ethereumMainnet, NULL, _announceCallback, headHash, headNumber, headTD, genesisHash);
    
    // Sleep for a little bit to allow the context to connect to the network
    sleep(5);
    
    //Initialize testing state
    _initTestState();
    
    //Initialize data needed for tests
    _initBlockHeaderTestData();
    
    // Run Tests on the LES messages
      run_GetTxStatus_Tests(les);
      run_GetBlockHeaders_Tests(les);
      run_GetBlockBodies_Tests(les);
      run_GetReceipts_Tests(les);
 //   run_GetProofsV2_Tests(les); //NOTE: The callback function won't be called.
      run_GetAccountState_Tests(les);
    //reallySendLESTransaction(les);
    
    printf ("Done\n");
}

