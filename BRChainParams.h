//
//  BRChainParams.h
//
//  Created by Aaron Voisine on 1/10/18.
//  Copyright (c) 2019 breadwallet LLC
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

#ifndef BRChainParams_h
#define BRChainParams_h

#include "BRMerkleBlock.h"
#include "BRSet.h"
#include <assert.h>

typedef struct {
    uint32_t height;
    UInt256 hash;
    uint32_t timestamp;
    uint32_t target;
} BRCheckPoint;

typedef struct {
    const char * const *dnsSeeds; // NULL terminated array of dns seeds
    uint16_t standardPort;
    uint32_t magicNumber;
    uint64_t services;
    int (*verifyDifficulty)(const BRMerkleBlock *block, const BRSet *blockSet); // blockSet must have last 2016 blocks
    const BRCheckPoint *checkpoints;
    size_t checkpointsCount;
} BRChainParams;

static const char *BRMainNetDNSSeeds[] = {
    "dnsseed.litecoinpool.org.", "seed-a.litecoin.loshan.co.uk.", "dnsseed.thrasher.io.",
    "dnsseed.koin-project.com.", "dnsseed.litecointools.com.", NULL};

static const char *BRTestNetDNSSeeds[] = {
    "testnet-seed.ltc.xurious.com.", "seed-b.litecoin.loshan.co.uk.", "dnsseed-testnet.thrasher.io.", NULL
};

// blockchain checkpoints - these are also used as starting points for partial chain downloads, so they must be at
// difficulty transition boundaries in order to verify the block difficulty at the immediately following transition
static const BRCheckPoint BRMainNetCheckpoints[] = {
    {       0, uint256("12a765e31ffd4059bada1e25190f6e98c99d9714d334efa41a195a7e7e04bfe2"), 1317972665, 0x1e0ffff0 },
    {   20160, uint256("633036c8df655531c2449b2d09b264cc0b49d945a89be23fd3c1a97361ca198c"), 1319798300, 0x1d055262 },
    {   40320, uint256("d148cdd2cf44069cef4b63f0feaf30a8d291ca9ea9ba7e83f226b9738c1d5e9c"), 1322522019, 0x1d018053 },
    {   60480, uint256("3250f0a560d55f039c34bfaee1b71297aa5104ac6641778f9a87d73232d12c6c"), 1325540574, 0x1d00e848 },
    {   80640, uint256("bedc0a090b740b1902d870aeb6caa89040a24e7d670d46f8ef035fd9d2e9ce80"), 1328779944, 0x1d00ab92 },
    {  100800, uint256("7b0b620d15f781faaaa73b43607a49d5becb2b803ef19b4010014646cc177a61"), 1331873688, 0x1d00ae9f },
    {  120960, uint256("dbd6249f30e5690890bc03dabcc0a526c46adcde572be06af4075b6ea28aa251"), 1334881566, 0x1d009e48 },
    {  141120, uint256("5d5e15a45cecf2b9528e36e63c407167423a2f9963a96bbce3b67b75fd10be2a"), 1338009318, 0x1d00d6a6 },
    {  161280, uint256("f595c754d0abcfe3616573bfabee01b230ec0ea6b2f2894c40214ea23d772b6c"), 1340918301, 0x1d008881 },
    {  181440, uint256("d7fa3152959f3c25e33edf825f7cbef75ee651d5f9183cc4ed8d19d57b8f35a4"), 1343534530, 0x1c1cd430 },
    {  201600, uint256("d481df8e8ce144fca9ae6b3157cc706e903c6ea161a13d2c421270354a02d6d0"), 1346567025, 0x1c1c89e8 },
    {  282240, uint256("8932095fba44bd6860fd71745c0dca908769221a47166ab1fb442b6cefcd53fb"), 1358801720, 0x1c0ced21 },
    {  342720, uint256("33f62e026a202be550e8a9df37d638d38991553544e279cb264123378bf46042"), 1367113967, 0x1c0095a5 },
    {  383040, uint256("5c0a443361c1356796a7db472c69433b6ce6108d61e4403fd9a9d91e01009ce3"), 1372971948, 0x1b481262 },
    {  443520, uint256("37d668803ed1efc24ffab4a2a90da9ac92679acf68370d7570f042c2bd6d651b"), 1382034998, 0x1b3f864f },
    {  504000, uint256("97db0624d3d5137bc085f0d731607314972bb4124b85b73420ef9aa5fc10d640"), 1390892377, 0x1b1aa868 },
    {  564480, uint256("c876276bf12754c2b265787d9e7ab83d429e59761dc63057f728529018db7834"), 1399724592, 0x1b099dce },
    {  624960, uint256("ccac71fafe98107b81ac3e0eed41190e4d47600962c93c49db8843b53f760bda"), 1408389228, 0x1b02552d },
    {  685440, uint256("29d2328990dda4c4870846d4e3d573785452bed68e6013930a83fc8d5fe89b09"), 1417289378, 0x1b01473b },
    {  745920, uint256("04809a35ff6e5054e21d14582072605b812b7d4ae11d3450e7c03a7237e1d35d"), 1426441593, 0x1b019b8c },
    {  806400, uint256("e2363e8b3e8f237b9b1bfc1c72ede80fef2c7bd1aabcd78afed82065a194b960"), 1435516150, 0x1b019268 },
    {  846720, uint256("6f5d94d7cfd01f1dbf4aa631b987f8e2ec9d0c57720604787b816bafe34192a8"), 1441561050, 0x1b0187a3 },
    {  901152, uint256("cfccdf8e3830ae4879e910051ac3dc583b4fb45b83be3a38019e5d9326dfa223"), 1449698771, 0x1b015b0e },
    {  953568, uint256("e46e01cf1239cffa69408ac162d517bac5a4899972e0328fd0ba4d93e8ad3764"), 1457542869, 0x1b013c91 },
    { 1058400, uint256("76ce37c66d449a4ffbfc35674cf932da701066a001dc223754f9250dd2bdbc62"), 1473296285, 0x1b013ca7 },
    { 1260000, uint256("85a22b528d805bf7a641d1d7c6d96ef5054beda3dcab6be7b83f2e3df24b33a8"), 1502976600, 0x1a25a0d3 },
    { 1411200, uint256("92c85b76f3d4bffca76b23717e4eb1b667c77c96fd52d4dd5dd843bbee64cd73"), 1524838967, 0x1a0203a7 },
	{ 2036160, uint256("97ab7a0bf3cd7d694c1b369090aea9449e93f92763808de2a073cc8ab0657292"), 1618643881, 0x1a01ab48 }
};

static const BRCheckPoint BRTestNetCheckpoints[] = {
    {       0, uint256("4966625a4b2851d9fdee139e56211a0d88575f59ed816ff5e6a63deb4e3e29a0"), 1486949366, 0x1e0ffff0 },
	{ 2282112, uint256("b64455a7630d72d982d7e00966e04e3c148a482d2f2b52e20bd7acc3aadbcd69"), 1649496420, 0x1e03ffff }
};

static int BRMainNetVerifyDifficulty(const BRMerkleBlock *block, const BRSet *blockSet)
{
    // const BRMerkleBlock *previous, *b = NULL;
    // uint32_t i;

    // assert(block != NULL);
    // assert(blockSet != NULL);

    // // check if we hit a difficulty transition, and find previous transition block
    // if ((block->height % BLOCK_DIFFICULTY_INTERVAL) == 0) {
    //     for (i = 0, b = block; b && i < BLOCK_DIFFICULTY_INTERVAL; i++) {
    //         b = BRSetGet(blockSet, &b->prevBlock);
    //     }
    // }

    // previous = BRSetGet(blockSet, &block->prevBlock);
    // return BRMerkleBlockVerifyDifficulty(block, previous, (b) ? b->timestamp : 0);
    return 1;
}

static int BRTestNetVerifyDifficulty(const BRMerkleBlock *block, const BRSet *blockSet)
{
    return 1; // XXX skip testnet difficulty check for now
}

static const BRChainParams BRMainNetParams = {
    BRMainNetDNSSeeds,
    9333,       // standardPort
    0xdbb6c0fb, // magicNumber
    0,          // services
    BRMainNetVerifyDifficulty,
    BRMainNetCheckpoints,
    sizeof(BRMainNetCheckpoints) / sizeof(*BRMainNetCheckpoints)};

static const BRChainParams BRTestNetParams = {
    BRTestNetDNSSeeds,
    19335,      // standardPort
    0xf1c8d2fd, // magicNumber
    0,          // services
    BRTestNetVerifyDifficulty,
    BRTestNetCheckpoints,
    sizeof(BRTestNetCheckpoints) / sizeof(*BRTestNetCheckpoints)};

#endif // BRChainParams_h
