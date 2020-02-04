//
//  BRCryptoCommonTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import XCTest
import Foundation
@testable import BRCrypto

class BRCryptoCommonTests: XCTestCase {

    override func setUp() { }

    override func tearDown() { }

    func testKey () {
        var s: String!
        var t: String!
        //var d: Data!
        var k: Key!
        var l: Key!
        var p: Bool!
        var n: Data!

        //
        // Password Proected
        //

        // mainnet (https://bitcoinpaperwallet.com/bitcoinpaperwallet/generate-wallet.html?design=alt-testnet)
        s = "6PRPGFR3vou5h9VXHVTUpnDisZpnKik5c7zWJrw8abi4AYW8fy4uPFYFXk"
        p = Key.isProtected(asPrivate: s)
        XCTAssertTrue(p)
        k = Key.createFromString(asPrivate: s, withPassphrase: "hodl")
        XCTAssertNotNil(k)
        XCTAssertEqual ("5KYkuC2SX6twF8C4yhDRJsy9tgBnn9aFsEXgaMLwRciopuRnBfT", k.encodeAsPrivate)
        XCTAssertNotNil (Key.createFromString(asPrivate: k.encodeAsPrivate))

        // testnet (https://bitcoinpaperwallet.com/bitcoinpaperwallet/generate-wallet.html?design=alt-testnet)
        s = "6PRVDGjn5m1Tj6wbP8kG6YozjE5qBHxE9BPfF8HxZHLdd1tAkENLjjsQve"
        p = Key.isProtected(asPrivate: s)
        XCTAssertTrue(p)
        k = Key.createFromString(asPrivate: s, withPassphrase: "hodl")
        XCTAssertNotNil(k)
        XCTAssertEqual ("92HBCaQbnqkGkSm8z2rxKA3DRdSrsniEswiWFe5CQXdfPqThR9N", k.encodeAsPrivate)
        XCTAssertNotNil (Key.createFromString(asPrivate: k.encodeAsPrivate))

        s = "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF"
        p = Key.isProtected(asPrivate: s)
        XCTAssertFalse(p)
        k = Key.createFromString(asPrivate: s, withPassphrase: "hodl")
        XCTAssertNil(k)

        s = "KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL"
        p = Key.isProtected(asPrivate: s)
        XCTAssertFalse(p)
        k = Key.createFromString(asPrivate: s, withPassphrase: "hodl")
        XCTAssertNil(k)

        //
        // Uncompressed
        //

        s = "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF"
        k = Key.createFromString(asPrivate: s)
        XCTAssertNotNil(k)
        XCTAssertTrue (k.hasSecret)
        XCTAssertEqual (s, k.encodeAsPrivate)
        XCTAssertNotNil (Key.createFromString(asPrivate: k.encodeAsPrivate))

        //
        // Compressed
        //

        s = "KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL"
        k = Key.createFromString(asPrivate: s)
        XCTAssertNotNil(k)
        XCTAssertEqual (s, k.encodeAsPrivate)
        l = Key.createFromString (asPrivate: k.encodeAsPrivate)
        XCTAssertTrue (k.privateKeyMatch(l))
        l = Key (secret: k.secret)
        XCTAssertTrue (k.privateKeyMatch(l))


        t = k.encodeAsPublic
        l = Key.createFromString(asPublic: t)
        XCTAssertNotNil (l)
        XCTAssertFalse (l.hasSecret)
        XCTAssertEqual (t, l.encodeAsPublic)

        XCTAssertTrue (l.publicKeyMatch(k))

        //
        // Bad Key
        //
        s = "XyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL"
        k = Key.createFromString(asPrivate: s)
        XCTAssertNil(k)

        //
        // Phrase
        //
        let phrases = [
            // See bitcoin/test.c
            "letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
            "legal winner thank year wave sausage worth useful legal winner thank yellow",
            "jelly better achieve collect unaware mountain thought cargo oxygen act hood bridge",
            "afford alter spike radar gate glance object seek swamp infant panel yellow",
            "turtle front uncle idea crush write shrug there lottery flower risk shell",
            "board flee heavy tunnel powder denial science ski answer betray cargo cat",
            //
            "ginger settle marine tissue robot crane night number ramp coast roast critic",
         ]

        phrases.forEach { (phrase) in
            // BIP39
            k = Key.createFrom (phrase: phrase, words: BRCryptoAccountTests.words)
            XCTAssertNotNil(k)
            XCTAssertNotNil (Key.createFromString(asPrivate: k.encodeAsPrivate))
            k = Key.createFrom (phrase: phrase, words: nil)
            XCTAssertNil(k)
            k = Key.createFrom (phrase: "not-a-chance", words: BRCryptoAccountTests.words)
            XCTAssertNil(k)

            // BIP32ApiAuth
            k = Key.createForBIP32ApiAuth (phrase: phrase, words: BRCryptoAccountTests.words)
            XCTAssertNotNil(k)
            XCTAssertNotNil (Key.createFromString(asPrivate: k.encodeAsPrivate))
            k = Key.createForBIP32ApiAuth (phrase: phrase, words: nil)
            XCTAssertNil(k)
            k = Key.createForBIP32ApiAuth (phrase: "not-a-chance", words: BRCryptoAccountTests.words)
            XCTAssertNil(k)

            // BIP32BitID
            k = Key.createForBIP32BitID (phrase: phrase,
                                         index: 2,
                                         uri: "some uri",
                                         words: BRCryptoAccountTests.words)
            XCTAssertNotNil(k)
            XCTAssertNotNil (Key.createFromString(asPrivate: k.encodeAsPrivate))
            k = Key.createForBIP32BitID (phrase: phrase,
                                         index: 2,
                                         uri: "some uri",
                                         words: nil)
            XCTAssertNil(k)
            k = Key.createForBIP32BitID (phrase: "not-a-chance",
                                         index: 2,
                                         uri: "some uri",
                                         words: BRCryptoAccountTests.words)
            XCTAssertNil(k)
        }

        // Pigeon
        s = "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF"
        k = Key.createFromString(asPrivate: s)
        n = "nonce".data(using: .utf8)
        l = Key.createForPigeonFrom(key: k, nonce: n)
        XCTAssertNotNil (Key.createFromString (asPrivate: l.encodeAsPrivate))
        XCTAssertNotNil(l)
    }

    func testHasher() {
        var d: Data!
        var a: Data!

        // SHA1
        d = "Free online SHA1 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data ([0x6f, 0xc2, 0xe2, 0x51, 0x72, 0xcb, 0x15, 0x19, 0x3c, 0xb1, 0xc6, 0xd4, 0x8f, 0x60, 0x7d, 0x42, 0xc1, 0xd2, 0xa2, 0x15])
        XCTAssertEqual (a, CoreHasher.sha1.hash(data: d))

        // sha256
        d = "Free online SHA256 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data ([0x43, 0xfd, 0x9d, 0xeb, 0x93, 0xf6, 0xe1, 0x4d, 0x41, 0x82, 0x66, 0x04, 0x51, 0x4e, 0x3d, 0x78,
                   0x73, 0xa5, 0x49, 0xac, 0x87, 0xae, 0xbe, 0xbf, 0x3d, 0x1c, 0x10, 0xad, 0x6e, 0xb0, 0x57, 0xd0])
        XCTAssertEqual (a, CoreHasher.sha256.hash(data: d))

        // sha224
        d = "Free online SHA224 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data([0x09, 0xcd, 0xa9, 0x39, 0xab, 0x1d, 0x6e, 0x7c, 0x3f, 0x81, 0x3b, 0xa2, 0x3a, 0xf3, 0x4b, 0xdf,
                  0xe9, 0x35, 0x50, 0x6d, 0xc4, 0x92, 0xeb, 0x77, 0xd8, 0x22, 0x6a, 0x64])
        XCTAssertEqual (a, CoreHasher.sha224.hash(data: d))

        // sha256_2
        d = "Free online SHA256_2 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data([0xe3, 0x9a, 0xee, 0xfe, 0xd2, 0x39, 0x02, 0xd7, 0x81, 0xef, 0xdc, 0x83, 0x8a, 0x0b, 0x5d, 0x16,
                  0xc0, 0xab, 0x90, 0x1d, 0xc9, 0x26, 0xbb, 0x6e, 0x2c, 0x1e, 0xdf, 0xb9, 0xc1, 0x8d, 0x89, 0xb8])
        XCTAssertEqual (a, CoreHasher.sha256_2.hash(data: d))

        // sha384
        d = "Free online SHA384 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data([0xef, 0x82, 0x38, 0x77, 0xa4, 0x66, 0x4c, 0x96, 0x41, 0xc5, 0x3a, 0xc2, 0x05, 0x59, 0xc3, 0x4b,
                  0x5d, 0x2c, 0x67, 0x94, 0x77, 0xde, 0x22, 0xff, 0xfa, 0xb3, 0x51, 0xe5, 0xe3, 0x3e, 0xa5, 0x3e,
                  0x42, 0x36, 0x15, 0xe1, 0xee, 0x3c, 0x85, 0xe0, 0xd7, 0xfa, 0xcb, 0x84, 0xdf, 0x2b, 0xa2, 0x17])
        XCTAssertEqual (a, CoreHasher.sha384.hash(data: d))

        // sha512
        d = "Free online SHA512 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data ([0x04, 0xf1, 0x15, 0x41, 0x35, 0xee, 0xcb, 0xe4, 0x2e, 0x9a, 0xdc, 0x8e, 0x1d, 0x53, 0x2f, 0x9c,
                   0x60, 0x7a, 0x84, 0x47, 0xb7, 0x86, 0x37, 0x7d, 0xb8, 0x44, 0x7d, 0x11, 0xa5, 0xb2, 0x23, 0x2c,
                   0xdd, 0x41, 0x9b, 0x86, 0x39, 0x22, 0x4f, 0x78, 0x7a, 0x51, 0xd1, 0x10, 0xf7, 0x25, 0x91, 0xf9,
                   0x64, 0x51, 0xa1, 0xbb, 0x51, 0x1c, 0x4a, 0x82, 0x9e, 0xd0, 0xa2, 0xec, 0x89, 0x13, 0x21, 0xf3])
        XCTAssertEqual (a, CoreHasher.sha512.hash(data: d))

        // rmd160
        d = "Free online RIPEMD160 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
       a = Data ([0x95, 0x01, 0xa5, 0x6f, 0xb8, 0x29, 0x13, 0x2b, 0x87, 0x48, 0xf0, 0xcc, 0xc4, 0x91, 0xf0, 0xec, 0xbc, 0x7f, 0x94, 0x5b])
        XCTAssertEqual (a, CoreHasher.rmd160.hash(data: d))

        // hash160
        d = "Free online HASH160 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data([0x62, 0x0a, 0x75, 0x2d, 0x20, 0x09, 0xd4, 0xc6, 0x59, 0x8b, 0x7f, 0x63, 0x4d, 0x34, 0xc5, 0xec, 0xd5, 0x23, 0x36, 0x72])
        XCTAssertEqual (a, CoreHasher.hash160.hash(data: d))

        // sha3
        d = "abc"
            .data(using: String.Encoding.utf8)
        a = Data ([0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2, 0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd, 0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b, 0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32])
        XCTAssertEqual (a, CoreHasher.sha3.hash(data: d))

        // keccak256
        d = ""
            .data(using: String.Encoding.utf8)
        a = Data ([0xc5, 0xd2, 0x46, 0x01, 0x86, 0xf7, 0x23, 0x3c, 0x92, 0x7e, 0x7d, 0xb2, 0xdc, 0xc7, 0x03, 0xc0, 0xe5, 0x00, 0xb6, 0x53, 0xca, 0x82, 0x27, 0x3b, 0x7b, 0xfa, 0xd8, 0x04, 0x5d, 0x85, 0xa4, 0x70])
        XCTAssertEqual (a, CoreHasher.keccak256.hash(data: d))

        // md5
        d = "Free online MD5 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
       a = Data ([0x0b, 0x3b, 0x20, 0xea, 0xf1, 0x69, 0x64, 0x62, 0xf5, 0x0d, 0x1a, 0x3b, 0xbd, 0xd3, 0x0c, 0xef])
        XCTAssertEqual (a, CoreHasher.md5.hash(data: d))

    }

    func testEncoder () {
        var d: Data!
        var a: String!
        var r: String!
        var s: String!

        // HEX
        d = Data([0xde, 0xad, 0xbe, 0xef])
        a = "deadbeef"
        r = CoreCoder.hex.encode(data: d)
        XCTAssertEqual (a, r)
        XCTAssertEqual (d, CoreCoder.hex.decode(string: r))

        s = "#&$@*^(*#!^"
        XCTAssertNil (CoreCoder.hex.decode(string: s))

        // base58
        s = "#&$@*^(*#!^"
        XCTAssertNil (CoreCoder.base58.decode(string: s))

        s = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
        d = CoreCoder.base58.decode(string: s)
        XCTAssertEqual(s, CoreCoder.base58.encode(data: d))

        s = "z";
        d = CoreCoder.base58.decode(string: s)
        XCTAssertEqual(s, CoreCoder.base58.encode(data: d))

        //  base58check
        d = Data([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        s = CoreCoder.base58check.encode(data: d)
        XCTAssertEqual (d, CoreCoder.base58check.decode(string: s))

        print ("one")
        d = Data([0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF])
        s = CoreCoder.base58check.encode(data: d)
        XCTAssertEqual (d, CoreCoder.base58check.decode(string: s))

        s = "#&$@*^(*#!^"
        XCTAssertNil (CoreCoder.base58check.decode(string: s))
    }

    func testEncryptor () {
        var k: Data!
        var d: Data!
        var a: Data!

        // aes-ecb
        k = Data([0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]) // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        d = Data ([0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef, 0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10])
        a = CoreCipher.aes_ecb(key: k).encrypt(data: d)
        XCTAssertEqual (d, CoreCipher.aes_ecb(key: k).decrypt(data: a))

        // cha-cha
        let secret: Key.Secret = (0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f)
        let key = Key (secret: secret)
        let nonce12: Data = Data([0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47])
        let ad: Data = Data([0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7])
        let msg: Data! = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it."
            .data (using: .utf8)
        let alg = CoreCipher.chacha20_poly1305 (key: key, nonce12: nonce12, ad: ad)

        let cipher = Data([0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2, 0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe, 0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6, 0x3d, 0xbe, 0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b, 0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36, 0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c, 0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58, 0xfa, 0xb3, 0x24, 0xe4, 0xfa, 0xd6, 0x75, 0x94, 0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc, 0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d, 0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b, 0x61, 0x16, 0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a, 0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91])
        XCTAssertEqual (cipher, alg.encrypt(data: msg))
        XCTAssertEqual (msg, alg.decrypt(data: cipher))

        // pigeon
        let pubKey = Key.createFromString(asPublic: "02d404943960a71535a79679f1cf1df80e70597c05b05722839b38ebc8803af517")!
        let pigeon = CoreCipher.pigeon (privKey: key, pubKey: pubKey, nonce12: nonce12)
        let pigeonCipher = pigeon.encrypt(data: msg)!
        XCTAssertEqual (pigeon.decrypt(data: pigeonCipher), msg)
    }

    func testSigner () {
        var msg: String!
        var digest: Data!
        var signature: Data!
        var signer: CoreSigner!
        var answer: Data!

        let secret: Key.Secret = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
        let key = Key (secret: secret)

        // Basic DER-encoded
        msg = "How wonderful that we have met with a paradox. Now we have some hope of making progress."
        digest = CoreHasher.sha256.hash(data: msg.data(using: .utf8)!)
        signer = CoreSigner.basicDER
        signature = signer.sign(data32: digest, using: key)

        answer = Data([0x30, 0x45, 0x02, 0x21, 0x00, 0xc0, 0xda, 0xfe, 0xc8, 0x25, 0x1f, 0x1d, 0x50, 0x10, 0x28, 0x9d, 0x21, 0x02, 0x32, 0x22, 0x0b, 0x03, 0x20, 0x2c, 0xba, 0x34, 0xec, 0x11, 0xfe, 0xc5, 0x8b, 0x3e, 0x93, 0xa8, 0x5b, 0x91, 0xd3, 0x02, 0x20, 0x75, 0xaf, 0xdc, 0x06, 0xb7, 0xd6, 0x32, 0x2a, 0x59, 0x09, 0x55, 0xbf, 0x26, 0x4e, 0x7a, 0xaa, 0x15, 0x58, 0x47, 0xf6, 0x14, 0xd8, 0x00, 0x78, 0xa9, 0x02, 0x92, 0xfe, 0x20, 0x50, 0x64, 0xd3])
        XCTAssertEqual(answer, signature)

        // Basic raw
        digest = CoreHasher.sha256.hash(data: msg.data(using: .utf8)!)
        signer = CoreSigner.basicJOSE
        signature = signer.sign(data32: digest, using: key)
        answer = CoreCoder.hex.decode(string: "c0dafec8251f1d5010289d210232220b03202cba34ec11fec58b3e93a85b91d375afdc06b7d6322a590955bf264e7aaa155847f614d80078a90292fe205064d3")
        XCTAssertEqual(signature.count, 64)
        XCTAssertEqual(signature, answer)

        // Compact
        signer = CoreSigner.compact

        msg = "foo"
        digest = CoreHasher.sha256.hash(data: msg.data(using: .utf8)!)

        signature = signer.sign(data32: digest, using: key)
        let keyPublic = signer.recover(data32: digest, signature: signature)
        XCTAssertNotNil(keyPublic)
        XCTAssertTrue(key.publicKeyMatch(keyPublic!))
    }

    func testCompactSigner() {
        let secrets = [
            "5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj",
            "5KC4ejrDjv152FGwP386VD1i2NYc5KkfSMyv1nGy1VGDxGHqVY3",
            "Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw", // compressed
            "L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g"
        ] // compressed

        let signatures = [
            "1c5dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6",
            "1c52d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d",
            "205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6",
            "2052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"
        ]

        let message =  "Very deterministic message".data(using: .utf8)
        let digest = CoreHasher.sha256_2.hash(data: message!)

        zip (secrets, signatures).forEach { secret, signature in
            let key = Key.createFromString (asPrivate: secret)
            XCTAssertNotNil(key)

            let outputSig = CoreSigner.compact.sign(data32: digest!, using: key!)!
            //print (CoreCoder.hex.encode(data: outputSig))
            XCTAssert(CoreCoder.hex.encode(data: outputSig) == signature)
        }
    }
}
