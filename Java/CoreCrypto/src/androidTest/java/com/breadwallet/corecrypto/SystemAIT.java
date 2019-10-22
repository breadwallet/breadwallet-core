/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;

import com.breadwallet.corecrypto.HelpersAIT.RecordingSystemListener;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.errors.MigrateBlockError;
import com.breadwallet.crypto.errors.MigrateError;
import com.breadwallet.crypto.errors.MigrateTransactionError;
import com.breadwallet.crypto.migration.BlockBlob;
import com.breadwallet.crypto.migration.PeerBlob;
import com.breadwallet.crypto.migration.TransactionBlob;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.google.common.util.concurrent.Uninterruptibles;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class SystemAIT {

    private File coreDataDir;

    @Before
    public void setup() {
        HelpersAIT.registerCryptoApiProvider();

        coreDataDir = HelpersAIT.generateCoreDataDir();
        HelpersAIT.createOrOverwriteDirectory(coreDataDir);
    }

    @After
    public void teardown() {
        HelpersAIT.deleteFile(coreDataDir);
    }

    @Test
    public void testSystemAppCurrencies() {
        // Create a query that fails (no authentication)
        BlockchainDb query = HelpersAIT.createDefaultBlockchainDbWithoutToken();

        // We need the UIDS to contain a valid ETH address BUT not be a default.  Since we are
        // using `isMainnet = false` use a mainnet address.
        List<com.breadwallet.crypto.blockchaindb.models.bdb.Currency> currencies = Collections.singletonList(
                System.asBlockChainDBModelCurrency(
                        "ethereum-ropsten" + ":" + Blockchains.ADDRESS_BRD_MAINNET,
                        "FOO Token",
                        "FOO",
                        "ERC20",
                        UnsignedInteger.valueOf(10)
                ).get()
        );

        System system = HelpersAIT.createAndConfigureSystemWithBlockchainDbAndCurrencies(coreDataDir, query, currencies);
        assertTrue(system.getNetworks().size() >= 1);

        Network network = null;
        for (Network n: system.getNetworks()) if (n.getCurrency().getCode().equals("eth") && !n.isMainnet()) network = n;
        assertNotNull(network);

        assertTrue(network.getCurrencyByCode("eth").isPresent());
        Optional<? extends Currency> fooCurrency = network.getCurrencyByCode("FOO");
        assertTrue(fooCurrency.isPresent());

        assertEquals("erc20", fooCurrency.get().getType());

        Optional<? extends Unit> fooDefault = network.defaultUnitFor(fooCurrency.get());
        assertTrue(fooDefault.isPresent());
        assertEquals(UnsignedInteger.valueOf(10), fooDefault.get().getDecimals());
        assertEquals("FOO", fooDefault.get().getSymbol());

        Optional<? extends Unit> fooBase = network.baseUnitFor(fooCurrency.get());
        assertTrue(fooBase.isPresent());
        assertEquals(UnsignedInteger.ZERO, fooBase.get().getDecimals());
        assertEquals("FOOI", fooBase.get().getSymbol());
    }

    @Test
    public void testSystemBtc() {
        testSystemForCurrency("btc", WalletManagerMode.API_ONLY, AddressScheme.BTC_LEGACY);
    }

    @Test
    public void testSystemBch() {
        testSystemForCurrency("bch", WalletManagerMode.P2P_ONLY, AddressScheme.BTC_LEGACY);
    }

    @Test
    public void testSystemEth() {
        testSystemForCurrency("eth", WalletManagerMode.API_ONLY, AddressScheme.ETH_DEFAULT);
    }

    @Test
    public void testSystemEthMigrationNotRequired() {
        testSystemMigrationNotRequiredForCurrency("eth");
    }

    @Test
    public void testSystemBtcMigrationSuccess() {
        testSystemMigrationSuccessForBitcoinCurrency("btc");
    }

    @Test
    public void testSystemBchMigrationSuccess() {
        testSystemMigrationSuccessForBitcoinCurrency("bch");
    }

    @Test
    public void testSystemBtcMigrationFailureOnTransaction() {
        testSystemMigrationFailureOnTransactionForBitcoinCurrency("btc");
    }

    @Test
    public void testSystemBchMigrationFailureOnTransaction() {
        testSystemMigrationFailureOnTransactionForBitcoinCurrency("bch");
    }

    @Test
    public void testSystemBtcMigrationFailureOnBlock() {
        testSystemMigrationFailureOnBlockForBitcoinCurrency("btc");
    }

    @Test
    public void testSystemBchMigrationFailureOnBlock() {
        testSystemMigrationFailureOnBlockForBitcoinCurrency("bch");
    }

    private void testSystemForCurrency(String currencyCode, WalletManagerMode mode, AddressScheme scheme) {
        RecordingSystemListener recorder = HelpersAIT.createRecordingListener();
        System system = HelpersAIT.createAndConfigureSystemWithListener(coreDataDir, recorder);

        // networks

        Collection<Network> networks = recorder.getAddedNetworks();
        assertNotEquals(0, networks.size());

        Optional<Network> maybeNetwork = HelpersAIT.getNetworkByCurrencyCode(networks, currencyCode);
        assertTrue(maybeNetwork.isPresent());

        Network network = maybeNetwork.get();
        system.createWalletManager(network, mode, scheme, Collections.emptySet());
        Uninterruptibles.sleepUninterruptibly(5, TimeUnit.SECONDS);

        // managers

        Collection<WalletManager> managers = recorder.getAddedManagers();
        assertEquals(1, managers.size());

        Optional<WalletManager> maybeManager = HelpersAIT.getManagerByCode(managers, currencyCode);
        assertTrue(maybeManager.isPresent());

        WalletManager manager = maybeManager.get();
        assertEquals(system, manager.getSystem());
        assertEquals(network, manager.getNetwork());

        // wallets

        Collection<Wallet> wallets = recorder.getAddedWallets();
        assertEquals(1, wallets.size());

        Optional<Wallet> maybeWallet = HelpersAIT.getWalletByCode(wallets, currencyCode);
        assertTrue(maybeWallet.isPresent());

        Wallet wallet = maybeWallet.get();
        assertEquals(manager, wallet.getWalletManager());
        assertEquals(manager.getCurrency(), wallet.getCurrency());
        assertEquals(network.getCurrency(), wallet.getCurrency());
        assertEquals(Amount.create(0, manager.getBaseUnit()), wallet.getBalance());
        assertEquals(WalletState.CREATED, wallet.getState());
        assertEquals(0, wallet.getTransfers().size());
    }

    private void testSystemMigrationNotRequiredForCurrency(String currencyCode) {
        RecordingSystemListener recorder = HelpersAIT.createRecordingListener();
        System system = HelpersAIT.createAndConfigureSystemWithListener(coreDataDir, recorder);

        Collection<Network> networks = recorder.getAddedNetworks();
        Network network = HelpersAIT.getNetworkByCurrencyCode(networks, currencyCode).get();

        assertFalse(system.migrateRequired(network));
    }

    private void testSystemMigrationSuccessForBitcoinCurrency(String currencyCode) {
        RecordingSystemListener recorder = HelpersAIT.createRecordingListener();
        System system = HelpersAIT.createAndConfigureSystemWithListener(coreDataDir, recorder);

        Collection<Network> networks = recorder.getAddedNetworks();
        Network network = HelpersAIT.getNetworkByCurrencyCode(networks, currencyCode).get();

        // transaction blob
        List<TransactionBlob> transactionBlobs = Collections.singletonList(
                TransactionBlob.BTC(
                        Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).decode(
                                "010000000001017b032f6a651c7dcbcfb78d817b303be8d20afa22901618b517f21755a7cd8d4801" +
                                        "00000023220020e0627b64745905646f276f355502a4053058b64edbf277119249611c98da4169ff" +
                                        "ffffff020cf962010000000017a914243157d578bd928a92e039e8d4dbbb294416935c87f3be2a00" +
                                        "000000001976a91448380bc7605e91a38f8d7ba01a2795416bf92dde88ac040047304402205f5de6" +
                                        "8896ca3edf97e3ea1fd3513903537fd5f2e0b3661d6c617b1c48fc69e102200e0f2059513be93183" +
                                        "929c7d3e2de0e9c7085706a88e8f746e8f5aa713d27a5201473044022050d8ecb9cd7fdacb6d6351" +
                                        "dec2bc5b3716328ef2c4466db44bdd34a657292b8c022068501bf81812ad8e3ed9df24354c371923" +
                                        "a07dc966a6e41463594774d009169e0169522103b8e138ed70232c9cbd1b9028121064236af12dbe" +
                                        "98641c3f74fa13166f272f582103f66ee7c87817d324921edc3f7d7726de5a18cfed057e5a50e7c7" +
                                        "4e2ae7e05ad72102a7bf21582d71e5da5c3bc43e84c88fdf32803aa4720e1c1a9d08aab541a4f331" +
                                        "53ae00000000").get(),
                        UnsignedInteger.ZERO,
                        UnsignedInteger.ZERO)
        );

        // block blob
        List<BlockBlob> blockBlobs = Collections.singletonList(
                BlockBlob.BTC(
                        Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).decode(
                                "0100000006e533fd1ada86391f3f6c343204b0d278d4aaec1c0b20aa27ba0300000000006abbb3eb" +
                                        "3d733a9fe18967fd7d4c117e4ccbbac5bec4d910d900b3ae0793e77f54241b4d4c86041b4089cc9b" +
                                        "0c000000084c30b63cfcdc2d35e3329421b9805ef0c6565d35381ca857762ea0b3a5a128bbca5065" +
                                        "ff9617cbcba45eb23726df6498a9b9cafed4f54cbab9d227b0035ddefbbb15ac1d57d0182aaee61c" +
                                        "74743a9c4f785895e563909bafec45c9a2b0ff3181d77706be8b1dcc91112eada86d424e2d0a8907" +
                                        "c3488b6e44fda5a74a25cbc7d6bb4fa04245f4ac8a1a571d5537eac24adca1454d65eda446055479" +
                                        "af6c6d4dd3c9ab658448c10b6921b7a4ce3021eb22ed6bb6a7fde1e5bcc4b1db6615c6abc5ca0421" +
                                        "27bfaf9f44ebce29cb29c6df9d05b47f35b2edff4f0064b578ab741fa78276222651209fe1a2c4c0" +
                                        "fa1c58510aec8b090dd1eb1f82f9d261b8273b525b02ff1a").get(),
                        UnsignedInteger.ZERO)
        );

        // peer blob
        List<PeerBlob> peerBlobs = Collections.singletonList(
                PeerBlob.BTC(
                        UnsignedInteger.ZERO,
                        UnsignedInteger.ZERO,
                        UnsignedLong.ZERO,
                        UnsignedInteger.ZERO
                )
        );

        assertTrue(system.migrateRequired(network));

        try {
            system.migrateStorage(network, transactionBlobs, blockBlobs, peerBlobs);
        } catch (MigrateError e) {
            fail();
        }
    }

    private void testSystemMigrationFailureOnTransactionForBitcoinCurrency(String currencyCode) {
        RecordingSystemListener recorder = HelpersAIT.createRecordingListener();
        System system = HelpersAIT.createAndConfigureSystemWithListener(coreDataDir, recorder);

        Collection<Network> networks = recorder.getAddedNetworks();
        Network network = HelpersAIT.getNetworkByCurrencyCode(networks, currencyCode).get();

        // transaction blob
        List<TransactionBlob> transactionBlobs = Collections.singletonList(
                TransactionBlob.BTC(
                        Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).decode(
                                "BAAD").get(),
                        UnsignedInteger.ZERO,
                        UnsignedInteger.ZERO)
        );

        assertTrue(system.migrateRequired(network));

        MigrateError error = null;
        try {
            system.migrateStorage(network, transactionBlobs, Collections.emptyList(), Collections.emptyList());
        } catch (MigrateTransactionError e) {
            error = e;
        } catch (MigrateError e) {
            fail();
        }
        assertNotNull(error);
    }

    private void testSystemMigrationFailureOnBlockForBitcoinCurrency(String currencyCode) {
        RecordingSystemListener recorder = HelpersAIT.createRecordingListener();
        System system = HelpersAIT.createAndConfigureSystemWithListener(coreDataDir, recorder);

        Collection<Network> networks = recorder.getAddedNetworks();
        Network network = HelpersAIT.getNetworkByCurrencyCode(networks, currencyCode).get();

        // block blob
        List<BlockBlob> blockBlobs = Collections.singletonList(
                BlockBlob.BTC(
                        Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).decode(
                                "BAAD").get(),
                        UnsignedInteger.ZERO)
        );

        assertTrue(system.migrateRequired(network));

        MigrateError error = null;
        try {
            system.migrateStorage(network, Collections.emptyList(), blockBlobs, Collections.emptyList());
        } catch (MigrateBlockError e) {
            error = e;
        } catch (MigrateError e) {
            fail();
        }
        assertNotNull(error);
    }
}
