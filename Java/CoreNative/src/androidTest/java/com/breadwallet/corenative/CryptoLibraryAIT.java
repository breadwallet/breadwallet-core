package com.breadwallet.corenative;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import com.breadwallet.corenative.crypto.BRCryptoAccount;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoUnit;
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
import java.util.Date;
import java.util.TimeZone;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class CryptoLibraryAIT {

    private String paperKey;
    private File coreDataDir;
    private int epoch;
    private Date epochDate;

    @Before
    public void setup() {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();

        // this is a compromised testnet paperkey
        paperKey = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        coreDataDir = new File (context.getFilesDir(), "corenative");
        Calendar calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC"));
        calendar.set(2017, /* September */ 8, 6);
        epoch = (int) TimeUnit.MILLISECONDS.toSeconds(calendar.getTimeInMillis());
        epochDate = calendar.getTime();

        coreDirClear();
        coreDirCreate();
    }

    @After
    public void teardown() {
        coreDirClear();
    }

    @Test
    public void testLoad() {
        assertNotNull(CryptoLibrary.INSTANCE);
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
    public void testBitcoinWalletManagerSyncModes () {
        int success = 0;
        int isBTC = 0;
        int isMainnet = 0;
        long blockHeight = 0;

        // BTC mainnet

        isBTC = 1;
        isMainnet = 1;
        blockHeight = 500000;

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(paperKey, coreDataDir.getAbsolutePath(), epoch, blockHeight, isBTC, isMainnet);
        assertEquals(1, success);

        // BTC testnet

        isBTC = 1;
        isMainnet = 0;
        blockHeight = 1500000;

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(paperKey, coreDataDir.getAbsolutePath(), epoch, blockHeight, isBTC, isMainnet);
        assertEquals(1, success);

        // BCH mainnet

        isBTC = 0;
        isMainnet = 1;
        blockHeight = 500000;

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(paperKey, coreDataDir.getAbsolutePath(), epoch, blockHeight, isBTC, isMainnet);
        assertEquals(1, success);

        // BCH testnet

        isBTC = 0;
        isMainnet = 0;
        blockHeight = 1500000;

        coreDirClear();
        success = TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSyncStress(paperKey, coreDataDir.getAbsolutePath(), epoch, blockHeight, isBTC, isMainnet);
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
    public void testCryptoWithAccountAndNetwork() {
        int success = 0;

        BRCryptoAccount account = BRCryptoAccount
                .createFromPhrase(paperKey.getBytes(StandardCharsets.UTF_8), UnsignedLong.valueOf(epoch));

        //
        // BTC
        //

        // MAINNET

        {
            BRCryptoNetwork network = createBitcoinNetwork(true, 500000);

            coreDirClear();
            success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                    account,
                    network,
                    coreDataDir.getAbsolutePath());
            assertEquals(1, success);
        }

        // TESTNET

        {
            BRCryptoNetwork network = createBitcoinNetwork(false, 1500000);

            coreDirClear();
            success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                    account,
                    network,
                    coreDataDir.getAbsolutePath());
            assertEquals(1, success);
        }

        //
        // BCH
        //

        // MAINNET

        {
            BRCryptoNetwork network = createBitcoinCashNetwork(true, 500000);

            coreDirClear();
            success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                    account,
                    network,
                    coreDataDir.getAbsolutePath());
            assertEquals(1, success);
        }

        // TESTNET

        {
            BRCryptoNetwork network = createBitcoinCashNetwork(false, 1500000);

            coreDirClear();
            success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                    account,
                    network,
                    coreDataDir.getAbsolutePath());
            assertEquals(1, success);
        }

        //
        // ETH
        //

        // MAINNET

        {
            BRCryptoNetwork network = createEthereumNetwork(true, 8000000);

            coreDirClear();
            success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                    account,
                    network,
                    coreDataDir.getAbsolutePath());
            assertEquals(1, success);
        }

        // TESTNET

        {
            BRCryptoNetwork network = createEthereumNetwork(false, 4500000);

            coreDirClear();
            success = TestCryptoLibrary.INSTANCE.runCryptoTestsWithAccountAndNetwork(
                    account,
                    network,
                    coreDataDir.getAbsolutePath());
            assertEquals(1, success);
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
        BRCryptoCurrency btc = BRCryptoCurrency
                .create("bitcoin", "bitcoin", "btc", "native", null);

        BRCryptoUnit satoshis = BRCryptoUnit
                .createAsBase(btc, "sat", "satoshi", "SAT");

        BRCryptoUnit bitcoins = BRCryptoUnit
                .create(btc, "btc", "bitcoin", "B", satoshis, UnsignedInteger.valueOf(8));

        BRCryptoNetwork network = BRCryptoNetwork
                .createAsBtc("bitcoin-" + (isMainnet ? "mainnet" : "testnet"), "bitcoin", isMainnet);

        BRCryptoAmount feePricePerCostFactor = BRCryptoAmount
                .create(1000, satoshis);

        BRCryptoNetworkFee fee = BRCryptoNetworkFee
                .create(UnsignedLong.valueOf(30 * 1000), feePricePerCostFactor, satoshis);

        network.setHeight(UnsignedLong.valueOf(blockHeight));

        network.setCurrency(btc);

        network.addCurrency(btc, satoshis, bitcoins);

        network.addCurrencyUnit(btc, satoshis);
        network.addCurrencyUnit(btc, bitcoins);

        network.addFee(fee);

        return network;
    }

    private static BRCryptoNetwork createBitcoinCashNetwork(boolean isMainnet, long blockHeight) {
        BRCryptoCurrency btc = BRCryptoCurrency
                .create("bitcoin-cash", "bitcoin cash", "bch", "native", null);

        BRCryptoUnit satoshis = BRCryptoUnit
                .createAsBase(btc, "sat", "satoshi", "SAT");

        BRCryptoUnit bitcoins = BRCryptoUnit
                .create(btc, "btc", "bitcoin", "B", satoshis, UnsignedInteger.valueOf(8));

        BRCryptoNetwork network = BRCryptoNetwork
                .createAsBch("bitcoin-cash-" + (isMainnet ? "mainnet" : "testnet"), "bitcoin cash", isMainnet);

        BRCryptoAmount feePricePerCostFactor = BRCryptoAmount
                .create(1000, satoshis);

        BRCryptoNetworkFee fee = BRCryptoNetworkFee
                .create(UnsignedLong.valueOf(30 * 1000), feePricePerCostFactor, satoshis);

        network.setHeight(UnsignedLong.valueOf(blockHeight));

        network.setCurrency(btc);

        network.addCurrency(btc, satoshis, bitcoins);

        network.addCurrencyUnit(btc, satoshis);
        network.addCurrencyUnit(btc, bitcoins);

        network.addFee(fee);

        return network;
    }

    private static BRCryptoNetwork createEthereumNetwork(boolean isMainnet, long blockHeight) {
        BRCryptoCurrency eth = BRCryptoCurrency
                .create("ethereum", "ethereum", "eth", "native", null);

        BRCryptoUnit wei = BRCryptoUnit
                .createAsBase(eth, "wei", "wei", "wei");

        BRCryptoUnit gwei = BRCryptoUnit
                .create(eth, "gwei", "gwei", "gwei", wei, UnsignedInteger.valueOf(9));

        BRCryptoUnit ether = BRCryptoUnit
                .create(eth, "ether", "eth", "E", wei, UnsignedInteger.valueOf(18));

        BRCryptoNetwork network = BRCryptoNetwork
                .createAsEth("ethereum-" + (isMainnet ? "mainnet" : "testnet"), "ethereum", isMainnet)
                .get();

        BRCryptoAmount feePricePerCostFactor = BRCryptoAmount
                .create(2.0, gwei);

        BRCryptoNetworkFee fee = BRCryptoNetworkFee
                .create(UnsignedLong.valueOf(1000), feePricePerCostFactor, gwei);

        network.setHeight(UnsignedLong.valueOf(blockHeight));

        network.setCurrency(eth);

        network.addCurrency(eth, wei, ether);

        network.addCurrencyUnit(eth, wei);
        network.addCurrencyUnit(eth, gwei);
        network.addCurrencyUnit(eth, ether);

        network.addFee(fee);

        return network;
    }

    public interface TestCryptoLibrary extends Library {
        TestCryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, TestCryptoLibrary.class);

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
