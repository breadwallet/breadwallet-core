package com.breadwallet.corenative;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import com.breadwallet.corenative.crypto.BRCryptoAccount;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoCoder;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoUnit;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerEvent;
import com.google.common.base.Stopwatch;
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

public class CryptoLibraryAIT {

    private String paperKey;
    private File coreDataDir;
    private int epoch;

    @Before
    public void setup() {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();

        // this is a compromised testnet paperkey
        paperKey = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        coreDataDir = new File (context.getFilesDir(), "corenative");
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
    public void testPerformance() {
        // testFunctionPerformance
        {
            final long count = 100 * 1000;
            final byte[] d = new byte[]{(byte) 0xde, (byte) 0xad, (byte) 0xbe, (byte) 0xef};
            final String r = "deadbeef";

            BRCryptoCoder c;
            Stopwatch t;

            double durationInMs;
            double durationInMsPerAttempt;
            long attemptsPerSecond;

            // this tests creating a coder and encoding/decoding using the current app behaviour (i.e. no direct mapping involved)

            t = Stopwatch.createStarted();
            for (int i = 0; i < count; i++) {
                c = CryptoLibrary.INSTANCE.cryptoCoderCreate(0);
                c.encode(d).get();
                c.decode(r).get();
            }
            t.stop();

            durationInMs = t.elapsed(TimeUnit.MILLISECONDS);
            durationInMsPerAttempt = durationInMs / count;
            attemptsPerSecond = (long) (1000 / durationInMsPerAttempt);
            Log.e("JNA", String.format("testFunctionPerformance-legacy: %s attempt duration in milliseconds", durationInMsPerAttempt));
            Log.e("JNA", String.format("testFunctionPerformance-legacy: %s attempts per second", attemptsPerSecond));

            // this tests creating a coder and encoding/decoding using direct mapping

            t = Stopwatch.createStarted();
            for (int i = 0; i < count; i++) {
                c = new BRCryptoCoder(CryptoLibraryDirect.cryptoCoderCreate(0));
                c.encodeDirect(d).get();
                c.decodeDirect(r).get();
            }
            t.stop();

            durationInMs = t.elapsed(TimeUnit.MILLISECONDS);
            durationInMsPerAttempt = durationInMs / count;
            attemptsPerSecond = (long) (1000 / durationInMsPerAttempt);
            Log.e("JNA", String.format("testFunctionPerformance-direct: %s attempt duration in milliseconds", durationInMsPerAttempt));
            Log.e("JNA", String.format("testFunctionPerformance-direct: %s attempts per second", attemptsPerSecond));
        }

        // testCallbackPerformance
        {
            final long count = 10 * 1000;
            double durationInMs;
            double durationInMsPerInvocation;
            long invocationsPerSecond;

            BRCryptoCWMListener.BRCryptoCWMListenerWalletManagerEvent callback = (context, manager, event) -> { /* nadda */ };
            BRCryptoCWMListener.BRCryptoCWMListenerWalletManagerEventDirect callbackDirect = (context, manager, event) -> new BRCryptoWalletManagerEvent(event);

            // This tests the current app behaviour (i.e. no direct mapping involved)

            CryptoLibrary.INSTANCE.cryptoWalletManagerCallMeMaybe(callback, 1); // call once to remove caching as a factor
            durationInMs = CryptoLibrary.INSTANCE.cryptoWalletManagerCallMeMaybe(callback, count);
            durationInMsPerInvocation = durationInMs / count;
            invocationsPerSecond = (long) (1000 / durationInMsPerInvocation);
            Log.e("JNA", String.format("testCallbackPerformance-legacy: %s event callback duration in milliseconds", durationInMsPerInvocation));
            Log.e("JNA", String.format("testCallbackPerformance-legacy: %s event callbacks per second", invocationsPerSecond));

            // This tests the callback being passed as a pointer (rather than by value as a callback argument)

            CryptoLibraryDirect.cryptoWalletManagerCallMeMaybe(callback, 1); // call once to remove caching as a factor
            durationInMs = CryptoLibraryDirect.cryptoWalletManagerCallMeMaybe(callback, count);
            durationInMsPerInvocation = durationInMs / count;
            invocationsPerSecond = (long) (1000 / durationInMsPerInvocation);

            Log.e("JNA", String.format("testCallbackPerformance-improved: %s event callback duration in milliseconds", durationInMsPerInvocation));
            Log.e("JNA", String.format("testCallbackPerformance-improved: %s event callbacks per second", invocationsPerSecond));

            // This tests the initial call being done with direct mapping and the callback being passed as a pointer (rather than by value as a callback argument)

            CryptoLibraryDirect.cryptoWalletManagerCallMeMaybeDirect(callbackDirect, 1); // call once to remove caching as a factor
            durationInMs = CryptoLibraryDirect.cryptoWalletManagerCallMeMaybeDirect(callbackDirect, count);
            durationInMsPerInvocation = durationInMs / count;
            invocationsPerSecond = (long) (1000 / durationInMsPerInvocation);

            Log.e("JNA", String.format("testCallbackPerformance-improved-again: %s event callback duration in milliseconds", durationInMsPerInvocation));
            Log.e("JNA", String.format("testCallbackPerformance-improved-again: %s event callbacks per second", invocationsPerSecond));
        }
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
                UnsignedLong.valueOf(epoch)
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
                UnsignedLong.valueOf(epoch)
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
                UnsignedLong.valueOf(epoch)
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

            network = BRCryptoNetwork
                    .createAsBtc("bitcoin-" + (isMainnet ? "mainnet" : "testnet"), "bitcoin", isMainnet);

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

            network = BRCryptoNetwork
                    .createAsBch("bitcoin-cash-" + (isMainnet ? "mainnet" : "testnet"), "bitcoin cash", isMainnet);

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

            eth = BRCryptoCurrency
                    .create("ethereum", "ethereum", "eth", "native", null);

            wei = BRCryptoUnit
                    .createAsBase(eth, "wei", "wei", "wei");

            gwei = BRCryptoUnit
                    .create(eth, "gwei", "gwei", "gwei", wei, UnsignedInteger.valueOf(9));

            ether = BRCryptoUnit
                    .create(eth, "ether", "eth", "E", wei, UnsignedInteger.valueOf(18));

            network = BRCryptoNetwork
                    .createAsEth("ethereum-" + (isMainnet ? "mainnet" : "testnet"), "ethereum", isMainnet)
                    .get();

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
