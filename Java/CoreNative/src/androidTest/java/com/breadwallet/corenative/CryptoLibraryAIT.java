package com.breadwallet.corenative;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import com.sun.jna.Library;
import com.sun.jna.Native;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;

import static org.junit.Assert.*;

public class CryptoLibraryAIT {

    private String paperKey;
    private File coreDataDir;

    @Before
    public void setup() {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();

        // this is a compromised testnet paperkey
        paperKey = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        coreDataDir = new File (context.getFilesDir(), "corenative");

        coreDirCreate();
        coreDirClear();
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
        TestCryptoLibrary.INSTANCE.BRRunTestsSync (paperKey, 1, 1);
    }

    @Test
    public void testBitcoinWalletManagerSync () {
        coreDirClear();
        TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSync (paperKey, coreDataDir.getAbsolutePath(), 1, 1);
        TestCryptoLibrary.INSTANCE.BRRunTestWalletManagerSync (paperKey, coreDataDir.getAbsolutePath(), 1, 1);
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

    public interface TestCryptoLibrary extends Library {
        TestCryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, TestCryptoLibrary.class);

        int BRRunTests();
        int BRRunSupTests();
        int BRRunTestsSync (String paperKey, int isBTC, int isMainnet);
        int BRRunTestWalletManagerSync (String paperKey, String storagePath, int isBTC, int isMainnet);

        void runCryptoTests();

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
