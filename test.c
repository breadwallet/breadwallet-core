//
//  test.c
//
//  Created by Aaron Voisine on 8/14/15.
//  Copyright (c) 2015 breadwallet LLC
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

#include "BRHash.h"
#include "BRMerkleBlock.h"
#include "BRPeer.h"
#include "BRTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int BRHashTests()
{
    // test sha1
    
    uint8_t md[64];
    char *s;
    
    s = "Free online SHA1 Calculator, type text here...";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x6f\xc2\xe2\x51\x72\xcb\x15\x19\x3c\xb1\xc6\xd4\x8f\x60\x7d\x42\xc1\xd2\xa2\x15",
                    *(UInt160 *)md)) return 0;
        
    s = "this is some text to test the sha1 implementation with more than 64bytes of data since it's internal digest "
        "buffer is 64bytes in size";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x08\x51\x94\x65\x8a\x92\x35\xb2\x95\x1a\x83\xd1\xb8\x26\xb9\x87\xe9\x38\x5a\xa3",
                    *(UInt160 *)md)) return 0;
        
    s = "123456789012345678901234567890123456789012345678901234567890";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x24\x5b\xe3\x00\x91\xfd\x39\x2f\xe1\x91\xf4\xbf\xce\xc2\x2d\xcb\x30\xa0\x3a\xe6",
                    *(UInt160 *)md)) return 0;
    
    // a message exactly 64bytes long (internal buffer size)
    s = "1234567890123456789012345678901234567890123456789012345678901234";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\xc7\x14\x90\xfc\x24\xaa\x3d\x19\xe1\x12\x82\xda\x77\x03\x2d\xd9\xcd\xb3\x31\x03",
                    *(UInt160 *)md)) return 0;
    
    s = ""; // empty
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09",
                    *(UInt160 *)md)) return 0;
    
    s = "a";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x86\xf7\xe4\x37\xfa\xa5\xa7\xfc\xe1\x5d\x1d\xdc\xb9\xea\xea\xea\x37\x76\x67\xb8",
                    *(UInt160 *)md)) return 0;

    // test sha256
    
    s = "Free online SHA256 Calculator, type text here...";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\x43\xfd\x9d\xeb\x93\xf6\xe1\x4d\x41\x82\x66\x04\x51\x4e\x3d\x78\x73\xa5\x49\xac"
                    "\x87\xae\xbe\xbf\x3d\x1c\x10\xad\x6e\xb0\x57\xd0", *(UInt256 *)md)) return 0;
        
    s = "this is some text to test the sha256 implementation with more than 64bytes of data since it's internal "
        "digest buffer is 64bytes in size";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\x40\xfd\x09\x33\xdf\x2e\x77\x47\xf1\x9f\x7d\x39\xcd\x30\xe1\xcb\x89\x81\x0a\x7e"
                    "\x47\x06\x38\xa5\xf6\x23\x66\x9f\x3d\xe9\xed\xd4", *(UInt256 *)md)) return 0;
    
    s = "123456789012345678901234567890123456789012345678901234567890";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\xde\xcc\x53\x8c\x07\x77\x86\x96\x6a\xc8\x63\xb5\x53\x2c\x40\x27\xb8\x58\x7f\xf4"
                    "\x0f\x6e\x31\x03\x37\x9a\xf6\x2b\x44\xea\xe4\x4d", *(UInt256 *)md)) return 0;
    
    // a message exactly 64bytes long (internal buffer size)
    s = "1234567890123456789012345678901234567890123456789012345678901234";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\x67\x64\x91\x96\x5e\xd3\xec\x50\xcb\x7a\x63\xee\x96\x31\x54\x80\xa9\x5c\x54\x42"
                    "\x6b\x0b\x72\xbc\xa8\xa0\xd4\xad\x12\x85\xad\x55", *(UInt256 *)md)) return 0;
    
    s = ""; // empty
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9\x24\x27\xae\x41\xe4"
                    "\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52\xb8\x55", *(UInt256 *)md)) return 0;
    
    s = "a";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\xca\x97\x81\x12\xca\x1b\xbd\xca\xfa\xc2\x31\xb3\x9a\x23\xdc\x4d\xa7\x86\xef\xf8"
                    "\x14\x7c\x4e\x72\xb9\x80\x77\x85\xaf\xee\x48\xbb", *(UInt256 *)md)) return 0;

    // test sha512
    
    s = "Free online SHA512 Calculator, type text here...";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x04\xf1\x15\x41\x35\xee\xcb\xe4\x2e\x9a\xdc\x8e\x1d\x53\x2f\x9c\x60\x7a\x84\x47"
                    "\xb7\x86\x37\x7d\xb8\x44\x7d\x11\xa5\xb2\x23\x2c\xdd\x41\x9b\x86\x39\x22\x4f\x78\x7a\x51"
                    "\xd1\x10\xf7\x25\x91\xf9\x64\x51\xa1\xbb\x51\x1c\x4a\x82\x9e\xd0\xa2\xec\x89\x13\x21\xf3",
                    *(UInt512 *)md)) return 0;
    
    s = "this is some text to test the sha512 implementation with more than 128bytes of data since it's internal "
        "digest buffer is 128bytes in size";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x9b\xd2\xdc\x7b\x05\xfb\xbe\x99\x34\xcb\x32\x89\xb6\xe0\x6b\x8c\xa9\xfd\x7a\x55"
                    "\xe6\xde\x5d\xb7\xe1\xe4\xee\xdd\xc6\x62\x9b\x57\x53\x07\x36\x7c\xd0\x18\x3a\x44\x61\xd7"
                    "\xeb\x2d\xfc\x6a\x27\xe4\x1e\x8b\x70\xf6\x59\x8e\xbc\xc7\x71\x09\x11\xd4\xfb\x16\xa3\x90",
                    *(UInt512 *)md)) return 0;
    
    s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567"
        "8901234567890";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x0d\x9a\x7d\xf5\xb6\xa6\xad\x20\xda\x51\x9e\xff\xda\x88\x8a\x73\x44\xb6\xc0\xc7"
                    "\xad\xcc\x8e\x2d\x50\x4b\x4a\xf2\x7a\xaa\xac\xd4\xe7\x11\x1c\x71\x3f\x71\x76\x95\x39\x62"
                    "\x94\x63\xcb\x58\xc8\x61\x36\xc5\x21\xb0\x41\x4a\x3c\x0e\xdf\x7d\xc6\x34\x9c\x6e\xda\xf3",
                    *(UInt512 *)md)) return 0;
    
    //exactly 128bytes (internal buf size)
    s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567"
        "890123456789012345678";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x22\x2b\x2f\x64\xc2\x85\xe6\x69\x96\x76\x9b\x5a\x03\xef\x86\x3c\xfd\x3b\x63\xdd"
                    "\xb0\x72\x77\x88\x29\x16\x95\xe8\xfb\x84\x57\x2e\x4b\xfe\x5a\x80\x67\x4a\x41\xfd\x72\xee"
                    "\xb4\x85\x92\xc9\xc7\x9f\x44\xae\x99\x2c\x76\xed\x1b\x0d\x55\xa6\x70\xa8\x3f\xc9\x9e\xc6",
                    *(UInt512 *)md)) return 0;
    
    s = ""; // empty
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\xcf\x83\xe1\x35\x7e\xef\xb8\xbd\xf1\x54\x28\x50\xd6\x6d\x80\x07\xd6\x20\xe4\x05"
                    "\x0b\x57\x15\xdc\x83\xf4\xa9\x21\xd3\x6c\xe9\xce\x47\xd0\xd1\x3c\x5d\x85\xf2\xb0\xff\x83"
                    "\x18\xd2\x87\x7e\xec\x2f\x63\xb9\x31\xbd\x47\x41\x7a\x81\xa5\x38\x32\x7a\xf9\x27\xda\x3e",
                    *(UInt512 *)md)) return 0;
    
    s = "a";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x1f\x40\xfc\x92\xda\x24\x16\x94\x75\x09\x79\xee\x6c\xf5\x82\xf2\xd5\xd7\xd2\x8e"
                    "\x18\x33\x5d\xe0\x5a\xbc\x54\xd0\x56\x0e\x0f\x53\x02\x86\x0c\x65\x2b\xf0\x8d\x56\x02\x52"
                    "\xaa\x5e\x74\x21\x05\x46\xf3\x69\xfb\xbb\xce\x8c\x12\xcf\xc7\x95\x7b\x26\x52\xfe\x9a\x75",
                    *(UInt512 *)md)) return 0;
    
    // test ripemd160
    
    s = "Free online RIPEMD160 Calculator, type text here...";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x95\x01\xa5\x6f\xb8\x29\x13\x2b\x87\x48\xf0\xcc\xc4\x91\xf0\xec\xbc\x7f\x94\x5b",
                    *(UInt160 *)md)) return 0;
    
    s = "this is some text to test the ripemd160 implementation with more than 64bytes of data since it's internal "
        "digest buffer is 64bytes in size";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x44\x02\xef\xf4\x21\x57\x10\x6a\x5d\x92\xe4\xd9\x46\x18\x58\x56\xfb\xc5\x0e\x09",
                    *(UInt160 *)md)) return 0;
    
    s = "123456789012345678901234567890123456789012345678901234567890";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x00\x26\x3b\x99\x97\x14\xe7\x56\xfa\x5d\x02\x81\x4b\x84\x2a\x26\x34\xdd\x31\xac",
                    *(UInt160 *)md)) return 0;
    
    // a message exactly 64bytes long (internal buffer size)
    s = "1234567890123456789012345678901234567890123456789012345678901234";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\xfa\x8c\x1a\x78\xeb\x76\x3b\xb9\x7d\x5e\xa1\x4c\xe9\x30\x3d\x1c\xe2\xf3\x34\x54",
                    *(UInt160 *)md)) return 0;
    
    s = ""; // empty
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x9c\x11\x85\xa5\xc5\xe9\xfc\x54\x61\x28\x08\x97\x7e\xe8\xf5\x48\xb2\x25\x8d\x31",
                    *(UInt160 *)md)) return 0;
    
    s = "a";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x0b\xdc\x9d\x2d\x25\x6b\x3e\xe9\xda\xae\x34\x7b\xe6\xf4\xdc\x83\x5a\x46\x7f\xfe",
                    *(UInt160 *)md)) return 0;

    return 1;
}

int BRMerkleBlockTests()
{
    // block 10001 filtered to include only transactions 0, 1, 2, and 6
    char block[] = "\x01\x00\x00\x00\x06\xe5\x33\xfd\x1a\xda\x86\x39\x1f\x3f\x6c\x34\x32\x04\xb0\xd2\x78\xd4\xaa\xec\x1c"
    "\x0b\x20\xaa\x27\xba\x03\x00\x00\x00\x00\x00\x6a\xbb\xb3\xeb\x3d\x73\x3a\x9f\xe1\x89\x67\xfd\x7d\x4c\x11\x7e\x4c"
    "\xcb\xba\xc5\xbe\xc4\xd9\x10\xd9\x00\xb3\xae\x07\x93\xe7\x7f\x54\x24\x1b\x4d\x4c\x86\x04\x1b\x40\x89\xcc\x9b\x0c"
    "\x00\x00\x00\x08\x4c\x30\xb6\x3c\xfc\xdc\x2d\x35\xe3\x32\x94\x21\xb9\x80\x5e\xf0\xc6\x56\x5d\x35\x38\x1c\xa8\x57"
    "\x76\x2e\xa0\xb3\xa5\xa1\x28\xbb\xca\x50\x65\xff\x96\x17\xcb\xcb\xa4\x5e\xb2\x37\x26\xdf\x64\x98\xa9\xb9\xca\xfe"
    "\xd4\xf5\x4c\xba\xb9\xd2\x27\xb0\x03\x5d\xde\xfb\xbb\x15\xac\x1d\x57\xd0\x18\x2a\xae\xe6\x1c\x74\x74\x3a\x9c\x4f"
    "\x78\x58\x95\xe5\x63\x90\x9b\xaf\xec\x45\xc9\xa2\xb0\xff\x31\x81\xd7\x77\x06\xbe\x8b\x1d\xcc\x91\x11\x2e\xad\xa8"
    "\x6d\x42\x4e\x2d\x0a\x89\x07\xc3\x48\x8b\x6e\x44\xfd\xa5\xa7\x4a\x25\xcb\xc7\xd6\xbb\x4f\xa0\x42\x45\xf4\xac\x8a"
    "\x1a\x57\x1d\x55\x37\xea\xc2\x4a\xdc\xa1\x45\x4d\x65\xed\xa4\x46\x05\x54\x79\xaf\x6c\x6d\x4d\xd3\xc9\xab\x65\x84"
    "\x48\xc1\x0b\x69\x21\xb7\xa4\xce\x30\x21\xeb\x22\xed\x6b\xb6\xa7\xfd\xe1\xe5\xbc\xc4\xb1\xdb\x66\x15\xc6\xab\xc5"
    "\xca\x04\x21\x27\xbf\xaf\x9f\x44\xeb\xce\x29\xcb\x29\xc6\xdf\x9d\x05\xb4\x7f\x35\xb2\xed\xff\x4f\x00\x64\xb5\x78"
    "\xab\x74\x1f\xa7\x82\x76\x22\x26\x51\x20\x9f\xe1\xa2\xc4\xc0\xfa\x1c\x58\x51\x0a\xec\x8b\x09\x0d\xd1\xeb\x1f\x82"
    "\xf9\xd2\x61\xb8\x27\x3b\x52\x5b\x02\xff\x1a";
    BRMerkleBlock *b;
    
    b = BRMerkleBlockDeserialize((uint8_t *)block, sizeof(block));
    
    if (! UInt256Eq(b->blockHash,
                       UInt256Reverse(*(UInt256 *)"\x00\x00\x00\x00\x00\x00\x80\xb6\x6c\x91\x1b\xd5\xba\x14\xa7"
                                      "\x42\x60\x05\x73\x11\xea\xeb\x19\x82\x80\x2f\x70\x10\xf1\xa9\xf0\x90")))
        return 0;
    
    if (! BRMerkleBlockIsValid(b, (uint32_t)time(NULL))) return 0;
    
    if (! BRMerkleBlockContainsTxHash(b, *(UInt256 *)"\x4c\x30\xb6\x3c\xfc\xdc\x2d\x35\xe3\x32\x94\x21\xb9\x80\x5e"
                                      "\xf0\xc6\x56\x5d\x35\x38\x1c\xa8\x57\x76\x2e\xa0\xb3\xa5\xa1\x28\xbb")) return 0;
    
    if (BRMerkleBlockTxHashes(b, NULL, 0) != 4) return 0;
    
    UInt256 txHashes[BRMerkleBlockTxHashes(b, NULL, 0)];
    
    BRMerkleBlockTxHashes(b, txHashes, 4);
    
    if (! UInt256Eq(txHashes[0], *(UInt256 *)"\x4c\x30\xb6\x3c\xfc\xdc\x2d\x35\xe3\x32\x94\x21\xb9\x80\x5e\xf0\xc6"
                    "\x56\x5d\x35\x38\x1c\xa8\x57\x76\x2e\xa0\xb3\xa5\xa1\x28\xbb")) return 0;
    
    if (! UInt256Eq(txHashes[1], *(UInt256 *)"\xca\x50\x65\xff\x96\x17\xcb\xcb\xa4\x5e\xb2\x37\x26\xdf\x64\x98\xa9"
                    "\xb9\xca\xfe\xd4\xf5\x4c\xba\xb9\xd2\x27\xb0\x03\x5d\xde\xfb")) return 0;
    
    if (! UInt256Eq(txHashes[2], *(UInt256 *)"\xbb\x15\xac\x1d\x57\xd0\x18\x2a\xae\xe6\x1c\x74\x74\x3a\x9c\x4f\x78"
                    "\x58\x95\xe5\x63\x90\x9b\xaf\xec\x45\xc9\xa2\xb0\xff\x31\x81")) return 0;
    
    if (! UInt256Eq(txHashes[3], *(UInt256 *)"\xc9\xab\x65\x84\x48\xc1\x0b\x69\x21\xb7\xa4\xce\x30\x21\xeb\x22\xed"
                    "\x6b\xb6\xa7\xfd\xe1\xe5\xbc\xc4\xb1\xdb\x66\x15\xc6\xab\xc5")) return 0;
    
    //TODO: test a block with an odd number of tree rows both at the tx level and merkle node level

    //TODO:XXXX test BRMerkleBlockVerifyDifficulty()
    
    BRMerkleBlockFree(b);
    return 1;
}

int BRPeerTests()
{
    BRPeer peer = { UINT128_ZERO, 0, 0, 0, 0, NULL };
    
    return peer.port;
}

int main(int argc, const char *argv[]) {
    printf("BRHashTests...        %s\n", (BRHashTests()) ? "success" : "FAIL");
    printf("BRMerkleBlockTests... %s\n", (BRMerkleBlockTests()) ? "success" : "FAIL");
    return 0;
}
