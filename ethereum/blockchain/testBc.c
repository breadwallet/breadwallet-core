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

#define BLOCK_HEADER_0_RLP "f9020ca00000000000000000000000000000000000000000000000000000000000000000a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000a0d7f8974fb5ac78d9ac099b9ad5018bedc2ce0a72dad1827a1709da30580f0544a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b9010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000850400000000008213880000a011bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82faa0000000000000000000000000000000000000000000000000000000000000000042"
#define BLOCK_HEADER_1_RLP "f90211a0d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d493479405a56e2d52c817161883f50c441c3228cfe54d9fa0d67e4d450343046425ae4271474353857ab860dbc0a1dde64b41b5cd3a532bf3a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008503ff80000001821388008455ba422499476574682f76312e302e302f6c696e75782f676f312e342e32a0969b900de27b6ac6a67742365dd65f55a0526c41fd18e1b16f1a1215c2e66f5988539bd4979fef1ec4"
#define BLOCK_HEADER_2_RLP "f90218a088e96d4537bea4d9c05d12549907b32561d3bf31f45aae734cdc119f13406cb6a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794dd2f1e6e498202e86d8f5442af596580a4f03c2ca04943d941637411107494da9ec8bc04359d731bfd08b72b4d0edcbd4cd2ecb341a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008503ff00100002821388008455ba4241a0476574682f76312e302e302d30636463373634372f6c696e75782f676f312e34a02f0790c5aa31ab94195e1f6443d645af5b75c46c04fbf9911711198a0ce8fdda88b853fa261a86aa9e"

#define BLOCK_HEADER_4000000_RLP "f90218a09b3c1d182975fdaa5797879cbc45d6b00a84fb3b13980a107645b2491bcca899a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347941e9939daaad6924ad004c2560e90804164900341a0c191817e387e5405867535fb7f97991346e5f95c9dd040fb21503762ee22f5f8a0e3957fe2e24a0699872fe045ea7d1af2da0ea331fe782c802902b1a0e80560a8a0cd98e056d619b4047ff1ec598e61fd42edb3b48e6748bac41e164e1671d02623b90100408000002040080200010150010000022800000010030000001225000021010840000010000000020080400800000001000000000020040000004000000001000020000000000000c000048c802010080000000000230080000180004000000020088038420800000200104204000800124000204800404000002910021080622020820180000080c00000080401980001008000028a000080400000001081004040000020002000120008100004240100140c03000080200200804000000000010000024004000000024000008000400000000400000001201000080100210100000002010000100000002400000204400800200800020003000020000000818703e5151f3eae1c833d090083666c4883437928845962979f97706f6f6c2e65746866616e732e6f726720284d4e313529a081277f51ee22c1022b848064b1c5af001e3ba06d808a1ef3fd52aad07279e0f088f285952002120e7f"
#define BLOCK_HEADER_4000001_RLP "f9020ea0b8a3f7f5cfc1748f91a684f20fe89031202cbadcd15078c49b85ec2a57f43853a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794ea674fdde714fd979de3edf0f56aa9716b898ec8a07b01440ffe0282749577cf99f6c90aa39e496af23bbdc64ab57a3f0d1bf8a467a0ab330290ef6907c3e411691347a3ba6933482354e48dc46738f2226aab0d848ca03db9076bd070e771806d05df3bfe7d83aae07fc94e35310ea304509f57fb27acb90100000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000800000280000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000400000000000000000000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000000000000080000000000000000000000000000000000000000000000000000041000000000000000000000000000004000000000000000000000008703e5d1c1e295f1833d090183666c488305282e84596297a38d65746865726d696e652d657536a04db91248cc4af54907e32cc5c160eeb7d5813cce11c87bbc85a0dc6db2b65419885d345a1001da875e"

#define BLOCK_HEADER_6000000_RLP "f90210a05d85a965cd1c00cbb7affb25371aeb2d7f610fda7207eb081c1ea2cb23254d5aa01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794829bd824b016326a401d083b33d092293333a830a0ecd323023ea0c000b6370e884ecd16cbe37244102688fc5c2e8c8a73bc1b18e1a050115fbf29591a3505eabfac5c5a6a8585e1d1c064a04348f00af02ddf79b58ea0d7adcb6c39b2aa65a8aa739e8638bdde4b3e01cd0ff1d0024fdca745ba3e6ea5b90100800b000004742018800113013800401243200020280a002206a2210000080300000c0088440401102010100202412c0302002804490118901200404d01040020000000449800241984c2120c4000200005482604c00480a021934009d8202802802000484214310003104450040009042062002200840048080001522008200281500001002421048282158006ac00488000098162a1000002068064040221108080082000484000280408014082810601100d00201000225200800810042420240008060840010042980010200440400000401082c8080420500003004670231400200d0020010000000082804aa02402088080505408580040508000800200870c6071524d8baa835b8d80837a214f8379fab1845b5246248fe4b883e5bda9e7a59ee4bb99e9b1bca0aed9520a8d287b8b459db916f2fe8e3337db07189fa8e8bb0f3e8e2cf5c7a98888b38c11380f0b72c3"
#define BLOCK_HEADER_6000001_RLP "f90210a0be847be2bceb74e660daf96b3f0669d58f59dc9101715689a00ef864a5408f43a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794829bd824b016326a401d083b33d092293333a830a05403dc9b2bf517f9c4919797ba0de4803cdfe2875fdbb46efe819b3949d7ea9ea074ae42835315bc064ef1bc1b38961d1d280c81ededc56a2a74f2912b4de96928a0a3576268ff5ef9e04f5b2a6e4c55b09178a840078ea14d1ceb2b8a7150c2af10b901001108801344200100840115120100220000000101101a00040402820501c0412e900400804084181061001020000408002100040420411014a0004021002410020014000090a010010116c00d00004000010020009005000800b1e1080820004010020080024003e4012800b810a80800242200004000410c0802921400602001444200020220304405c0101702a2101964000082000000301048e064200600400200982200004202080c0002000081000200300008118010401884086000020202000002400c621040180040628a0000104062249a3d1a41000002002060702006580805120800080001000c80428e208008d02078000c083444400102000082870c5ee5542341f9835b8d81837a3086837a28a4845b5246398fe4b883e5bda9e7a59ee4bb99e9b1bca0e0b2f7763077b1baeb2c27db06c7057d70667f671ccd5ccf09d4722cdf13230c888f62ecd00c385043"


#define BLOCK_HEADER_6500000_RLP "f90205a032fe0157e9a5d2d8336fafd58404833c28897a251e44a61810044280787ef764a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794b2930b35844a230f00e51431acae96fe543a0347a073a5463d90927bfdb0e3e9b719cfc70e6c5516d47847cf33bb7968fd70b27397a04db2dcbb5f0ae923ee0d16b0a5f31ce7912a964046252183d4b7e8ba6d631679a0434d91a0b5da90c87ebac9e8339a9515527ec59649cff896124156274e81a9b6b90100022848a050ec47a081bd80108a4003298802d00c8012020921150c4a05210b680180020418c42402a3504000810222a2e6006e062824a1a509009290012604822980400939406481414410a8482020800810000cc82401028891414402c00401000480ab027c4800a220300030486c200058904266c80900c4000130644060200a00600459304022b208141687040200004801600081114a900102d010351100021400e0468300001a90208040528900007504e44044e044604380c815620100a910022289000381421a0411600070cc1c0063056100080d81c0a19d21206430a218a0018012a440a0300100894c82c0c84c00245000069508415a0e0d31c020870bb854c51cecb583632ea0837a121d8379c924845bc050e38473656f32a04cbfb895a6919791d3680a00a0f81ce2f76d0dc0a3e09f85f277a72bf608302e883bc296840f7c5efd"
#define BLOCK_HEADER_6500001_RLP "f9020fa070c81c3cb256b5b930f05b244d095cb4845e9808c48d881e3cc31d18ae4c3ae5a0c2aa23c06575377756178d2618711dcab452d937481a3f9b5da35a37114052b19452e44f279f4203dcf680395379e5f9990a69f13ca0ab06dbcb41715c5519c1cb7bb7424d92d7830d5d049ebe0e68ce934d2584ac9ea0c8c7f4fcb7f264dcdc8b7b6d5c1f0cd347e3091de202758349f663ab62ea5d92a0711a6671cf79dd249d56d104ddce27e96812fda63a80b0f49c0859fe029daa02b9010000011800024210080100281100100002000000000000000c08000000020805080004000020000100000000000200008210004000020000800008880002200020400040000a01841010000048208000000500008400000180002200000000001005200000260000001000000000000810100051000020000000000110000000003000000020000000800000000000000000000002408040000000001000000000000000204044000010408200400100001000041000008000000800010000000000010002000200490000000000000000000000000400000402000808044020200001000000000000000000900000004401000000100010008000000400004110870bb856c51cecb583632ea1837a2145833002e6845bc050f08e7777772e4257506f6f6c2e6e6574a06871463fa03dbcf40efbc3834cc7997c8ad1c9291b18332014b404748279e30d88f401c6b821de3870"

static BREthereumBlockHeader
testGetBlockHeader (const char *rlp) {
    BRRlpData data;
    data.bytes = decodeHexCreate(&data.bytesCount, rlp, strlen (rlp));

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem blockItem = rlpGetItem(coder, data);

    BREthereumBlockHeader header = blockHeaderRlpDecode(blockItem, RLP_TYPE_NETWORK, coder);

    rlpDataRelease(data);
    rlpReleaseItem (coder, blockItem);
    rlpCoderRelease(coder);

    return header;
}

static BREthereumBlock
testGetBlock (const char *rlp) {
    BRRlpData data;
    data.bytes = decodeHexCreate(&data.bytesCount, rlp, strlen (rlp));

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem blockItem = rlpGetItem(coder, data);

    BREthereumBlock block = blockRlpDecode(blockItem, ethereumMainnet, RLP_TYPE_NETWORK, coder);

    rlpDataRelease(data);
    rlpReleaseItem (coder, blockItem);
    rlpCoderRelease(coder);

    return block;
}

extern void
runBlockHeaderTest (void) {
    // validate a block with its parent.
    BREthereumBlockHeader header_0 = testGetBlockHeader(BLOCK_HEADER_0_RLP); // networkGetGenesisBlockHeader (ethereumMainnet);
#if 0
    BREthereumBlockHeader header_1 = testGetBlockHeader(BLOCK_HEADER_1_RLP);
    BREthereumBlockHeader header_2 = testGetBlockHeader(BLOCK_HEADER_2_RLP);

    // Pre-HOMESTEAD_FORK_BLOCK_NUMBER
    assert (ETHEREUM_BOOLEAN_IS_TRUE (blockHeaderIsValidFull (header_2,
                                                              header_1,
                                                              0,
                                                              header_0,
                                                              NULL)));
#endif
    BREthereumBlockHeader header_6500000 = testGetBlockHeader(BLOCK_HEADER_6500000_RLP); // networkGetGenesisBlockHeader (ethereumMainnet);
    BREthereumBlockHeader header_6500001 = testGetBlockHeader(BLOCK_HEADER_6500001_RLP);

    assert (ETHEREUM_BOOLEAN_IS_TRUE (blockHeaderIsValidFull (header_6500001,
                                                              header_6500000,
                                                              0,
                                                              header_0,
                                                              NULL)));

    BREthereumBlockHeader header_6000000 = testGetBlockHeader(BLOCK_HEADER_6000000_RLP); // networkGetGenesisBlockHeader (ethereumMainnet);
    BREthereumBlockHeader header_6000001 = testGetBlockHeader(BLOCK_HEADER_6000001_RLP);

    assert (ETHEREUM_BOOLEAN_IS_TRUE (blockHeaderIsValidFull (header_6000001,
                                                              header_6000000,
                                                              0,
                                                              header_0,
                                                              NULL)));

    // Pre-BYZANTIUM_FORK_BLOCK_NUMBER
    BREthereumBlockHeader header_4000000 = testGetBlockHeader(BLOCK_HEADER_4000000_RLP); // networkGetGenesisBlockHeader (ethereumMainnet);
    BREthereumBlockHeader header_4000001 = testGetBlockHeader(BLOCK_HEADER_4000001_RLP);

    assert (ETHEREUM_BOOLEAN_IS_TRUE (blockHeaderIsValidFull (header_4000001,
                                                              header_4000000,
                                                              0,
                                                              header_0,
                                                              NULL)));


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
    rlpReleaseItem(coder, blockItem);

    blockItem = blockRlpEncode(block, ethereumMainnet, RLP_TYPE_NETWORK, coder);
    rlpShowItem(coder, blockItem, "BlockTest");
    rlpReleaseItem(coder, blockItem);
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

    // Log
    data.bytes = decodeHexCreate(&data.bytesCount, LOG_1_RLP, strlen (LOG_1_RLP));

    BRRlpCoder coder  = rlpCoderCreate();
    BRRlpItem logItem = rlpGetItem(coder, data);
    BREthereumLog log = logRlpDecode(logItem, RLP_TYPE_NETWORK, coder);
    rlpReleaseItem(coder, logItem);

    BREthereumAddress address = logGetAddress(log);
    size_t addressBytesCount;
    uint8_t *addressBytes = decodeHexCreate(&addressBytesCount, LOG_1_ADDRESS, strlen(LOG_1_ADDRESS));
    assert (addressBytesCount == sizeof (address.bytes));
    assert (0 == memcmp (address.bytes, addressBytes, addressBytesCount));
    free (addressBytes);

    // topic-0
    // topic-1


    logItem = logRlpEncode(log, RLP_TYPE_NETWORK, coder);
    BRRlpData encodeData = rlpGetData(coder, logItem);
    rlpReleaseItem(coder, logItem);

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
    rlpReleaseItem(coder, item);

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

    rlpReleaseItem(coder, encoding);
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
    runBlockHeaderTest ();
    runBlockTests();
    runLogTests();
    runAccountStateTests();
    runTransactionStatusTests();
    runTransactionReceiptTests();
}

