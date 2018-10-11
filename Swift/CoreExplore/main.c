//
//  main.c
//  CoreExplore
//
//  Created by Ed Gamble on 8/25/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>
#include "BREthereum.h"
#include "BRRlp.h"
#include "BRUtil.h"
#include "BREthereumBlockChain.h"

#define TEST_TRANS_ETH      "0xf86a75843b9aca00825208943d7eefb552b7d633e7f9eb48cd82cd098ecd5b4687038d7ea4c68000802aa045827725970e3c9729c9450b3ff04f98f10e231ebdeec5d522585a9a57bab1b4a025547970f9bedbef17d4aadd38f4263955633507689a9d7598c9e9bc38438d03"
#define TEST_TRANS_BRD      "0xf8a976841dcd65008301676094722dd3f80bac40c951b51bdd28dd19d43576218080b844a9059cbb0000000000000000000000003d7eefb552b7d633e7f9eb48cd82cd098ecd5b46000000000000000000000000000000000000000000000000000000e8d4a5100029a02604f887d60d438d29c73b69ade7208ced970d5c74b1bf5b2f156e56c785f15da03b56daa107f678fee099347af966093081e3ef87dc6040a1ce0113452e37f664"
#define TEST_TRANS          TEST_TRANS_ETH

static void
handleTrans (BRRlpCoder coder, const char *input) {
    BRRlpData data;
    BRRlpItem item;

    // Strip a leading "0x"
    if (0 == strncmp (input, "0x", 2))
        input = &input[2];

    // Fill `data` and `item`
    data.bytes = decodeHexCreate(&data.bytesCount, input, strlen (input));
    item = rlpGetItem (coder, data);
    rlpShow(data, "Trans:");

    // Extract a transaction
    BREthereumTransaction transaction = transactionRlpDecode (item, ethereumTestnet, RLP_TYPE_TRANSACTION_SIGNED, coder);

    transactionShow(transaction, "EXP");
    eth_log ("EXP", "    Raw   : %s", input);
}

#define SOME_RLP TEST_TRANS

static void
handleRLP (BRRlpCoder coder, const char *input) {
    BRRlpData data;
    BRRlpItem item;

    // Strip a leading "0x"
    if (0 == strncmp (input, "0x", 2))
        input = &input[2];

    // Fill `data` and `item`
    data.bytes = decodeHexCreate(&data.bytesCount, input, strlen (input));
    item = rlpGetItem (coder, data);
    rlpShowItem (coder, item, "RLP");
    rlpReleaseItem (coder, item);
    rlpDataRelease(data);
}

///
/// MARK: - Handle RLP Huge
///
#if 0
// Use in BREthereumNode recv() to write RLP data to file.
if (bytesCount > 2 * 1024 * 1024) {
    eth_log (LES_LOG_TOPIC, "XTRA: Large TCP item: %zu", bytesCount);
    FILE *foo = fopen ("/Users/ebg/les-item.txt", "w");

    BRRlpData data = rlpGetDataSharedDontRelease (node->coder.rlp, item);
    char *dataAsHex = encodeHexCreate(NULL, data.bytes, data.bytesCount);

    size_t written = fwrite (dataAsHex , sizeof(char), strlen(dataAsHex), foo);
    assert (strlen(dataAsHex) == written);
    fclose (foo);
}
#endif

static void
handleRLPHuge (BRRlpCoder coder, const char *filename) {
    BRRlpData data;
    BRRlpItem item;

    FILE *file = fopen (filename, "r");
    char *input = malloc (10 * 1024  *1024);
    size_t inputCount = fread (input, sizeof (char), 10 * 1024 * 1024, file);
    input[inputCount] = '\0';

    // Strip a leading "0x"
    if (0 == strncmp (input, "0x", 2))
        input = &input[2];

    // Fill `data` and `item`
    data.bytes = decodeHexCreate(&data.bytesCount, input, strlen (input));
    item = rlpGetItem (coder, data);

#if 0
    // use to debug sub-itens
    size_t l1Count = 0;
    const BRRlpItem *l1Items = rlpDecodeList (coder, item, &l1Count);
    assert (3 == l1Count);

    size_t l2Count = 0;
    const BRRlpItem *l2Items = rlpDecodeList (coder, l1Items[2], &l2Count); // 192 'block bodies'
    for (size_t index = 0; index < l2Count; index++) {
        eth_log("EXP", "L2: %zu", index);
        rlpReleaseItem (coder, l2Items[index]);
    }

    size_t l3Count = 0;
    const BRRlpItem *l3Items = rlpDecodeList(coder, l2Items[0], &l3Count);

    size_t l4Count = 0;
    const BRRlpItem *l4Items = rlpDecodeList(coder, l3Items[1], &l4Count); // bodies
    for (size_t index = 0; index < l4Count; index++) {
        eth_log("EXP", "L4: %zu", index);
       rlpReleaseItem (coder, l4Items[index]);
    }

    size_t l5Count = 0;
    const BRRlpItem *l5Items = rlpDecodeList (coder, l4Items[0], &l5Count); // N transactions
        for (size_t index = 0; index < l5Count; index++) {
            eth_log("EXP", "L5: %zu", index);
            rlpReleaseItem (coder, l5Items[index]);
        }

    eth_log("EXP", "L6...%s", "");
#endif
    //    rlpShow(data, "RLP:");
    rlpReleaseItem(coder, item);
    eth_log("EXP", "released%s", "");
}

int main(int argc, const char * argv[]) {
    BRRlpCoder coder = rlpCoderCreate();

    const char *input = TEST_TRANS;

    if (argc > 1) {
        input = argv[1];
    }

//    handleRLPHuge(coder, "/Users/ebg/les-item-2.txt");
    handleRLP (coder, SOME_RLP);
//    handleTrans(coder, TEST_TRANS_ETH);
//    eth_log ("EXP", "\n\n\n%s", "");
//    handleTrans(coder, TEST_TRANS_BRD);

    rlpCoderRelease(coder);
    return 0;
}
