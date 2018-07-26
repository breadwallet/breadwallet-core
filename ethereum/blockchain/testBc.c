//
//  testBc.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "BREthereumBlockChain.h"

//
// Bloom Test
//
#define BLOOM_ADDR_1 "095e7baea6a6c7c4c2dfeb977efac326af552d87"
#define BLOOM_ADDR_2 "0000000000000000000000000000000000000000"  // topic
#define BLOOM_ADDR_1_OR_2_RESULT "00000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000000000000800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000040000000000000000000000000000000000000000000000000000000"
extern void
runBloomTests (void) {
    printf ("==== Bloom\n");

    BREthereumBloomFilter filter1 = bloomFilterCreateAddress(addressCreate(BLOOM_ADDR_1));
    BREthereumBloomFilter filter2 = logTopicGetBloomFilterAddress(addressCreate(BLOOM_ADDR_2));

    BREthereumBloomFilter filter = bloomFilterOr(filter1, filter2);
    char *filterAsString = bloomFilterAsString(filter);
    assert (0 == strcmp (filterAsString, BLOOM_ADDR_1_OR_2_RESULT));

    BREthereumBloomFilter filterx = bloomFilterCreateString(BLOOM_ADDR_1_OR_2_RESULT);
    assert (ETHEREUM_BOOLEAN_IS_TRUE(bloomFilterEqual(filter, filterx)));

    assert (ETHEREUM_BOOLEAN_IS_TRUE(bloomFilterMatch(filter, filter1)));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(bloomFilterMatch(filter, filter2)));
    assert (ETHEREUM_BOOLEAN_IS_FALSE(bloomFilterMatch(filter, bloomFilterCreateAddress(addressCreate("195e7baea6a6c7c4c2dfeb977efac326af552d87")))));

}


//
// block Test
//
// We used to have a block test based on Ethereum-Java test cases - but those tests included a
// 'SeedHash' which we don't get from our GETH ewm.  We axed that test:

//// The following block differs from the above.  We are doing a decode/encode - the decode will
//// recognize a pre-EIP-155 RLP sequence (so we can recover the address from the signature); the
//// encode always encodes with EIP-155 - thus the `signature.v` value will differ - the difference
//// is 10 if using mainnet.  So we found the two characters in the transactions signature encoding
//// an changed them from 0x1b 27) to 0x2a (37)
//
//#define BLOCK_1_RLP "f90286f9021aa0afa4726a3d669141a00e70b2bf07f313abbb65140ca045803d4f0ef8dc426274a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000a02debf71e4cc78eaacdd660ebc93f641c09fc19a49caf6b159171f5ba9d4928cba05259d6e3b135d378cc542b5f5b9587861e965e8efa156948f161a0fba6adf47da0f315ec0a9ad4f2db3303623360931df1d08bfdd9e0bd4cbbc2d64b5b1de4304bb901000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000080000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000004000000000000000000000000000000000000000000000000000000083020000018301e84882560b8454f835be42a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a0c7f14dcfe22dd19b5e00e1827dea4525f9a7d1cbe319520b59f6875cfcf839c2880a6f958326b74cc5f866f864800a82c35094095e7baea6a6c7c4c2dfeb977efac326af552d8785012a05f2008025a042b9d6700235542229ba4942f6c7f975a50515be4d1f092cba4a98730300529ca0f7def4fa32fb4cac965f111ff02c56362ab140c90c3b62361cb65526ba6dcc3dc0"
//#define BLOCK_HEADER_1_RLP "f9021aa0afa4726a3d669141a00e70b2bf07f313abbb65140ca045803d4f0ef8dc426274a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000a02debf71e4cc78eaacdd660ebc93f641c09fc19a49caf6b159171f5ba9d4928cba05259d6e3b135d378cc542b5f5b9587861e965e8efa156948f161a0fba6adf47da0f315ec0a9ad4f2db3303623360931df1d08bfdd9e0bd4cbbc2d64b5b1de4304bb901000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000080000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000004000000000000000000000000000000000000000000000000000000083020000018301e84882560b8454f835be42a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a0c7f14dcfe22dd19b5e00e1827dea4525f9a7d1cbe319520b59f6875cfcf839c2880a6f958326b74cc5"                                                                     // 1b -> 25 (27 + 10 => chainID EIP-155)
//// 0xf90286 f9021a a0afa4726a3d669141a00e70b2bf07f313abbb65140ca045803d4f0ef8dc426274 a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347 940000000000000000000000000000000000000000 a02debf71e4cc78eaacdd660ebc93f641c09fc19a49caf6b159171f5ba9d4928cb a05259d6e3b135d378cc542b5f5b9587861e965e8efa156948f161a0fba6adf47d a0f315ec0a9ad4f2db3303623360931df1d08bfdd9e0bd4cbbc2d64b5b1de4304b b9010000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000000000000800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000040000000000000000000000000000000000000000000000000000000 83,020000 01 83,01e848 82,560b 84,54f835be 42 a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421 a0c7f14dcfe22dd19b5e00e1827dea4525f9a7d1cbe319520b59f6875cfcf839c2 88,0a6f958326b74cc5 f866f864800a82c35094095e7baea6a6c7c4c2dfeb977efac326af552d8785012a05f200801ba042b9d6700235542229ba4942f6c7f975a50515be4d1f092cba4a98730300529ca0f7def4fa32fb4cac965f111ff02c56362ab140c90c3b62361cb65526ba6dcc3dc0
////  Block   Header:           parentHash                                                ommersHash                                                          beneficiary                                 stateRoot                                                       transactionRoot                                                     receiptsRoot                                                        logsBloom                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           Diff     Num    gLimit  gUsed   Timestamp  Extra    seedHash??                                                      mixHash                                                             nonce                   <transactions>

// Now we have a test based on some 'genesis rlp' encoding... but it doesn't exactly match our
// genesis block AND it has a strange encoding for the nonce as 0x88000000000000002a but our
// RLP encode/decode produces just an 'immediate' of '2a'.
//
// So, we neuter some tests.
//
//  Block   Header:           parentHash                                                ommersHash                                                          beneficiary                                 stateRoot                                                       transactionRoot                                                     receiptsRoot                                                        logsBloom                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           Diff     Num    gLimit  gUsed   Timestamp  Extra    seedHash??                                                      mixHash                                                             nonce                   <transactions>   <ommers>
// 0xf901f8 f901f3 a00000000000000000000000000000000000000000000000000000000000000000 a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347 940000000000000000000000000000000000000000 a09178d0f23c965d81f0834a4c72c6253ce6830f4022b1359aaebfc1ecba442d4e a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421 a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421 b9010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 83,020000 80   83,2fefd8 80      80          80                                                                      a00000000000000000000000000000000000000000000000000000000000000000 88000000000000002a       c0              c0
#define GENESIS_RLP "f901f8f901f3a00000000000000000000000000000000000000000000000000000000000000000a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000a09178d0f23c965d81f0834a4c72c6253ce6830f4022b1359aaebfc1ecba442d4ea056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302000080832fefd8808080a0000000000000000000000000000000000000000000000000000000000000000088000000000000002ac0c0"

extern void
runBlockTest0 (void) {
    printf ("==== Block\n");

    BRRlpData data;
//    int typeMismatch;

    //
    // Block
    //
    data.bytes = decodeHexCreate(&data.bytesCount, GENESIS_RLP, strlen (GENESIS_RLP));

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem blockItem = rlpGetItem(coder, data);

    BREthereumBlock block = blockRlpDecode(blockItem, ethereumMainnet, RLP_TYPE_NETWORK, coder);

    BREthereumBlockHeader header = blockGetHeader(block);
    BREthereumBlockHeader genesis = networkGetGenesisBlockHeader (ethereumMainnet);

    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(blockHeaderGetParentHash(header), blockHeaderGetParentHash(genesis))));
    assert(blockHeaderGetTimestamp(header) == blockHeaderGetTimestamp(genesis));
    assert(blockHeaderGetNumber(header) == blockHeaderGetNumber(genesis));
    //    assert(blockHeaderGetGasUsed(header) == blockHeaderGetGasUsed(genesis));
    //    assert(blockHeaderGetDifficulty(header) == blockHeaderGetDifficulty(genesis));
    //    assert(blockHeaderGetNonce(header) == blockHeaderGetNonce(genesis));

    /*
     assert (1 == blockGetTransactionsCount(block));
     BREthereumTransaction transaction = blockGetTransaction (block, 0);
     assert (0 == transactionGetNonce(transaction));
     assert (0 == strcmp ("0x", transactionGetData(transaction)));
     assert (ETHEREUM_COMPARISON_EQ == gasCompare(transactionGetGasLimit(transaction), gasCreate(50000)));
     assert (ETHEREUM_COMPARISON_EQ == gasPriceCompare(transactionGetGasPrice(transaction), gasPriceCreate(etherCreateNumber(10, WEI))));
     assert (ETHEREUM_COMPARISON_EQ == amountCompare(transactionGetAmount(transaction),
     amountCreateEther(etherCreateNumber(5000000000, WEI)),
     &typeMismatch));
     */
    assert (0 == blockGetOmmersCount(block));
    assert (0 == blockGetTransactionsCount(block));

    blockItem = blockRlpEncode(block, ethereumMainnet, RLP_TYPE_NETWORK, coder);
    rlpShowItem(coder, blockItem, "BlockTest");
    rlpDataRelease(data);
    blockRelease(block);
    blockHeaderRelease (genesis);
    rlpCoderRelease(coder);
}

/*
 * What does the Block's LogBloomFilter hold?  Source, Target, Contract?
 */
#define BLOCK_1_NUMBER 0x50FCD2
#define BLOCK_1_BLOOM "0006b02423400010043a0d004a40cf191040184040501110ea204d020268010ca04444107c040b381880d24c166084a302201a4020819062161862318029804e050844c139320cb1815c4c1e25082202660300c8542904084021b9020104228401a228216e60128cd04840a04a480c4145400620080740004820a09002a0e0c46b20a8818195067428080c0494948016018100a0031185b108021304002030140e47800908401c82962040304448e606315294a92009902128108221114480114882250a01ae800212c29260021d4002e60ba0e55a88a88809b8102c2280a8d480d491ccc0468400c04900cd0400e29d10a0007329302819832c140412801588"
#define BLOCK_1_HASH "19f2c349b015df2d509d849af885f6f95dfe6713ac907311e5362b41ef50273a"
#define BLOCK_1_TX_132 "0xe530827e823590dd3f227f43c8b3a9fbd029842bdc39df44bd91ee7de489601a"
#define BLOCK_1_TX_132_SOURCE "0xc587d7c282bc85bcfd21af63883fa7319dbcff68"
#define BLOCK_1_TX_132_TARGET "0xb18c54a48f698199bca52dc68e58cd0fcd6cbdd2"
#define BLOCK_1_TX_132_TOKENC "0x358d12436080a01a16f711014610f8a4c2c2d233"

#define BLOCK_2_NUMBER 0x5778A9
#define BLOCK_2_BLOOM "0x00a61039e24688a200002e10102021116002220040204048308206009928412802041520200201115014888000c00080020a002021308850c60d020d00200188062900c83288401115821a1c101200d00318080088df000830c1938002a018040420002a22201000680a391c91610e4884682a00910446003da000b9501020009c008205091c0b04108c000410608061a07042141001820440d404042002a4234f00090845c1544820140430552592100352140400039000108e052110088800000340422064301701c8212008820c4648a020a482e90a0268480000400021800110414680020205002400808012c6248120027c4121119802240010a2181983"
#define BLOCK_2_HASH "0x0a89dd55d38929468c1303b92ab43ca57269ac864175fc6208ae739ffcc17c9b"
#define BLOCK_2_TX_53 "0x3063e073c0b90693639fad94258797baf39c1e2b2a6e56b2e85010e5c963f3b3"
#define BLOCK_2_TX_53_SOURCE "0x49f4c50d9bcc7afdbcf77e0d6e364c29d5a660df"
#define BLOCK_2_TX_53_TARGET "0xb0f225defec7625c6b5e43126bdde398bd90ef62"
#define BLOCK_2_TX_53_TOKENC "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"


extern void
runBlockTest1 () {
    // BLOCK_1
    {
        BREthereumBloomFilter filter = bloomFilterCreateString(BLOCK_1_BLOOM);

        BREthereumAddress addressSource = addressCreate(BLOCK_1_TX_132_SOURCE);
        BREthereumAddress addressTarget = addressCreate(BLOCK_1_TX_132_TARGET);
        BREthereumAddress addressTokenC = addressCreate(BLOCK_1_TX_132_TOKENC);

        // 'LogsBloom' holds contact and topic info ...
        assert (ETHEREUM_BOOLEAN_IS_TRUE (bloomFilterMatch(filter, bloomFilterCreateAddress(addressTokenC))));
        assert (ETHEREUM_BOOLEAN_IS_TRUE (bloomFilterMatch(filter, logTopicGetBloomFilterAddress(addressSource))));
        assert (ETHEREUM_BOOLEAN_IS_TRUE (bloomFilterMatch(filter, logTopicGetBloomFilterAddress(addressTarget))));

        // It does not contain transaction address info.
        assert (ETHEREUM_BOOLEAN_IS_FALSE (bloomFilterMatch(filter, bloomFilterCreateAddress(addressSource))));
        assert (ETHEREUM_BOOLEAN_IS_FALSE (bloomFilterMatch(filter, bloomFilterCreateAddress(addressTarget))));
    }

    // BLOCK_2
    {
        BREthereumBloomFilter filter = bloomFilterCreateString(BLOCK_2_BLOOM);

        BREthereumAddress addressSource = addressCreate(BLOCK_2_TX_53_SOURCE);
        BREthereumAddress addressTarget = addressCreate(BLOCK_2_TX_53_TARGET);
        BREthereumAddress addressTokenC = addressCreate(BLOCK_2_TX_53_TOKENC);

        // 'LogsBloom' holds contact and topic info ...
        assert (ETHEREUM_BOOLEAN_IS_TRUE (bloomFilterMatch(filter, bloomFilterCreateAddress(addressTokenC))));
        assert (ETHEREUM_BOOLEAN_IS_TRUE (bloomFilterMatch(filter, logTopicGetBloomFilterAddress(addressSource))));
        assert (ETHEREUM_BOOLEAN_IS_TRUE (bloomFilterMatch(filter, logTopicGetBloomFilterAddress(addressTarget))));

        // It does not contain transaction address info.
        assert (ETHEREUM_BOOLEAN_IS_FALSE (bloomFilterMatch(filter, bloomFilterCreateAddress(addressSource))));
        assert (ETHEREUM_BOOLEAN_IS_FALSE (bloomFilterMatch(filter, bloomFilterCreateAddress(addressTarget))));
    }
}

static void
runBlockCheckpointTest (void) {
    const BREthereumBlockCheckpoint *cp1;
    BREthereumBlockHeader hd1;

    cp1 = blockCheckpointLookupLatest(ethereumMainnet);
    //    assert (5750000 == cp1->number);

    hd1 = blockCheckpointCreatePartialBlockHeader(cp1);
    assert (cp1->number == blockHeaderGetNumber(hd1));
    assert (cp1->timestamp == blockHeaderGetTimestamp(hd1));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(cp1->hash, blockHeaderGetHash(hd1))));
    blockHeaderRelease(hd1);
}

/*  Ehtereum Java
 byte[] rlp = Hex.decode("f85a94d5ccd26ba09ce1d85148b5081fa3ed77949417bef842a0000000000000000000000000459d3a7595df9eba241365f4676803586d7d199ca0436f696e7300000000000000000000000000000000000000000000000000000080");
 LogInfo logInfo = new LogInfo(rlp);

 assertEquals("d5ccd26ba09ce1d85148b5081fa3ed77949417be",
 Hex.toHexString(logInfo.getAddress()));
 assertEquals("", Hex.toHexString(logInfo.getData()));

 assertEquals("000000000000000000000000459d3a7595df9eba241365f4676803586d7d199c",
 logInfo.getTopics().get(0).toString());
 assertEquals("436f696e73000000000000000000000000000000000000000000000000000000",
 logInfo.getTopics().get(1).toString());

 */

#define LOG_1_RLP "f85a94d5ccd26ba09ce1d85148b5081fa3ed77949417bef842a0000000000000000000000000459d3a7595df9eba241365f4676803586d7d199ca0436f696e7300000000000000000000000000000000000000000000000000000080"
#define LOG_1_ADDRESS "d5ccd26ba09ce1d85148b5081fa3ed77949417be"
#define LOG_1_TOPIC_0 "000000000000000000000000459d3a7595df9eba241365f4676803586d7d199c"
#define LOG_1_TOPIC_1 "436f696e73000000000000000000000000000000000000000000000000000000"

extern void
runLogTests (void) {
    printf ("==== Log\n");

    BRRlpData data;
    BRRlpData encodeData;

    // Log
    data.bytes = decodeHexCreate(&data.bytesCount, LOG_1_RLP, strlen (LOG_1_RLP));

    BRRlpCoder coder  = rlpCoderCreate();
    BRRlpItem logItem = rlpGetItem(coder, data);
    BREthereumLog log = logRlpDecode(logItem, RLP_TYPE_NETWORK, coder);

    BREthereumAddress address = logGetAddress(log);
    size_t addressBytesCount;
    uint8_t *addressBytes = decodeHexCreate(&addressBytesCount, LOG_1_ADDRESS, strlen(LOG_1_ADDRESS));
    assert (addressBytesCount == sizeof (address.bytes));
    assert (0 == memcmp (address.bytes, addressBytes, addressBytesCount));
    free (addressBytes);

    // topic-0
    // topic-1


    logItem = logRlpEncode(log, RLP_TYPE_NETWORK, coder);
    rlpDataExtract(coder, logItem, &encodeData.bytes, &encodeData.bytesCount);

    assert (data.bytesCount == encodeData.bytesCount
            && 0 == memcmp (data.bytes, encodeData.bytes, encodeData.bytesCount));

    rlpShow(data, "LogTest");

    // Archive
    BREthereumHash someBlockHash = HASH_INIT("fc45a8c5ebb5f920931e3d5f48992f3a89b544b4e21dc2c11c5bf8165a7245d6");
    uint64_t someBlockNumber = 11592;

    BREthereumHash someTxHash = HASH_INIT("aa2703c3ae5d0024b2c3ab77e5200bb2a8eb39a140fad01e89a495d73760297c");
    uint64_t someTxIndex = 108;

    logInitializeIdentifier(log, someTxHash, someTxIndex);

    logSetStatus(log, transactionStatusCreateIncluded(gasCreate(0), someBlockHash, someBlockNumber, 0));
    BREthereumTransactionStatus status = logGetStatus(log);

    BRRlpItem item = logRlpEncode(log, RLP_TYPE_ARCHIVE, coder);
    BREthereumLog logArchived = logRlpDecode(item, RLP_TYPE_ARCHIVE, coder);
    BREthereumTransactionStatus statusArchived = logGetStatus(logArchived);

    assert (status.type == statusArchived.type);

    BREthereumHash statusArchiveTxHash;
    size_t statusArchiveIndex;
    logExtractIdentifier(logArchived, &statusArchiveTxHash, &statusArchiveIndex);
//    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(status.identifier.transactionHash, statusArchiveTxHash)));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual (someTxHash, statusArchiveTxHash)));

//    assert (status.identifier.transactionReceiptIndex == statusArchiveIndex);
    assert (someTxIndex == statusArchiveIndex);

    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(status.u.included.blockHash, statusArchived.u.included.blockHash)));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(someBlockHash, statusArchived.u.included.blockHash)));

    assert (status.u.included.blockNumber == statusArchived.u.included.blockNumber);
    assert (someBlockNumber = statusArchived.u.included.blockNumber);

    rlpDataRelease(encodeData);
    rlpDataRelease(data);
    rlpCoderRelease(coder);
}

//
// Acount State
//
#define ACCOUNT_STATE_NONCE  1234
#define ACCOUNT_STATE_BALANCE   5000000000

extern BREthereumAccountState
accountStateCreate (uint64_t nonce,
                    BREthereumEther balance,
                    BREthereumHash storageRoot,
                    BREthereumHash codeHash);

static BREthereumHash emptyHash;
extern void
runAccountStateTests (void) {
    printf ("==== Account State\n");

    BREthereumAccountState state = accountStateCreate(ACCOUNT_STATE_NONCE,
                                                      etherCreateNumber(ACCOUNT_STATE_BALANCE, WEI),
                                                      emptyHash,
                                                      emptyHash);
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem encoding = accountStateRlpEncode(state, coder);

    BREthereumAccountState decodedState = accountStateRlpDecode(encoding, coder);

    assert (accountStateGetNonce(state) == accountStateGetNonce(decodedState));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(etherIsEQ(accountStateGetBalance(state),
                                               accountStateGetBalance(decodedState))));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(accountStateGetStorageRoot(state),
                                               accountStateGetStorageRoot(decodedState))));

    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(accountStateGetCodeHash(state),
                                               accountStateGetCodeHash(decodedState))));


    rlpCoderRelease(coder);
}

//
// Transaction Status
//
extern void
runTransactionStatusTests (void) {
    printf ("==== Transaction Status\n");
//    BREthereumTransactionStatus status;

    // We only decode... and we have no RLP do decode yet.
}

//
// Transaction Receipt
//

// From Ethereum Java - a 'six item' RLP Encoding - but expecting only 'four items'.  Java
// apparently tests w/ 'six' because the encoding includes additiona element for serialization.
#define RECEIPT_1_RLP "f88aa0966265cc49fa1f10f0445f035258d116563931022a3570a640af5d73a214a8da822b6fb84000000010000000010000000000008000000000000000000000000000000000000000000000000000000000020000000000000014000000000400000000000440d8d7948513d39a34a1a8570c9c9f0af2cba79ac34e0ac8c0808301e24086873423437898"

extern void
runTransactionReceiptTests (void) {
    printf ("==== Transaction Receipt\n");
//    BRRlpData data;
//    BRRlpData encodeData;

    /*
     // Log
     data.bytes = decodeHexCreate(&data.bytesCount, RECEIPT_1_RLP, strlen (RECEIPT_1_RLP));

     BREthereumTransactionReceipt receipt = transactionReceiptDecodeRLP(data);
     // assert

     encodeData = transactionReceiptEncodeRLP(receipt);
     assert (data.bytesCount == encodeData.bytesCount
     && 0 == memcmp (data.bytes, encodeData.bytes, encodeData.bytesCount));

     rlpDataRelease(encodeData);
     rlpDataRelease(data);
     */
}


static void
runBlockTests (void) {
    runBlockTest0();
    runBlockTest1();
    runBlockCheckpointTest ();
}

extern void
runBcTests (void) {
    runBloomTests();
    runBlockTests();
    runLogTests();
    runAccountStateTests();
    runTransactionStatusTests();
    runTransactionReceiptTests();
}

