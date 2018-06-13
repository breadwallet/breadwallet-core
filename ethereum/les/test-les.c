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
#include "BREthereumHash.h"
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
#include "../lightnode/BREthereumLightNode.h"

// LES Tests
void _announceCallback (BREthereumLESAnnounceContext context,
                        BREthereumHash headHash,
                        uint64_t headNumber,
                        UInt256 headTotalDifficulty) {
    
    printf("RECEIVED AN ANNOUNCE MESSAGE!!!!!!\n");
}
void transactionStatusCallback(BREthereumLESTransactionStatusContext context,
                               BREthereumHash transaction,
                               BREthereumTransactionStatus status){
    
    char transactionHashStr[] = "c070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c";
    BREthereumHash transactionHash;
    decodeHex(transactionHash.bytes, 32, transactionHashStr, strlen(transactionHashStr));
    
    assert(memcmp(transaction.bytes, transactionHash.bytes, 32) == 0);
}
static void _transactionStatus(BREthereumLESTransactionStatusContext context,
                       BREthereumHash transaction,
                       BREthereumTransactionStatus status){
    
   printf("RECEIVED AN TRANSACTION STATUS");
}
void prepareLESTransaction (BREthereumLES les, const char *paperKey, const char *recvAddr, const uint64_t gasPrice, const uint64_t gasLimit, const uint64_t amount) {
    printf ("     Prepare Transaction\n");

    BREthereumLightNode node = ethereumCreate(ethereumMainnet, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN);
    // A wallet amount Ether
    BREthereumWalletId wallet = ethereumGetWallet(node);
    // END - One Time Code Block

    // Optional - will provide listNodeWalletCreateTransactionDetailed.
    ethereumWalletSetDefaultGasPrice(node, wallet, WEI, gasPrice);
    ethereumWalletSetDefaultGasLimit(node, wallet, gasLimit);

    BREthereumAmount amountAmountInEther =
    ethereumCreateEtherAmountUnit(node, amount, WEI);

    BREthereumTransactionId tx1 =
    ethereumWalletCreateTransaction
    (node,
     wallet,
     recvAddr,
     amountAmountInEther);

    ethereumWalletSignTransaction (node, wallet, tx1, paperKey);

    const char *rawTransactionHexEncoded =
    lightNodeGetTransactionRawDataHexEncoded(node, wallet, tx1, "0x");

    printf ("        Raw Transaction: %s\n", rawTransactionHexEncoded);

    char *fromAddr = ethereumGetAccountPrimaryAddress(node);
    BREthereumTransactionId *transactions = ethereumWalletGetTransactions(node, wallet);
    
    assert (NULL != transactions && -1 != transactions[0]);

    BREthereumTransactionId transaction = transactions[0];
    assert (0 == strcmp (fromAddr, ethereumTransactionGetSendAddress(node, transaction)) &&
            0 == strcmp (recvAddr, ethereumTransactionGetRecvAddress(node, transaction)));

    BREthereumTransaction actualTransaction = lightNodeLookupTransaction(node, transaction);

    assert(lesSubmitTransaction(les, NULL, _transactionStatus, TRANSACTION_RLP_SIGNED, actualTransaction) == LES_SUCCESS);
    
    sleep(600);
    
    free (fromAddr);
    ethereumDestroy(node);
}
#define GAS_PRICE_20_GWEI       2000000000
#define GAS_PRICE_10_GWEI       1000000000
#define GAS_PRICE_5_GWEI         500000000
#define GAS_LIMIT_DEFAULT 21000


// Local (PaperKey) -> LocalTest @ 5 GWEI gasPrice @ 21000 gasLimit & 0.0001/2 ETH
#define ACTUAL_RAW_TX "f86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260"
// eth.sendRawTran ('0xf86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260', function (err, hash) { if (!err) console.log(hash); });
extern void
reallySendLESTransaction() {

    //Prepare values to be given to an les context
    BREthereumHash headHash;
    char headHashStr[] = "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3";
    assert(32 == (strlen(headHashStr)/2));
    decodeHex(headHash.bytes, 32, headHashStr, strlen(headHashStr));

    uint64_t headNumber = 0;
    uint64_t headTD = 0x400000000;
    
    BREthereumHash genesisHash;
    decodeHex(genesisHash.bytes, 32, headHashStr, strlen(headHashStr));

    char paperKey[1024];
    char recvAddress[1024];

    fputs("PaperKey: ", stdout);
    fgets (paperKey, 1024, stdin);
    paperKey[strlen(paperKey) - 1] = '\0';

    fputs("Address: ", stdout);
    fgets (recvAddress, 1024, stdin);
    recvAddress[strlen(recvAddress) - 1] = '\0';

    printf ("PaperKey: '%s'\nAddress: '%s'\n", paperKey, recvAddress);

    // Create an LES context
    BREthereumLES les = lesCreate(ethereumMainnet, NULL, _announceCallback, headHash, headNumber, headTD, genesisHash);
    
    //
    sleep(5);

    // 0.001/2 ETH
    prepareLESTransaction(les, paperKey, recvAddress, GAS_PRICE_5_GWEI, GAS_LIMIT_DEFAULT, 1000000000000000000 / 1000 / 2);
}
void blockBodiesCallback  (BREthereumLESBlockBodiesContext context,
       BREthereumHash block,
       BREthereumTransaction transactions[],
       BREthereumBlockHeader ommers[]) {
    
    eth_log("BLOCK_BODIES", "%s", "Got a block body");
    
}

void receiptsCallback(BREthereumLESBlockBodiesContext context,
                      BREthereumHash block,
                      BREthereumTransactionReceipt receipts[]){
    
    eth_log("Receipts Callback", "%s", "Got a receipts array");
}

void runLESTest() {/*

    //Prepare values to be given to an les context
    BREthereumHash headHash;
    char headHashStr[] = "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3";
    assert(32 == (strlen(headHashStr)/2));
    decodeHex(headHash.bytes, 32, headHashStr, strlen(headHashStr));

    uint64_t headNumber = 0;
    uint64_t headTD = 0x400000000;
    
    BREthereumHash genesisHash;
    decodeHex(genesisHash.bytes, 32, headHashStr, strlen(headHashStr));

    // Create an LES context
    BREthereumLES les = lesCreate(ethereumMainnet, NULL, _announceCallback, headHash, headNumber, headTD, genesisHash);
  
    //Sleep for a bit to allow the les context to connect to the network
    sleep(5);
    
    // Prepare values to be given to a send tranactions status message
    char transactionHashStr[] = "c070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c";
   // char transactionHashStr[] = "78453edd2955e6ef6b200f5f9b98b3940d0d3f1528f902e7e855df56bf934cc5";
    assert(32 == (strlen(transactionHashStr)/2));
    BREthereumHash transactionHash;
    decodeHex(transactionHash.bytes, 32, transactionHashStr, strlen(transactionHashStr));
  
    lesGetBlockHeaders(les, NULL, headersCallback, 5732521, 10, 1, ETHEREUM_BOOLEAN_FALSE);
 
    BREthereumHash* blocks;
    array_new(blocks, 2);
    
    char hash1Str[] = "04e59a99b84a38a4d5d47ef22acdee3853bbcc9a4186f35256a7ace512f454f2";    //Block Number: 5732522
    assert(32 == (strlen(hash1Str)/2));
    BREthereumHash hash1;
    decodeHex(hash1.bytes, 32, hash1Str, strlen(hash1Str));
    
    char hash2Str[] = "089a6c0b4b960261287d30ee40b1eea2da2972e7189bd381137f55540d492b2c";   //Block Number: 5732521
    assert(32 == (strlen(hash2Str)/2));
    BREthereumHash hash2;
    decodeHex(hash2.bytes, 32, hash2Str, strlen(hash2Str));
    
      array_add(blocks, hash1);
      array_add(blocks, hash2);

    lesGetBlockBodies(les, NULL, blockBodiesCallback, blocks);
 
  //  lesGetReceipts(les,NULL,receiptsCallback,blocks); */
 
 /**************/
 
 
 //   assert(lesGetTransactionStatusOne(les, NULL, transactionStatusCallback, transactionHash) == LES_SUCCESS);
    
  /*  BREthereumAddress address = addressCreate("0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF");
    uint8_t hash[32];
    memcpy(hash,  address.bytes, 20);
    memset(&hash[20], 0, 32 - 20);
    
    //BRKeccak256(hash, address.bytes, 20);
    
    BREthereumHash key, key2;
    memcpy(key.bytes, hash, 32);
    memset(key2.bytes, 0, 32);
    //lesGetGetProofsV2One(les,hash1,key,key2, 0); */
    
    //Sleep to allow for the results to get back from the program. 
    sleep(600);
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
        eth_log("run_GetBlockHeaders_Tests", "%s", "Test 4 Successful");
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
        eth_log("run_GetBlockHeaders_Tests", "%s", "Test 3 Successful");
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
        eth_log("run_GetBlockHeaders_Tests", "%s", "Test 2 Successful");
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
        eth_log("run_GetBlockHeaders_Tests", "%s", "Test 1 Successful");
    }
}
static void run_GetBlockHeaders_Tests(BREthereumLES les){
    
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
    
    //Wait for a little bit to get a reply back from the server.
    sleep(60);
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

     eth_log("run_GetTxStatus_Tests", "%s", "Test 2 Successful");
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

     eth_log("run_GetTxStatus_Tests", "%s", "Test 1 Successful");
}

static void run_GetTxStatus_Tests(BREthereumLES les){

    // Prepare values to be given to a send tranactions status message
    BREthereumHash transaction1Hash = hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c");
    BREthereumHash transaction2Hash = hashCreate("0x78453edd2955e6ef6b200f5f9b98b3940d0d3f1528f902e7e855df56bf934cc5");

    assert(lesGetTransactionStatusOne(les, (void *)&_GetTxStatus_Context1, _GetTxStatus_Test1_Callback, transaction1Hash) == LES_SUCCESS);

    BREthereumHash* transactions;
    array_new(transactions, 2);
    array_add(transactions, transaction1Hash);
    array_add(transactions, transaction2Hash);
    
    assert(lesGetTransactionStatus(les, (void *)&_GetTxStatus_Context2, _GetTxStatus_Test2_Callback, transactions) == LES_SUCCESS);
    
    //Wait for a little bit to get a reply back from the server.
    sleep(300);
    
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
    
    assert(context1 == &_GetBlockBodies_Context1); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&block, &_blockHeaderTestData[_GetBlockBodies_Context1].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(transactions) == _blockHeaderTestData[_GetBlockBodies_Context1].transactionCount);
    assert(array_count(ommers) == _blockHeaderTestData[_GetBlockBodies_Context1].ommersCount);
    
    eth_log("run_GetBlockBodies_Tests", "%s", "Test 1 Successful");
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
    
    if(_GetBlockBodies_Context2 == 4){
        eth_log("run_GetBlockBodies_Tests", "%s", "Test 2 Successful");
    }
}

static void run_GetBlockBodies_Tests(BREthereumLES les){

    //Request block bodies 4732522
    _GetBlockBodies_Context1 = BLOCK_4732522_IDX;
    assert(lesGetBlockBodiesOne(les, (void *)&_GetBlockBodies_Context1, _GetBlockBodies_Callback_Test1, _blockHeaderTestData[BLOCK_4732522_IDX].hash) == LES_SUCCESS);
   
    //Request block bodies 4732522, 4732521
    _GetBlockBodies_Context2 = BLOCK_4732522_IDX;
    BREthereumHash* blockHeaders;
    array_new(blockHeaders, 1);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX + 1].hash);
    
    assert(lesGetBlockBodies(les, (void *)&_GetBlockBodies_Context2, _GetBlockBodies_Callback_Test2, blockHeaders) == LES_SUCCESS);
    
    //Wait for a little bit to get a reply back from the server.
    sleep(60);
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
    
    eth_log("run_GetReceipts_Tests", "%s", "Test 1 Successful");
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
    if(_GetReceipts_Context2 == 3){
        eth_log("run_GetReceipts_Tests", "%s", "Test 2 Successful");
    }
}

static void run_GetReceipts_Tests(BREthereumLES les){

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
    
    
    //Wait for a little bit to get a reply back from the server.
    sleep(60);
}

//
// Test GetProofsV2
//
static int _GetProofsV2_Context1 = 0;

static void run_GetProofsV2_Tests(BREthereumLES les){
/*
    BREthereumAddress address = addressCreate("0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF");
    uint8_t hash[32];
    memcpy(hash, address.bytes, 20);
    memset(&hash[20], 0, 32 - 20);
    
    //BRKeccak256(hash, address.bytes, 20);
    
    BREthereumHash key, key2;
    memcpy(key.bytes, hash, 32);
    memset(key2.bytes, 0, 32);
    
    //Request the proofs for block 4732522
    _GetProofsV2_Context1 = BLOCK_4732522_IDX
    
    
    lesGetGetProofsV2One(les,hash1,key,key2, 0)
    
    assert(lesGetReceiptsOne(les, (void *)&_GetReceipts_Context1, _GetReceipts_Callback_Test1, _blockHeaderTestData[BLOCK_4732522_IDX].hash) == LES_SUCCESS);
*/ 
    
    //Wait for a little bit to get a reply back from the server.
    sleep(60);
}

void runLEStests(void) {

    //Prepare values to be given to a LES context
    BREthereumHash headHash;
    char headHashStr[] = "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3";
    assert(32 == (strlen(headHashStr)/2));
    decodeHex(headHash.bytes, 32, headHashStr, strlen(headHashStr));

    uint64_t headNumber = 0;
    uint64_t headTD = 0x400000000;
    
    BREthereumHash genesisHash;
    decodeHex(genesisHash.bytes, 32, headHashStr, strlen(headHashStr));

    // Create an LES context
    BREthereumLES les = lesCreate(ethereumMainnet, NULL, _announceCallback, headHash, headNumber, headTD, genesisHash);

    // Sleep for a little bit to allow the context to connect to the network
    sleep(3);
    
    //Initialize data needed for tests
    _initBlockHeaderTestData();
    
    
    //Run Tests on the LES messages
      run_GetTxStatus_Tests(les);
 //      run_GetBlockHeaders_Tests(les);
  //   run_GetBlockBodies_Tests(les);
  //   run_GetReceipts_Tests(les);
  //   run_GetProofsV2_Tests(les);
    
}

