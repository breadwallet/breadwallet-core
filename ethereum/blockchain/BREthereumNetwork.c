//
//  BREthereumNetwork
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/13/18.
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

#include <stdlib.h>
#include "BREthereumNetwork.h"

static void
networkInitilizeAllIfAppropriate (void);

struct BREthereumNetworkRecord {
    const char *name;
    int chainId;
    BREthereumHash genesisBlockHeaderHash;
    BREthereumHash trustedCheckpointBlockHeaderHash;
};

extern BREthereumChainId
networkGetChainId (BREthereumNetwork network) {
    networkInitilizeAllIfAppropriate();
    return network->chainId;
}

extern BREthereumHash
networkGetGenesisBlockHeaderHash (BREthereumNetwork network) {
    networkInitilizeAllIfAppropriate();
    return network->genesisBlockHeaderHash;
}

extern BREthereumHash
networkGetTrustedCheckpointBlockHeaderHash (BREthereumNetwork network) {
    networkInitilizeAllIfAppropriate();
    return network->trustedCheckpointBlockHeaderHash;
}

extern const char *
networkGetName (BREthereumNetwork network) {
    return network->name;
}
//
// MARK: - Static Network Definitions
//

//
// Mainnet
//
static struct BREthereumNetworkRecord ethereumMainnetRecord = {
    "Mainnet",
    1,
    EMPTY_HASH_INIT,
    EMPTY_HASH_INIT
};
const BREthereumNetwork ethereumMainnet = &ethereumMainnetRecord;

/*
// MainnetChainConfig is the chain parameters to run a node on the main network.
MainnetChainConfig = &ChainConfig{
  ChainId:        big.NewInt(1),
  HomesteadBlock: big.NewInt(1150000),
  DAOForkBlock:   big.NewInt(1920000),
  DAOForkSupport: true,
  EIP150Block:    big.NewInt(2463000),
  EIP150Hash:     common.HexToHash("0x2086799aeebeae135c246c65021c82b4e15a2c451340993aacfd2751886514f0"),
  EIP155Block:    big.NewInt(2675000),
  EIP158Block:    big.NewInt(2675000),
  ByzantiumBlock: big.NewInt(4370000),
  Ethash: new(EthashConfig),
}
*/

//
// Testnet
//
static struct BREthereumNetworkRecord ethereumTestnetRecord = {
    "Testnet",
    3,
    EMPTY_HASH_INIT,
    EMPTY_HASH_INIT
};
const BREthereumNetwork ethereumTestnet = &ethereumTestnetRecord;

/*
// TestnetChainConfig contains the chain parameters to run a node on the Ropsten test network.
TestnetChainConfig = &ChainConfig{
  ChainId:        big.NewInt(3),
  HomesteadBlock: big.NewInt(0),
  DAOForkBlock:   nil,
  DAOForkSupport: true,
  EIP150Block:    big.NewInt(0),
  EIP150Hash:     common.HexToHash("0x41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d"),
  EIP155Block:    big.NewInt(10),
  EIP158Block:    big.NewInt(10),
  ByzantiumBlock: big.NewInt(1700000),
  Ethash: new(EthashConfig),
}
*/
//
// Rinkeby
//
static struct BREthereumNetworkRecord ethereumRinkebyRecord = {
    "Rinkeby",
    4,
    EMPTY_HASH_INIT,
    EMPTY_HASH_INIT
};
const BREthereumNetwork ethereumRinkeby = &ethereumRinkebyRecord;

/*
// RinkebyChainConfig contains the chain parameters to run a node on the Rinkeby test network.
RinkebyChainConfig = &ChainConfig{
  ChainId:        big.NewInt(4),
  HomesteadBlock: big.NewInt(1),
  DAOForkBlock:   nil,
  DAOForkSupport: true,
  EIP150Block:    big.NewInt(2),
  EIP150Hash:     common.HexToHash("0x9b095b36c15eaf13044373aef8ee0bd3a382a5abb92e402afa44b8249c3a90e9"),
  EIP155Block:    big.NewInt(3),
  EIP158Block:    big.NewInt(3),
  ByzantiumBlock: big.NewInt(1035301),
  Clique: &CliqueConfig{
    Period: 15,
    Epoch:  30000,
  },
}
*/

//
// MARK: - Trusted Checkpoints
//

/*
// trustedCheckpoint represents a set of post-processed trie roots (CHT and BloomTrie) associated with
// the appropriate section index and head hash. It is used to start light syncing from this checkpoint
// and avoid downloading the entire header chain while still being able to securely access old headers/logs.
type trustedCheckpoint struct {
    name                                string
    sectionIdx                          uint64
    sectionHead, chtRoot, bloomTrieRoot common.Hash
}

var (
     mainnetCheckpoint = trustedCheckpoint{
     name:          "mainnet",
     sectionIdx:    153,
     sectionHead:   common.HexToHash("04c2114a8cbe49ba5c37a03cc4b4b8d3adfc0bd2c78e0e726405dd84afca1d63"),
     chtRoot:       common.HexToHash("d7ec603e5d30b567a6e894ee7704e4603232f206d3e5a589794cec0c57bf318e"),
     bloomTrieRoot: common.HexToHash("0b139b8fb692e21f663ff200da287192201c28ef5813c1ac6ba02a0a4799eef9"),
     }

     ropstenCheckpoint = trustedCheckpoint{
     name:          "ropsten",
     sectionIdx:    79,
     sectionHead:   common.HexToHash("1b1ba890510e06411fdee9bb64ca7705c56a1a4ce3559ddb34b3680c526cb419"),
     chtRoot:       common.HexToHash("71d60207af74e5a22a3e1cfbfc89f9944f91b49aa980c86fba94d568369eaf44"),
     bloomTrieRoot: common.HexToHash("70aca4b3b6d08dde8704c95cedb1420394453c1aec390947751e69ff8c436360"),
     }
     )

// trustedCheckpoints associates each known checkpoint with the genesis hash of the chain it belongs to
var trustedCheckpoints = map[common.Hash]trustedCheckpoint{
    params.MainnetGenesisHash: mainnetCheckpoint,
    params.TestnetGenesisHash: ropstenCheckpoint,
}

 // Rinkeby: genesis for all intents and purposes.
 // > INFO [06-06|11:34:07] Block synchronisation started
 // INFO [06-06|11:34:08] Imported new block headers               count=192 elapsed=76.267ms number=192 hash=8c570c…ba360c ignored=0

*/
static void
networkInitilizeAllIfAppropriate (void) {
    static int needsInitialization = 1;

    if (needsInitialization) {

        // Mainnet

        ethereumMainnetRecord.genesisBlockHeaderHash =
        hashCreate ("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3");

        ethereumMainnetRecord.trustedCheckpointBlockHeaderHash =
        hashCreate("0x04c2114a8cbe49ba5c37a03cc4b4b8d3adfc0bd2c78e0e726405dd84afca1d63");

        // Testnet / 'Ropsten'

        ethereumTestnetRecord.genesisBlockHeaderHash =
        hashCreate("0x41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d");

        ethereumTestnetRecord.trustedCheckpointBlockHeaderHash =
        hashCreate("0x1b1ba890510e06411fdee9bb64ca7705c56a1a4ce3559ddb34b3680c526cb419");

        // Rinkeby

        ethereumRinkebyRecord.genesisBlockHeaderHash =
        hashCreate("0x6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177");
        
        ethereumRinkebyRecord.trustedCheckpointBlockHeaderHash =
        hashCreate("0x6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177");

        // Notable RACE
        needsInitialization = 0;

    }
}
