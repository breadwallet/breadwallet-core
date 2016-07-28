import unittest
import breadwallet


class IntTests(unittest.TestCase):
    def test_allocation(self):
        u512 = breadwallet.UInt512()
        self.assertNotEqual(u512, None)


class KeyTests(unittest.TestCase):
    def test_allocation(self):
        phrase = "axis husband project any sea patch drip tip spirit tide bring belt"
        mpk = breadwallet.MasterPubKey.from_phrase(phrase)
        self.assertNotEqual(mpk, None)

    def test_derive_key_allocate(self):
        phrase = "axis husband project any sea patch drip tip spirit tide bring belt"
        seed = breadwallet.derive_key(phrase)
        self.assertNotEqual(seed, None)

    def test_bitid_allocate(self):
        phrase = "inhale praise target steak garlic cricket paper better evil almost sadness crawl city banner amused fringe fox insect roast aunt prefer hollow basic ladder"
        seed = breadwallet.derive_key(phrase)
        key = breadwallet.Key.from_bitid(seed, 0, "http://bitid.bitcoin.blue/callback")
        self.assertNotEqual(key, None)

    def test_bitid_address(self):
        phrase = "inhale praise target steak garlic cricket paper better evil almost sadness crawl city banner amused fringe fox insect roast aunt prefer hollow basic ladder"
        seed = breadwallet.derive_key(phrase)
        key = breadwallet.Key.from_bitid(seed, 0, "http://bitid.bitcoin.blue/callback")
        self.assertNotEqual(key.address(), None)
        self.assertEqual(str(key.address()), "1J34vj4wowwPYafbeibZGht3zy3qERoUM1")


class WalletTests(unittest.TestCase):
    def test_allocation(self):
        phrase = "axis husband project any sea patch drip tip spirit tide bring belt"
        mpk = breadwallet.MasterPubKey.from_phrase(phrase)
        wallet = breadwallet.Wallet(mpk)
        self.assertNotEqual(wallet, None)
