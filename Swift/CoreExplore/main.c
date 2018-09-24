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

#define SOME_RLP "F88FD18F70726F746F636F6C56657273696F6E01CB896E6574776F726B496401CD86686561645464850400000000EA886865616448617368A0D4E56740F876AEF8C010B86A40D5F56745A118D0906A34E69AEC8CDB1CB8FA3C987686561644E756D80ED8B67656E6573697348617368A0D4E56740F876AEF8C010B86A40D5F56745A118D0906A34E69AEC8CDB1CB8FA3"
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
    rlpShow(data, "RLP:");
}

int main(int argc, const char * argv[]) {
    BRRlpCoder coder = rlpCoderCreate();

    const char *input = TEST_TRANS;

    if (argc > 1) {
        input = argv[1];
    }

    handleRLP (coder, SOME_RLP);
//    handleTrans(coder, TEST_TRANS_ETH);
//    eth_log ("EXP", "\n\n\n%s", "");
//    handleTrans(coder, TEST_TRANS_BRD);

    return 0;
}
