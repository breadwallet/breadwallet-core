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

    def test_callback_on_sync_started_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_sync_started, None)
        wallet.on_sync_started = None
        self.assertEqual(wallet.on_sync_started, None)
        wallet.on_sync_started = cb_a
        self.assertEqual(wallet.on_sync_started, cb_a)
        wallet.on_sync_started = None
        self.assertEqual(wallet.on_sync_started, None)
        wallet.on_sync_started = cb_a
        wallet.on_sync_started = cb_b
        self.assertEqual(wallet.on_sync_started, cb_b)

    def test_callback_on_sync_succeeded_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_sync_succeeded, None)
        wallet.on_sync_succeeded = None
        self.assertEqual(wallet.on_sync_succeeded, None)
        wallet.on_sync_succeeded = cb_a
        self.assertEqual(wallet.on_sync_succeeded, cb_a)
        wallet.on_sync_succeeded = None
        self.assertEqual(wallet.on_sync_succeeded, None)
        wallet.on_sync_succeeded = cb_a
        wallet.on_sync_succeeded = cb_b
        self.assertEqual(wallet.on_sync_succeeded, cb_b)

    def test_callback_on_sync_failed_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_sync_failed, None)
        wallet.on_sync_failed = None
        self.assertEqual(wallet.on_sync_failed, None)
        wallet.on_sync_failed = cb_a
        self.assertEqual(wallet.on_sync_failed, cb_a)
        wallet.on_sync_failed = None
        self.assertEqual(wallet.on_sync_failed, None)
        wallet.on_sync_failed = cb_a
        wallet.on_sync_failed = cb_b
        self.assertEqual(wallet.on_sync_failed, cb_b)

    def test_callback_on_tx_status_update_setters_and_getters(self):
        wallet = self._get_wallet()

        def cb_a(): pass
        def cb_b(): pass

        self.assertEqual(wallet.on_tx_status_update, None)
        wallet.on_tx_status_update = None
        self.assertEqual(wallet.on_tx_status_update, None)
        wallet.on_tx_status_update = cb_a
        self.assertEqual(wallet.on_tx_status_update, cb_a)
        wallet.on_tx_status_update = None
        self.assertEqual(wallet.on_tx_status_update, None)
        wallet.on_tx_status_update = cb_a
        wallet.on_tx_status_update = cb_b
        self.assertEqual(wallet.on_tx_status_update, cb_b)
