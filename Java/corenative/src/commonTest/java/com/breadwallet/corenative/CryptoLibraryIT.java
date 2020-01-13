package com.breadwallet.corenative;

import com.breadwallet.corenative.crypto.BRCryptoAccount;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoUnit;
import com.google.common.io.Files;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Library;
import com.sun.jna.Native;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.Calendar;
import java.util.TimeZone;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class CryptoLibraryIT {

    private String paperKey;
    private String uids;
    private File coreDataDir;
    private int epoch;

    @Before
    public void setup() {
        // this is a compromised testnet paperkey
        paperKey = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        coreDataDir = Files.createTempDir();
        Calendar calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC"));
        calendar.set(2017, /* September */ 8, 6);
        epoch = (int) TimeUnit.MILLISECONDS.toSeconds(calendar.getTimeInMillis());

        coreDirClear();
        coreDirCreate();
    }

    @After
    public void teardown() {
        coreDirClear();
    }

    @Test
    public void testLoad() {
        assertNotNull(TestCryptoLibrary.INSTANCE);
    }

    // Bitcoin

    @Test
    public void testBitcoin() {
        assertEquals(1, TestCryptoLibrary.INSTANCE.BRRunTests());
    }

    @Test
    public void testBitcoinSyncOne() {
        int success = 0;

        success = TestCryptoLibrary.INSTANCE.BRRunTestsSync (paperKey, 1, 1);
        assertEquals(1, success);
    }

    @Test
    public void testBitcoinWalletManagerSync () {
        coreDirClear();
        int success = 0;

        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSync (paperKey, coreDataDir.getAbsolutePath(), 1, 1);
        assertEquals(1, success);

        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSync (paperKey, coreDataDir.getAbsolutePath(), 1, 1);
        assertEquals(1, success);
    }

    @Test
    public void testBitcoinWalletManagerSyncStressBtc() {
        int success;

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(
                paperKey,
                coreDataDir.getAbsolutePath(),
                epoch,
                500000,
                1,
                1
        );
        assertEquals(1, success);

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(
                paperKey,
                coreDataDir.getAbsolutePath(),
                epoch,
                1500000,
                1,
                0
        );
        assertEquals(1, success);
    }

    @Test
    public void testBitcoinWalletManagerSyncStressBch() {
        int success;

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(
                paperKey,
                coreDataDir.getAbsolutePath(),
                epoch,
                500000,
                0,
                1
        );
        assertEquals(1, success);

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(
                paperKey,
                coreDataDir.getAbsolutePath(),
                epoch,
                1500000,
                0,
                0
        );
        assertEquals(1, success);
    }

    // Support

    @Test
    public void testBitcoinSupport() {
        assertEquals(1, TestCryptoLibrary.INSTANCE.BRRunSupTests());
    }

    // Crypto

    @Test
    public void testCrypto() {
        TestCryptoLibrary.INSTANCE.runCryptoTests();
    }

    @Test
    public void testCryptoWithAccountAndNetworkBtc() {
        BRCryptoAccount account = BRCryptoAccount.createFromPhrase(
                paperKey.getBytes(StandardCharsets.UTF_8),
                UnsignedLong.valueOf(epoch),
                uids
        );

        try {
            BRCryptoNetwork network;

            network = createBitcoinNetwork(true, 500000);
            try {
                coreDirClear();
                int success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                        account,
                        network,
                        coreDataDir.getAbsolutePath());
                assertEquals(1, success);
            } finally {
                network.give();
            }

            network = createBitcoinNetwork(false, 1500000);
            try {
                coreDirClear();
                int success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                        account,
                        network,
                        coreDataDir.getAbsolutePath());
                assertEquals(1, success);
            } finally {
                network.give();
            }

        } finally {
            account.give();
        }
    }

    @Test
    public void testCryptoWithAccountAndNetworkBch() {
        BRCryptoAccount account = BRCryptoAccount.createFromPhrase(
                paperKey.getBytes(StandardCharsets.UTF_8),
                UnsignedLong.valueOf(epoch),
                uids
        );

        try {
            BRCryptoNetwork network;

            network = createBitcoinCashNetwork(true, 500000);
            try {
                coreDirClear();
                int success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                        account,
                        network,
                        coreDataDir.getAbsolutePath());
                assertEquals(1, success);
            } finally {
                network.give();
            }

            network = createBitcoinCashNetwork(false, 1500000);
            try {
                coreDirClear();
                int success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                        account,
                        network,
                        coreDataDir.getAbsolutePath());
                assertEquals(1, success);
            } finally {
                network.give();
            }
        } finally {
            account.give();
        }
    }

    @Test
    public void testCryptoWithAccountAndNetworkEth() {
        BRCryptoAccount account = BRCryptoAccount.createFromPhrase(
                paperKey.getBytes(StandardCharsets.UTF_8),
                UnsignedLong.valueOf(epoch),
                uids
        );

        try {
            BRCryptoNetwork network;

            network = createEthereumNetwork(true, 8000000);
            try {
                coreDirClear();
                int success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                        account,
                        network,
                        coreDataDir.getAbsolutePath());
                assertEquals(1, success);

                network.give();
            } finally {
                network.give();
            }

            network = createEthereumNetwork(false, 4500000);
            try {
                coreDirClear();
                int success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                        account,
                        network,
                        coreDataDir.getAbsolutePath());
                assertEquals(1, success);
            } finally {
                network.give();
            }

        } finally {
            account.give();
        }
    }

    // Ethereum

    @Test
    public void testEthereumRlp () {
        TestCryptoLibrary.INSTANCE.runRlpTests();
    }

    @Test
    public void testEthereumUtil () {
        TestCryptoLibrary.INSTANCE.runUtilTests();
    }

    @Test
    public void testEthereumEvent () {
        TestCryptoLibrary.INSTANCE.runEventTests ();
    }

    @Test
    public void testEthereumBase () {
        TestCryptoLibrary.INSTANCE.runBaseTests();
    }

    @Test
    public void testEthereumBlockChain () {
        TestCryptoLibrary.INSTANCE.runBcTests();
    }

    @Test
    public void testEthereumContract () {
        TestCryptoLibrary.INSTANCE.runContractTests ();
    }

    @Test
    public void testEthereumBasics() {
        TestCryptoLibrary.INSTANCE.runTests(0);
    }

    @Test
    public void testEWM () {
        TestCryptoLibrary.INSTANCE.runEWMTests(paperKey, coreDataDir.getAbsolutePath());
    }

    @Test
    public void testLES () {
        TestCryptoLibrary.INSTANCE.runLESTests(paperKey);
        TestCryptoLibrary.INSTANCE.runNodeTests();
    }

    // Ripple

    @Test
    public void testRipple() {
        TestCryptoLibrary.INSTANCE.runRippleTest();
    }

    // Test Bits

    private void coreDirCreate() {
        coreDataDir.mkdirs();
    }

    private void coreDirClear() {
        deleteRecursively(coreDataDir);
    }

    private static void deleteRecursively (File file) {
        if (file.isDirectory()) {
            for (File child : file.listFiles()) {
                deleteRecursively(child);
            }
        }
        file.delete();
    }

    private static BRCryptoNetwork createBitcoinNetwork(boolean isMainnet, long blockHeight) {
        BRCryptoCurrency btc = null;
        BRCryptoUnit satoshis = null;
        BRCryptoUnit bitcoins = null;
        BRCryptoAmount factor = null;
        BRCryptoNetworkFee fee = null;
        BRCryptoNetwork network = null;

        try {

            network = BRCryptoNetwork.findBuiltin("bitcoin-" + (isMainnet ? "mainnet" : "testnet")).get();

            btc = BRCryptoCurrency
                    .create("bitcoin", "bitcoin", "btc", "native", null);

            satoshis = BRCryptoUnit
                    .createAsBase(btc, "sat", "satoshi", "SAT");

            bitcoins = BRCryptoUnit
                    .create(btc, "btc", "bitcoin", "B", satoshis, UnsignedInteger.valueOf(8));

            factor = BRCryptoAmount
                    .create(1000, satoshis);

            fee = BRCryptoNetworkFee
                    .create(UnsignedLong.valueOf(30 * 1000), factor, satoshis);

            network.setHeight(UnsignedLong.valueOf(blockHeight));

            network.setCurrency(btc);
            network.addCurrency(btc, satoshis, bitcoins);

            network.addCurrencyUnit(btc, satoshis);
            network.addCurrencyUnit(btc, bitcoins);

            network.addFee(fee);

            return network.take();

        } finally {
            if (null != btc) btc.give();
            if (null != satoshis) satoshis.give();
            if (null != bitcoins) bitcoins.give();
            if (null != factor) factor.give();
            if (null != fee) fee.give();
            if (null != network) network.give();
        }
    }

    private static BRCryptoNetwork createBitcoinCashNetwork(boolean isMainnet, long blockHeight) {
        BRCryptoCurrency btc = null;
        BRCryptoUnit satoshis = null;
        BRCryptoUnit bitcoins = null;
        BRCryptoAmount factor = null;
        BRCryptoNetworkFee fee = null;
        BRCryptoNetwork network = null;

        try {

            network = BRCryptoNetwork.findBuiltin("bitcoincash-" + (isMainnet ? "mainnet" : "testnet")).get();

            btc = BRCryptoCurrency
                    .create("bitcoin-cash", "bitcoin cash", "bch", "native", null);

            satoshis = BRCryptoUnit
                    .createAsBase(btc, "sat", "satoshi", "SAT");

            bitcoins = BRCryptoUnit
                    .create(btc, "btc", "bitcoin", "B", satoshis, UnsignedInteger.valueOf(8));

            factor = BRCryptoAmount
                    .create(1000, satoshis);

            fee = BRCryptoNetworkFee
                    .create(UnsignedLong.valueOf(30 * 1000), factor, satoshis);

            network.setHeight(UnsignedLong.valueOf(blockHeight));

            network.setCurrency(btc);
            network.addCurrency(btc, satoshis, bitcoins);

            network.addCurrencyUnit(btc, satoshis);
            network.addCurrencyUnit(btc, bitcoins);

            network.addFee(fee);

            return network.take();

        } finally {
            if (null != btc) btc.give();
            if (null != satoshis) satoshis.give();
            if (null != bitcoins) bitcoins.give();
            if (null != factor) factor.give();
            if (null != fee) fee.give();
            if (null != network) network.give();
        }
    }

    private static BRCryptoNetwork createEthereumNetwork(boolean isMainnet, long blockHeight) {
        BRCryptoCurrency eth = null;
        BRCryptoUnit wei = null;
        BRCryptoUnit gwei = null;
        BRCryptoUnit ether = null;
        BRCryptoAmount factor = null;
        BRCryptoNetworkFee fee = null;
        BRCryptoNetwork network = null;

        try {

            network = BRCryptoNetwork.findBuiltin("ethereum-" + (isMainnet ? "mainnet" : "ropsten")).get();

            eth = BRCryptoCurrency
                    .create("ethereum", "ethereum", "eth", "native", null);

            wei = BRCryptoUnit
                    .createAsBase(eth, "wei", "wei", "wei");

            gwei = BRCryptoUnit
                    .create(eth, "gwei", "gwei", "gwei", wei, UnsignedInteger.valueOf(9));

            ether = BRCryptoUnit
                    .create(eth, "ether", "eth", "E", wei, UnsignedInteger.valueOf(18));

            factor = BRCryptoAmount
                    .create(2.0, gwei);

            fee = BRCryptoNetworkFee
                    .create(UnsignedLong.valueOf(1000), factor, gwei);

            network.setHeight(UnsignedLong.valueOf(blockHeight));

            network.setCurrency(eth);

            network.addCurrency(eth, wei, ether);

            network.addCurrencyUnit(eth, wei);
            network.addCurrencyUnit(eth, gwei);
            network.addCurrencyUnit(eth, ether);

            network.addFee(fee);

            return network.take();

        } finally {
            if (null != eth) eth.give();
            if (null != wei) wei.give();
            if (null != gwei) gwei.give();
            if (null != ether) ether.give();
            if (null != factor) factor.give();
            if (null != fee) fee.give();
            if (null != network) network.give();
        }
    }

    public interface TestCryptoLibrary extends Library {
        TestCryptoLibrary INSTANCE = Native.load(CryptoLibrary.LIBRARY_NAME, TestCryptoLibrary.class);

        int BRRunTests();
        int BRRunSupTests();
        int BRRunTestsSync (String paperKey, int isBTC, int isMainnet);
        int BRRunTestWalletManagerSync (String paperKey, String storagePath, int isBTC, int isMainnet);
        int BRRunTestWalletManagerSyncStress(String paperKey, String storagePath, int epoch, long blockHeight, int isBTC, int isMainnet);

        void runCryptoTests();
        int runCryptoTestsWithAccountAndNetwork(BRCryptoAccount account, BRCryptoNetwork network, String storagePath);

        void runUtilTests();
        void runRlpTests();
        void runEventTests();
        void runBaseTests();
        void runBcTests();
        void runContractTests();
        void runTests(int reallySend);
        void runEWMTests(String paperKey, String storagePath);
        void runLESTests(String paperKey);
        void runNodeTests();

        void runRippleTest();
    }
}
