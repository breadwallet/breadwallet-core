import hashlib
import unittest
import breadwallet


class IntTests(unittest.TestCase):
    def test_allocation_256(self):
        u256 = breadwallet.UInt256()
        self.assertNotEqual(u256, None)

    def test_from_hex_256(self):
        u256 = breadwallet.UInt256.from_hex('0000000000000000000000000000000000000000000000000000000000000001')
        self.assertNotEqual(u256, None)

    def test_from_hex_and_back_256(self):
        h = '0000000000000000000000000000000000000000000000000000000000000001'
        u256 = breadwallet.UInt256.from_hex(h)
        self.assertEqual(h, u256.hex)

    def test_from_hash_and_back_256(self):
        h = hashlib.sha256()
        h.update('test123'.encode('utf8'))
        u256 = breadwallet.UInt256.from_hash(h)
        self.assertEqual(h.hexdigest(), u256.hex)

    def test_allocation(self):
        u512 = breadwallet.UInt512()
        self.assertNotEqual(u512, None)


class AddressTests(unittest.TestCase):
    def test_allocation(self):
        addy = breadwallet.Address("1J34vj4wowwPYafbeibZGht3zy3qERoUM1")
        self.assertNotEqual(addy, None)

        with self.assertRaises(TypeError):
            addy2 = breadwallet.Address(2304203942340)

    def test_equality(self):
        addy1_s = "1J34vj4wowwPYafbeibZGht3zy3qERoUM1"
        addy1_o = breadwallet.Address(addy1_s)
        addy2_s = "1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX"
        addy2_o = breadwallet.Address(addy2_s)
        self.assertEqual(addy1_o, addy1_o)
        self.assertEqual(addy1_o, addy1_s)
        self.assertNotEqual(addy1_o, addy2_o)
        self.assertNotEqual(addy1_o, addy2_s)
        self.assertNotEqual(addy1_o, None) # allow none comparison
        with self.assertRaises(TypeError):
            addy1_o == 234234 # raises when trying to compare against anything other than a string/address/none

    def test_to_str(self):
        addy1_s = "1J34vj4wowwPYafbeibZGht3zy3qERoUM1"
        self.assertEqual(str(breadwallet.Address(addy1_s)), addy1_s)


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
        self.assertNotEqual(key.address, None)
        self.assertEqual(str(key.address), "1J34vj4wowwPYafbeibZGht3zy3qERoUM1")

    def test_privkeykey_is_valid(self):
        self.assertFalse(breadwallet.Key.privkey_is_valid("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRz"))
        self.assertTrue(breadwallet.Key.privkey_is_valid("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy"))

    def test_set_privkey(self):
        k = breadwallet.Key()
        k.privkey = 'S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy'
        self.assertEqual(k.address, '1CciesT23BNionJeXrbxmjc7ywfiyM4oLW')

    def test_sign(self):
        k = breadwallet.Key()
        k.secret = breadwallet.UInt256.from_hex('fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140')
        message = "Equations are more important to me, because politics is for the present, but an equation is something for eternity."
        h = hashlib.sha256()
        h.update(message.encode('utf8'))

        sig = k.sign(h.digest()) # can take either a hash or a bytes object
        sig2 = k.sign(h)
        sig3 = b'\x30\x44\x02\x20\x54\xc4\xa3\x3c\x64\x23\xd6\x89\x37\x8f\x16\x0a\x7f\xf8\xb6\x13\x30\x44\x4a\xbb\x58\xfb\x47\x0f\x96\xea\x16\xd9\x9d\x4a\x2f\xed\x02\x20\x07\x08\x23\x04\x41\x0e\xfa\x6b\x29\x43\x11\x1b\x6a\x4e\x0a\xaa\x7b\x7d\xb5\x5a\x07\xe9\x86\x1d\x1f\xb3\xcb\x1f\x42\x10\x44\xa5'
        self.assertEqual(len(sig), len(sig3))
        self.assertEqual(sig, sig3)
        self.assertEqual(sig, sig2)

    def test_verify(self):
        k = breadwallet.Key()
        k.secret = breadwallet.UInt256.from_hex('fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140')
        message = "Equations are more important to me, because politics is for the present, but an equation is something for eternity."
        h = hashlib.sha256()
        h.update(message.encode('utf8'))

        sig = k.sign(h)
        self.assertTrue(k.verify(h, sig))
        self.assertTrue(k.verify(h.digest(), sig)) # works with both hash objects or bytes objects


class TransactionTests(unittest.TestCase):
    def test_allocation(self):
        t = breadwallet.Transaction()
        self.assertNotEqual(t, None)


class WalletTests(unittest.TestCase):
    def _get_wallet(self):
        phrase = "axis husband project any sea patch drip tip spirit tide bring belt"
        mpk = breadwallet.MasterPubKey.from_phrase(phrase)
        return breadwallet.Wallet(mpk)

    def test_allocation(self):
        phrase = "axis husband project any sea patch drip tip spirit tide bring belt"
        mpk = breadwallet.MasterPubKey.from_phrase(phrase)
        wallet = breadwallet.Wallet(mpk)
        self.assertNotEqual(wallet, None)

    def test_callback_on_balance_changed_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_balance_changed, None)
        wallet.on_balance_changed = None
        self.assertEqual(wallet.on_balance_changed, None)
        wallet.on_balance_changed = cb_a
        self.assertEqual(wallet.on_balance_changed, cb_a)
        wallet.on_balance_changed = None
        self.assertEqual(wallet.on_balance_changed, None)
        wallet.on_balance_changed = cb_a
        wallet.on_balance_changed = cb_b
        self.assertEqual(wallet.on_balance_changed, cb_b)

    def test_callback_on_tx_added_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_tx_added, None)
        wallet.on_tx_added = None
        self.assertEqual(wallet.on_tx_added, None)
        wallet.on_tx_added = cb_a
        self.assertEqual(wallet.on_tx_added, cb_a)
        wallet.on_tx_added = None
        self.assertEqual(wallet.on_tx_added, None)
        wallet.on_tx_added = cb_a
        wallet.on_tx_added = cb_b
        self.assertEqual(wallet.on_tx_added, cb_b)

    def test_callback_on_tx_updated_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_tx_updated, None)
        wallet.on_tx_updated = None
        self.assertEqual(wallet.on_tx_updated, None)
        wallet.on_tx_updated = cb_a
        self.assertEqual(wallet.on_tx_updated, cb_a)
        wallet.on_tx_updated = None
        self.assertEqual(wallet.on_tx_updated, None)
        wallet.on_tx_updated = cb_a
        wallet.on_tx_updated = cb_b
        self.assertEqual(wallet.on_tx_updated, cb_b)

    def test_callback_on_tx_deleted_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_tx_deleted, None)
        wallet.on_tx_deleted = None
        self.assertEqual(wallet.on_tx_deleted, None)
        wallet.on_tx_deleted = cb_a
        self.assertEqual(wallet.on_tx_deleted, cb_a)
        wallet.on_tx_deleted = None
        self.assertEqual(wallet.on_tx_deleted, None)
        wallet.on_tx_deleted = cb_a
        wallet.on_tx_deleted = cb_b
        self.assertEqual(wallet.on_tx_deleted, cb_b)

    def test_get_receive_address(self):
        wallet = self._get_wallet()
        self.assertNotEqual(wallet.receive_address, None)
        self.assertEqual(wallet.receive_address, wallet.receive_address)

    def test_get_change_address(self):
        wallet = self._get_wallet()
        self.assertNotEqual(wallet.change_address, None)
        self.assertEqual(wallet.change_address, wallet.change_address)
