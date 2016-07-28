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

    def test_privkeykey_is_valid(self):
        self.assertFalse(breadwallet.Key.privkey_is_valid("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRz"))
        self.assertTrue(breadwallet.Key.privkey_is_valid("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy"))


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
