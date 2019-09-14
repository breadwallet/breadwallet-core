//
//  main.c
//  CoreExplore
//
//  Created by Ed Gamble on 8/25/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <stdio.h>
#include <unistd.h>         // sleep
#include <pthread.h>

#include "support/BRAssert.h"
#include "support/BRAddress.h"
#include "support/BRBIP39Mnemonic.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39WordsEn.h"
#include "bitcoin/BRTransaction.h"
#include "bitcoin/BRWallet.h"

#include "ethereum/rlp/BRRlp.h"
#include "ethereum/util/BRUtil.h"
#include "ethereum/blockchain/BREthereumBlockChain.h"
#include "ethereum/ewm/BREthereumAccount.h"
#include "ethereum/ewm/BREthereumTransfer.h"
#include "ethereum/BREthereum.h"

#define TEST_TRANS_ETH      "0xf86a75843b9aca00825208943d7eefb552b7d633e7f9eb48cd82cd098ecd5b4687038d7ea4c68000802aa045827725970e3c9729c9450b3ff04f98f10e231ebdeec5d522585a9a57bab1b4a025547970f9bedbef17d4aadd38f4263955633507689a9d7598c9e9bc38438d03"
#define TEST_TRANS_BRD      "0xf8a976841dcd65008301676094722dd3f80bac40c951b51bdd28dd19d43576218080b844a9059cbb0000000000000000000000003d7eefb552b7d633e7f9eb48cd82cd098ecd5b46000000000000000000000000000000000000000000000000000000e8d4a5100029a02604f887d60d438d29c73b69ade7208ced970d5c74b1bf5b2f156e56c785f15da03b56daa107f678fee099347af966093081e3ef87dc6040a1ce0113452e37f664"
#define TEST_TRANS          TEST_TRANS_ETH

static void
handlePaperKeyToAccount (void) {
    char paperKey[1024];

    printf ("Enter PaperKey: ");
    char *readPaperKey = fgets (paperKey, 1023, stdin);
    size_t paperKeyLen = strlen (paperKey);

    if (0 == paperKeyLen) { printf ("Failed\n"); return; };
    if ('\n' == paperKey[paperKeyLen-1])
        paperKey[paperKeyLen - 1] = '\0';

    printf ("Read: %s\n", paperKey);

    BREthereumAccount account = createAccount(paperKey);
    char *publicAddress = accountGetPrimaryAddressString(account);
    printf ("Public Address: %s\n", publicAddress);
    free (publicAddress);

}

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

/// MARK: - Handle RLP Huge

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

#define BITCOIN_TX_HASH_1  "cd72882cc0d9c380d9884745ce984fdb972947b7ab980741fcae5c86a583faf4"
#define BITCOIN_TX_PARSE_1 "010000000001010cc9189dcce9dbe1179af3fab25ce118f2a3b32f4c0b307b5ba3d488ce3d223f0000000023220020f0fcdc34552c541f424552c55362a3c8c2d1add1f8d104b9f576f5bd0b5e514cffffffff0235732500000000001976a914bebb7325352454d51f430aedd584614dd0e5778d88accb44d9010000000017a9141c4dc70480cccc282a9c6d8fa14b76e3004c0477870a0048304502210083966aa95f9d75a7ba24d0a8691057c27738669c2dea444caa435f73ef1d3db002205a6524a3c22a8e568f5c3f511daa44386d366053cc72b97c50d33e7bb9831838014830450221009684994da402205cd0c55adc97fcdef536d5db21c2446134c7b4089c6c5fed5602201978181ba0c85a6f99ced55ae068cba201b9ec5906999f075fcf57dd92033f12016952210324073a7265277c326631b4d3d48cebef16c3285f5322a983d2f21d5d013396c22103aa8acbbe95fe4e212157e191e10067f66d9b255eec974f430bf9e9dc56b9539e210245ad079728c3a19bc5aa18539ee9ae3f2e0148bb438483f764cce0874272117453ae0000000033790800447dd121"

static void
handleBitconTransactionHashReverse (const char *chars) {
    size_t bytesLen;
    uint8_t *bytes = decodeHexCreate (&bytesLen, chars, strlen(chars));
    assert (bytesLen == 32);

    UInt256 hash;
    memcpy (hash.u8, bytes, 32);

    UInt256 hashReversed = UInt256Reverse(hash);
    printf ("Hash Forward: %s\n", chars);
    printf ("Hash Reverse: %s\n", encodeHexCreate(NULL, hashReversed.u8, 32));
}

#define BITCOIN_TX_PARSE_N "0100000000010195603c95b906c613da65825ae978d396027d3afba127ccb3e17f168250acc93600000000232200201f7babec88c8bd38c26a41abada201282b95cdd448c407af01529a0edb0b8101ffffffff02ce0acf010000000017a91425dba4104eeaf2a6c18888cc98e74e33ce58579a87a8932700000000001976a914bebb7325352454d51f430aedd584614dd0e5778d88ac0a0047304402204094574ae42312cacd982d467ab042d7247be753f44b679524e25b0ab20c7c9402203971031fffd999704f2e55977625be3363a0daefd16efb94deed72b305c3f73501483045022100cf88682953e6aca362e2f9c24a63ef1a2256e1ef81ab37151486e08f8183343702201b59a4db648f0645811d33558ceebbb43527b0d12766da1cebfb85e9f2a82c920169522103d46657769cefd69484dc2811ffdd355102c146b56a3097d79a96feca1e6375e621034aefa9486d6dd43506f3417dbd1fe062a7a4e6747ae1dccddbca29c951593ea2210229ee8d328be97fd28b1cc66a6febba28c38e3b61e0552a0a982b14ca6b93b72953ae000000009f7e0800720ddd21"
static void
handleBitcoinTransactionParse (const char *chars) {
    size_t bytesLen;
    uint8_t *bytes = decodeHexCreate (&bytesLen, chars, strlen(chars));
    
    BRTransaction *tx = BRTransactionParse(bytes, bytesLen);
    assert (NULL != tx);
}

//
// BRAssert
//
typedef void* (*ThreadRoutine) (void*);

pthread_t asserter;

void assertHandle(void *info) {
    printf ("AssertHandle\n");
}

void assertRecover (void *info) {
    printf ("AssertRecover: %d\n", (int) info);
}

int work (int option) {
    if (0 == option % 2) {
        printf ("Assert\n");
        BRAssert(0);
    }
    else {
        printf ("Fail\n");
        BRFail();
    }
    return 1;
}

void *assertThread (void *ignore) {
    pthread_setname_np ("Asserter");
    sleep (2);
    work ((int) ignore);

    return NULL;
}

//
// Transaction Decode
//
#define ETH_TRANS1_ADDR "0xf89862b47e26d7ceb8f91216bba42013567521f9"
#define ETH_TRANS1 "f8a92e84ee6b280083029040940abdace70d3790235af448c88547603b945604ea80b844a9059cbb00000000000000000000000043a0fe792d38745575f624728070b20072208d2d00000000000000000000000000000000000000000000000029a2241af62c000026a06774cdf4cbd9b4cc0003d030d8ef8916017a6ccec4a6856cc18aaf8bb623c91ca034aceeb4d46af817138480ce34d63be8ef8ad37a5aa10afc851dd6f5293999d1"

// addr:
#define ETH_TRANS2_ADDR "0x8c13c45d667fcbafe125b3b71db761a811e4a7b0"
#define ETH_TRANS2 "f8a92e847735940083029040940abdace70d3790235af448c88547603b945604ea80b844a9059cbb00000000000000000000000043a0fe792d38745575f624728070b20072208d2d00000000000000000000000000000000000000000000000029a2241af62c000026a09e64771d0ad4d4e509e8bf980cf3d7fa759cf8e09aefc3ee8f39bbdfbe82fca4a0473a223c9993040edf5792bf1f787a8c3d2bce4c618230b27d5068628397b6a5"

#define ETH_PAPER_KEY "era frozen diary puzzle person pond spy lawsuit sell group bachelor elbow"

static BREthereumAddress
handleEthTransactionDecode1 (BRRlpCoder coder, const char *rlpString) {
    size_t rlpBytesCount;
    uint8_t *rlpBytes = decodeHexCreate (&rlpBytesCount, rlpString, strlen (rlpString));

    BRRlpData  data  = { rlpBytesCount, rlpBytes };
    BRRlpItem  item  = rlpGetItem(coder, data);
    rlpShowItem(coder, item, "FOO");

    BREthereumTransaction transaction = transactionRlpDecode (item, ethereumMainnet, RLP_TYPE_TRANSACTION_SIGNED, coder);
    BREthereumSignature sig1 = transactionGetSignature(transaction);
    BREthereumAddress   add1 = transactionExtractAddress (transaction, ethereumMainnet, coder);

    BREthereumTransfer transfer = transferCreateWithTransactionOriginating (transaction, TRANSFER_BASIS_TRANSACTION);
    BREthereumAccount  account = createAccount(ETH_PAPER_KEY);
    BREthereumAddress  address = accountGetPrimaryAddress (account);

    transferSign (transfer, ethereumMainnet, account, address, ETH_PAPER_KEY);
    BREthereumSignature sig2 = transactionGetSignature (transferGetOriginatingTransaction(transfer));
    BREthereumAddress   add2 = transactionExtractAddress (transferGetOriginatingTransaction(transfer), ethereumMainnet, coder);

    return add2;
}

static void
handleEthTransactionDecode (BRRlpCoder coder) {

    BREthereumAddress addr1 = handleEthTransactionDecode1 (coder, ETH_TRANS1);
    BREthereumAddress addr2 = handleEthTransactionDecode1 (coder, ETH_TRANS2);
    assert (ETHEREUM_BOOLEAN_TRUE == addressEqual (addr1, addr2));
}

//
// BRWalletUnusedAddr - do multiple calls produce multiple multiple addres
//
#define WALLET_GAP      10

static void
handleWalletAddrs (void) {
    BRAddress addrs1[WALLET_GAP];
    BRAddress addrs2[WALLET_GAP];

    const char *phrase = "a random seed";
    UInt512 seed;

    BRBIP39DeriveKey(&seed, phrase, NULL);

    BRMasterPubKey mpk = BRBIP32MasterPubKey(&seed, sizeof(seed));

    BRWallet *wallet = BRWalletNew (BITCOIN_ADDRESS_PARAMS, NULL, 0, mpk);

    BRWalletUnusedAddrs (wallet, addrs1, WALLET_GAP, 0);
    BRWalletUnusedAddrs (wallet, addrs2, WALLET_GAP, 0);

    for (size_t index = 0; index < WALLET_GAP; index++)
        assert (0 == strcmp (addrs1[index].s, addrs2[index].s));

    int diff = memcmp (addrs1[0].s, addrs2[0].s, sizeof (BRAddress));
//    assert (0 == memcmp (addrs1, addrs2, sizeof (addrs1)));
}


#define IAN_COLEMAN_PAPER_KEY   "trigger verb tilt script hospital pumpkin young then same clerk march afford"
#define IAN_COLEMAN_RIPPLE_DEX0_PUBKEY "0286d3703a7cf4540401cf96838a8d9eb5dd06e490d92988c00e3a185c975324e5"
#define IAN_COLEMAN_RIPPLE_DEX0_PRIKEY "2b8bcc2ac6e96d76c56c1135340e395b0aec99540617d76ded17acbf8f4e3eb4"
#define IAN_COLEMAN_RIPPLE_DEX0_ADDR   "rf3uTZX3ATLJN9hGekyXLiGgXzGHEVinSF"

static BRKey
derivePrivateKeyFromSeedRipple (UInt512 seed, uint32_t index) {
    BRKey privateKey;

    // The BIP32 privateKey for m/44'/60'/0'/0/index
    BRBIP32PrivKeyPath(&privateKey, &seed, sizeof(UInt512), 5,
                       44 | BIP32_HARD,          // purpose  : BIP-44
                       144 | BIP32_HARD,          // coin_type: Ripple
                       0 | BIP32_HARD,          // account  : <n/a>
                       0,                        // change   : not change
                       index);                   // index    :

    privateKey.compressed = 0;

    return privateKey;
}

static void
handleRippleAccount (void) {
    installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);

    UInt512 seed = deriveSeedFromPaperKey (IAN_COLEMAN_PAPER_KEY);
    BRKey   key  = derivePrivateKeyFromSeedRipple (seed, 0);
    key.compressed = 1;

    // Fill key.pubKey
    assert (33 == BRKeyPubKey (&key, NULL, 0));

    // Expected result
    size_t pubKeyResultLen;
    uint8_t *pubKeyResult = decodeHexCreate(&pubKeyResultLen, IAN_COLEMAN_RIPPLE_DEX0_PUBKEY, strlen(IAN_COLEMAN_RIPPLE_DEX0_PUBKEY));
    assert (33 == pubKeyResultLen);

    // Confirm
    assert (0 == memcmp (pubKeyResult, key.pubKey, 33));
}

static void
handleBRBCashAddrDecode (void) {
    char *bCashAddr = "77047ecdd5ae988f30d68e828dad668439ad3e5ebba05680089c80f0be82d889";
    char bitcoinAddr36 [37];
    BRBCashAddrDecode(bitcoinAddr36, bCashAddr);
}

//
//
//
//
// String from HexEncoding
//
static void
handleStringFromHex (const char *hex) {
    if (0 == strncmp (hex, "0x", 2))
        hex += 2;

    size_t   bytesCount;
    uint8_t *bytes = decodeHexCreate(&bytesCount, hex, strlen(hex));

    char string[bytesCount + 1];
    for (size_t index = 0; index < bytesCount; index++)
        string[index] = (char) bytes[index];
    string[bytesCount] = '\0';

    printf ("Hex : %s\nChar: %s\n", hex, string);
}

static void
handleAddressFromString (const char *hex, BRRlpCoder coder) {
    BREthereumAddress address = addressCreate(hex);

    BRRlpItem item = addressRlpEncode(address, coder);
    rlpShowItem(coder, item, "ADDR");
    rlpReleaseItem(coder, item);
}
void handleLogDecode (BRRlpCoder coder) {
    FILE *foo = fopen ("/Users/ebg/log-item", "r");

    uint8_t bytes[1024];
    memset (bytes, 0, 1024);

    size_t bytesCount = fread (bytes, 1, 1024, foo);

    BRRlpData data = { bytesCount, bytes };

    BRRlpItem  item  = rlpGetItem (coder, data);

    BREthereumLog log = logRlpDecode(item, RLP_TYPE_ARCHIVE, coder);

    rlpReleaseItem (coder, item);

    logRelease(log);
}

static void handleHasherShowData (uint8_t *data, size_t dataLen) {
    printf ("a = Data([");
    for (size_t index = 0; index < dataLen; index++)
        printf ("0x%02x%s", data[index], (index + 1 < dataLen ? ", " : ""));
    printf ("])\n");
}
void handleHasherTestGen () {
    const char *message;
    uint8_t digest[1024];

    message = "Free online SHA256 Calculator, type text here...";
    BRSHA256(digest, message, strlen(message));
    handleHasherShowData (digest, 32);

    message = "Free online SHA224 Calculator, type text here...";
    BRSHA224 (digest, message, strlen(message));
    handleHasherShowData (digest, 28);

    message = "Free online SHA256_2 Calculator, type text here...";
    BRSHA256_2(digest, message, strlen(message));
    handleHasherShowData (digest, 32);

    message = "Free online SHA384 Calculator, type text here...";
    BRSHA384(digest, message, strlen(message));
    handleHasherShowData (digest, 48);

    message = "Free online HASH160 Calculator, type text here...";
    BRHash160(digest, message, strlen(message));
    handleHasherShowData (digest, 20);
}

int main(int argc, const char * argv[]) {
    BRRlpCoder coder = rlpCoderCreate();

    const char *input = TEST_TRANS;

    if (argc > 1) {
        input = argv[1];
    }

#if 0
    handleBitconTransactionHashReverse(BITCOIN_TX_HASH_1);
    handleBitcoinTransactionParse(BITCOIN_TX_PARSE_N);
#endif

#if 0
    handlePaperKeyToAccount();
    handleRLPHuge(coder, "/Users/ebg/les-item-2.txt");
    handleRLP (coder, SOME_RLP);
    handleTrans(coder, TEST_TRANS_ETH);
    eth_log ("EXP", "\n\n\n%s", "");
    handleTrans(coder, TEST_TRANS_BRD);
#endif

#if 0
    BRAssertInstall (NULL, assertHandle);
    BRAssertDefineRecovery((BRAssertRecoveryInfo) 1, assertRecover);
    BRAssertDefineRecovery((BRAssertRecoveryInfo) 2, assertRecover);

    printf ("AssertConnect\n");
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize (&attr, 1024 * 1024);
    pthread_create (&asserter, &attr, (ThreadRoutine) assertThread, (void*) 0);
    sleep (1);
    pthread_create (&asserter, &attr, (ThreadRoutine) assertThread, (void*) 1);
    pthread_attr_destroy(&attr);

    pthread_join(asserter, NULL);
    BRAssertUninstall ();
    printf ("main: Paused\n");
    sleep (5);
    printf ("main: Done\n");
#endif

#if 0
    handleEthTransactionDecode (coder);
#endif

#if 0
    handleWalletAddrs();
#endif


#if 0
    handleStringFromHex("0x696e73756666696369656e742066756e647320666f7220676173202a207072696365202b2076616c7565");
    handleAddressFromString("0x0AbdAce70D3790235af448C88547603b945604ea", coder);

#endif

#if 0
    handleRippleAccount();
#endif

#if 0
    handleBRBCashAddrDecode();
#endif

#if 0
    handleLogDecode(coder);
#endif

#if 1
    handleHasherTestGen();
#endif
    rlpCoderRelease(coder);
    return 0;
}
