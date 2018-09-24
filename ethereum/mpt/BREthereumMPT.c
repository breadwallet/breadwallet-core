//
//  BREthereumMPT.c
//  Core
//
//  Created by Ed Gamble on 8/21/18.
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

#include "BREthereumMPT.h"

extern BREthereumMPTNodePath
mptProofDecode (BRRlpItem item,
                BRRlpCoder coder) {
//    rlpShowItem (coder, item, "MPT Proof");
    return (BREthereumMPTNodePath) {};
}

extern BRArrayOf(BREthereumMPTNodePath)
mptProofDecodeList (BRRlpItem item,
                    BRRlpCoder coder) {
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);

    // TODO: Wrong - here to flag success!
//    assert (itemsCount == 0);
    
    BRArrayOf (BREthereumMPTNodePath) proofs;
    array_new (proofs, itemsCount);
    for (size_t index = 0; index < itemsCount; index++)
        array_add (proofs, mptProofDecode (items[index], coder));
    return proofs;
}

/*
 // A Parity 'Acount Response' - first of 5 list elements is the 'proof.
 ETH: RECV: L  3: [
 ETH: RECV:   I  0: 0x
 ETH: RECV:   I  5: 0x06fc1f1820
 ETH: RECV:   L  1: [
 ETH: RECV:     L  2: [
 ETH: RECV:       I  1: 0x05
 ETH: RECV:       L  5: [
 ETH: RECV:         L  4: [
 ETH: RECV:           I532: 0xf90211a03a712d5560b6d66ac7e3d5c2f353f98f3d1e8eb57d8808a4468738d8a8998a2ea08cd597e7ec5295f00cc07d8d35e5379eb1101d379d493992724dc931ca1c7eaaa0beec28d1a915ed45c7c018be13e031a08685cd98b3f456ce8c4e1638e545dc97a00272de229cfed792911729093c1d157bc0020e6e7d4d40976ebff9ac1607240aa0939d7c0751da736d51a5ec2cd302527b0c4587672ef593a534ee7a9a3f1a9626a023a9385ffe8f1a51e70e35ca7998170bf185378e5cc1d0a966417cde38be23c4a0793a58f36fcbe76bf9fbb2bbcb19ea51acd360a4df30fec005d13a8bf86c22f4a08c17d3694692ef726b43c53dd8fe8aee41e84c4b43d2d3ef34bfa11c7bc2a7caa05f9a676a864a596847a476bbcfd061899971f44dcfa9250859f48ad8eea7a8cba0381cdfe640dc160a3950beecee2951f2b9eb116dbed4067aeac286c78fb07b87a0c8d5ab1e05fb8d666fc93a597c03304f01f9be06503cb5aadafa247fb9a40635a0be46d054ec47780978674d01aa667ab4a3beff8e543ded5ba2873f3c06eab603a00f5556798e9b88cab09262f6de1a93b4a93d93b74b03803454ebc455409468f4a0b8027d751aea9f22674e1c6229b42e0c35839376e4d5a9b409ecc1f85aaa222ca08e0af74fe8e5a23b87f4f0e79ee2b12595eb141c787c3fdb78f632d3cfea80bea010398823ba2cef79533fde4483...
 ETH: RECV:           I532: 0xf90211a0ed14737976d472d5a0dc31cc084ae669760fc9b3a63ef27b8e073b17d02fa531a07674ac23fe29a9cdc8510918b949a068f67e05bed349fc55a9ee7bc107a1b27da0b456f0f2102a3afc355a39495d7415828f2697299b161cf9d8caf8c75eeda8ffa0e194204f08d529fc75c8c5065888b6b1360e149fc677c162fac05a859960dcd6a045d755db41c21e6f0f73bee654cd687d9ae84ee905c6d46c8dc139ff22959028a0cd0d006e6357eaf3684997ecf8f7b92fe44915b6a2b294e34106a40187e6b90ca074117ce39bb34adb9e06c7b859d0b75cb74526981ab3b6ce975016cdb18c64b4a0bb8c3dded0e6588d7faf43cb71934734cb29ccb9089abeb95849164c0dbd50afa0844f9f0f888f5d98dac8ee20a6ccc5a6d287d066da07bc183b144813be3c67e4a0aeaa105c4b1f3536963f44bedff164c932c970a87606cd44dd1995f3d5f32f43a0ff7f2792a2dd3a0440cb92aebe3ff9171b6822d1bf3905deee7c448cbd178df5a0bbb4a30dd84dd6d301498a0a353df0062a232670e2da5e1cdd48ec23fc5fd23ca0fc1358c994f2e1b55a158021c24494d4c91c5e0e4d5eae2995e058b13adcbb05a063c0ae53e9acf888e0f481d8142539d7b35edaacf633f3b1bc7cb813d476c821a008952b92e181b05afaf5515fc885dcd20cd9fc6eee0e558f504f76e08feb149ba02e9c3743f7e68a9f45b4967e29...
 ETH: RECV:           I500: 0xf901f1a00227242fc68ba11508f0281955260f6e83047813e4b396f15859785cd140cb38a01530cd144b1279d6793ec93ce7398bccc0ea495e93de4fe9eb360d790892eff2a0b0b2eb186eab9c6c5e85849546eb03c5c14344311b56ffb4f2eec6ba24d86011a005907bf762d3dccfe4b837ea9bf8986a40d8437460f37ab1797fd296ce11af0aa03bd826be6fa0b8b94f70a416d983bb13f89bb1e7c8beeec68fb606a83731ea8ba0c854f4bca8a1e166c86c48743c38c3086475256495ed5a45d510a19d38f6e24ca0b75b916508ae302040b03c13aacdaa08a3da4bbb6de37a8903249d168e11a427a0afd9f37c2f6de7ede16e719c0abf25b538ce864a23107f4fc6bbce57ff4c7726a00723d0a49f99a02b7df3ef00481d68e3039e8d91a6ec35469ca55353f14f29d3a0515fa8681d0bb5be4be790639e9c1b7b4b23a663c1da4cc28d23389a296c9c4780a0f49d7be086e62548d0efd728f8c0f992bc411efedfafb970ca81d743e6554b19a079011dfc7a0c0d395f2b1189ddd0b37c881a1795038f40ae717fb213630f750aa0b9a06777e72e8ec747ebf5f7ddd44ce8faa425b16c7578eaff665072c0ff0adea002f29b2f9e2cd183b5c64cfeab74baa38595a491276b020258cf65738abc764da05af330b2d8e30e35080ecc8a369255fb178f3d5494c44d98ea76f771d36990a780
 ETH: RECV:           I147: 0xf891a0646c4869356fa863975773e426530e944debfe815af65e5b4b486f7199b4a001808080a0fde83b87df3f275af35f64a8999ddc908132f9cccf7743365e3ed8feb205b90aa02a1e54363f1736a73ea82fc0df7150bd9f03d536cd89f6603f18b699e751c709808080808080a0a67fc8aba92c88b7ea317c4c2ebf23cd34cdc3f0701f8166376a75b48ef951fc80808080
 ETH: RECV:         ]
 ETH: RECV:         I  0: 0x
 ETH: RECV:         I  0: 0x
 ETH: RECV:         I 32: 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
 ETH: RECV:         I 32: 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
 ETH: RECV:       ]
 ETH: RECV:     ]
 ETH: RECV:   ]
 ETH: RECV: ]
*/
