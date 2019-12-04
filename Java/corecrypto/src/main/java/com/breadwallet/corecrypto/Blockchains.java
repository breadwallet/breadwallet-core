/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/18/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.BlockchainFee;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.CurrencyDenomination;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;
import java.util.Locale;

/* package */
final class Blockchains {

    ///
    /// Defined Currency Denominations
    ///
    /// We define default blockchains but these are wholly insufficient given that the
    /// specification includes `blockHeight` (which can never be correct).
    ///

    private static CurrencyDenomination SATOSHI = CurrencyDenomination.create(
            "Satoshi",
            "sat",
            UnsignedInteger.valueOf(0),
            "sat"
    );


    private static CurrencyDenomination BTC_BITCOIN = CurrencyDenomination.create(
            "Bitcoin",
            "btc",
            UnsignedInteger.valueOf(8),
            "₿"
    );

    private static CurrencyDenomination BTC_BITCOIN_TESTNET = CurrencyDenomination.create(
            "Bitcoin Testnet",
            "btc",
            UnsignedInteger.valueOf(8),
            "₿"
    );


    private static CurrencyDenomination BCH_BITCOIN = CurrencyDenomination.create(
            "Bitcoin Cash",
            "bch",
            UnsignedInteger.valueOf(8),
            "bch"
    );

    private static CurrencyDenomination BCH_BITCOIN_TESTNET = CurrencyDenomination.create(
            "Bitcoin Cash Testnet",
            "bch",
            UnsignedInteger.valueOf(8),
            "bch"
    );


    private static CurrencyDenomination ETH_WEI = CurrencyDenomination.create(
            "Wei",
            "wei",
            UnsignedInteger.valueOf(0),
            "wei"
    );

    private static CurrencyDenomination ETH_GWEI = CurrencyDenomination.create(
            "Gwei",
            "gwei",
            UnsignedInteger.valueOf(9),
            "gwei"
    );

    private static CurrencyDenomination ETH_ETHER = CurrencyDenomination.create(
            "Ether",
            "eth",
            UnsignedInteger.valueOf(18),
            "Ξ"
    );

    ///
    /// Defined Currencies
    ///
    /// We define default blockchains but these are wholly insufficient given that the
    /// specification includes `blockHeight` (which can never be correct).
    ///

    // Mainnet

    /* package */
    static final String ADDRESS_BRD_MAINNET = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6";

    /* package */
    static final String ADDRESS_BRD_TESTNET = "0x7108ca7c4718efa810457f228305c9c71390931a";

    private static Currency CURRENCY_BTC_MAINNET = Currency.create(
            "bitcoin-mainnet:__native__",
            "Bitcoin",
            "btc",
            "native",
            "bitcoin-mainnet",
            null,
            true,
            ImmutableList.of(
                    SATOSHI,
                    BTC_BITCOIN
            )
    );

    private static Currency CURRENCY_BCH_MAINNET = Currency.create(
            "bitcoincash-mainnet:__native__",
            "Bitcoin Cash",
            "bch",
            "native",
            "bitcoincash-mainnet",
            null,
            true,
            ImmutableList.of(
                    SATOSHI,
                    BCH_BITCOIN
            )
    );

    private static Currency CURRENCY_ETH_MAINNET = Currency.create(
            "ethereum-mainnet:__native__",
            "Ethereum",
            "eth",
            "native",
            "ethereum-mainnet",
            null,
            true,
            ImmutableList.of(
                    ETH_WEI,
                    ETH_GWEI,
                    ETH_ETHER
            )
    );

    private static Currency CURRENCY_BRD_MAINNET = Currency.create(
            makeCurrencyIdentifierErc20("ethereum-mainnet:", ADDRESS_BRD_MAINNET),
            "BRD Token",
            "brd",
            "erc20",
            "ethereum-mainnet",
            ADDRESS_BRD_MAINNET,
            true,
            makeCurrencyDemominationsErc20("brd", UnsignedInteger.valueOf(18))
    );

    // Testnet

    private static Currency CURRENCY_BTC_TESTNET = Currency.create(
            "bitcoin-testnet:__native__",
            "Bitcoin Test",
            "btc",
            "native",
            "bitcoin-testnet",
            null,
            true,
            ImmutableList.of(
                    SATOSHI,
                    BTC_BITCOIN_TESTNET
            )
    );

    private static Currency CURRENCY_BCH_TESTNET = Currency.create(
            "bitcoincash-testnet:__native__",
            "Bitcoin Cash Testnet",
            "bch",
            "native",
            "bitcoincash-testnet",
            null,
            true,
            ImmutableList.of(
                    SATOSHI,
                    BCH_BITCOIN_TESTNET
            )
    );

    private static Currency CURRENCY_ETH_ROPSTEN = Currency.create(
            "ethereum-ropsten:__native__",
            "Ethereum Testnet",
            "eth",
            "native",
            "ethereum-ropsten",
            null,
            true,
            ImmutableList.of(
                    ETH_WEI,
                    ETH_GWEI,
                    ETH_ETHER
            )
    );

    private static Currency CURRENCY_BRD_ROPSTEN = Currency.create(
            makeCurrencyIdentifierErc20("ethereum-ropsten:", ADDRESS_BRD_TESTNET),
            "BRD Token Testnet",
            "brd",
            "erc20",
            "ethereum-ropsten",
            ADDRESS_BRD_TESTNET,
            true,
            makeCurrencyDemominationsErc20("brd", UnsignedInteger.valueOf(18))
    );

    private static String makeCurrencyIdentifierErc20(String blockchainId, String address) {
        return String.format(Locale.ROOT, "%s:%s", blockchainId, address);
    }

    /* package */
    static List<CurrencyDenomination> makeCurrencyDemominationsErc20 (String code, UnsignedInteger decimals) {
        String name = code.toUpperCase(Locale.ROOT);
        code = code.toLowerCase(Locale.ROOT);

        return ImmutableList.of(
                CurrencyDenomination.create(
                        String.format(Locale.ROOT, "%s Token INT", name),
                        String.format(Locale.ROOT, "%si", code),
                        UnsignedInteger.ZERO,
                        String.format(Locale.ROOT, "%si", code)
                ),
                CurrencyDenomination.create(
                        String.format(Locale.ROOT, "%s Token", name),
                        code,
                        decimals,
                        code
                )
        );
    }

    ///
    /// Defined Blockchains
    ///
    /// We define default blockchains but these are wholly insufficient given that the
    /// specification includes `blockHeight` (which can never be correct).
    ///

    // Mainnet

    private static Blockchain BLOCKCHAIN_BTC_MAINNET = Blockchain.create(
            "bitcoin-mainnet",
            "Bitcoin",
            "mainnet",
            true,
            "bitcoin-mainnet:__native__",
            UnsignedLong.valueOf(595000),
            ImmutableList.of(
                    BlockchainFee.create(
                            "30",
                            "10m",
                            UnsignedLong.valueOf(10 * 60 * 1000)
                    )
            ),
            UnsignedInteger.valueOf(6)
    );

    private static Blockchain BLOCKCHAIN_BCH_MAINNET = Blockchain.create(
            "bitcoincash-mainnet",
            "Bitcoin Cash",
            "mainnet",
            true,
            "bitcoincash-mainnet:__native__",
            UnsignedLong.valueOf(600000),
            ImmutableList.of(
                    BlockchainFee.create(
                            "30",
                            "10m",
                            UnsignedLong.valueOf(10 * 60 * 1000)
                    )
            ),
            UnsignedInteger.valueOf(6)
    );

    private static Blockchain BLOCKCHAIN_ETH_MAINNET = Blockchain.create(
            "ethereum-mainnet",
            "Ethereum",
            "mainnet",
            true,
            "ethereum-mainnet:__native__",
            UnsignedLong.valueOf(8570000),
            ImmutableList.of(
                    BlockchainFee.create(
                            "2000000000",
                            "1m",
                            UnsignedLong.valueOf(60 * 1000)
                    )
            ),
            UnsignedInteger.valueOf(6)
    );

    // Testnet

    private static Blockchain BLOCKCHAIN_BTC_TESTNET = Blockchain.create(
            "bitcoin-testnet",
            "Bitcoin Testnet",
            "testnet",
            false,
            "bitcoin-testnet:__native__",
            UnsignedLong.valueOf(1575000),
            ImmutableList.of(
                    BlockchainFee.create(
                            "30",
                            "10m",
                            UnsignedLong.valueOf(10 * 60 * 1000)
                    )
            ),
            UnsignedInteger.valueOf(6)
    );

    private static Blockchain BLOCKCHAIN_BCH_TESTNET = Blockchain.create(
            "bitcoincash-testnet",
            "Bitcoin Cash Testnet",
            "testnet",
            false,
            "bitcoincash-testnet:__native__",
            UnsignedLong.valueOf(1325000),
            ImmutableList.of(
                    BlockchainFee.create(
                            "30",
                            "10m",
                            UnsignedLong.valueOf(10 * 60 * 1000)
                    )
            ),
            UnsignedInteger.valueOf(6)
    );

    private static Blockchain BLOCKCHAIN_ETH_ROPSTEN = Blockchain.create(
            "ethereum-ropsten",
            "Ethereum Ropsten",
            "testnet",
            false,
            "ethereum-ropsten:__native__",
            UnsignedLong.valueOf(6415000),
            ImmutableList.of(
                    BlockchainFee.create(
                            "2000000000",
                            "1m",
                            UnsignedLong.valueOf(60 * 1000)
                    )
            ),
            UnsignedInteger.valueOf(6)
    );

    ///
    /// Supported Blockchains
    ///

    /* package */
    static List<Blockchain> SUPPORTED_BLOCKCHAINS = ImmutableList.of(
            Blockchains.BLOCKCHAIN_BTC_MAINNET,
            Blockchains.BLOCKCHAIN_BCH_MAINNET,
            Blockchains.BLOCKCHAIN_ETH_MAINNET,

            Blockchains.BLOCKCHAIN_BTC_TESTNET,
            Blockchains.BLOCKCHAIN_BCH_TESTNET,
            Blockchains.BLOCKCHAIN_ETH_ROPSTEN
    );

    ///
    /// Default Currencies
    ///

    /* package */
    static final List<com.breadwallet.crypto.blockchaindb.models.bdb.Currency> DEFAULT_CURRENCIES = ImmutableList.of(
            Blockchains.CURRENCY_BTC_MAINNET,
            Blockchains.CURRENCY_BCH_MAINNET,
            Blockchains.CURRENCY_ETH_MAINNET,
            Blockchains.CURRENCY_BRD_MAINNET,

            Blockchains.CURRENCY_BTC_TESTNET,
            Blockchains.CURRENCY_BCH_TESTNET,
            Blockchains.CURRENCY_ETH_ROPSTEN,
            Blockchains.CURRENCY_BRD_ROPSTEN
    );

    ///
    /// Address Schemes
    ///

    /* package */
    static final ImmutableMultimap<String, AddressScheme> SUPPORTED_ADDRESS_SCHEMES;

    static {
        ImmutableMultimap.Builder<String, AddressScheme> builder = new ImmutableMultimap.Builder<>();
        builder.put("bitcoin-mainnet", AddressScheme.BTC_SEGWIT);
        builder.put("bitcoin-mainnet", AddressScheme.BTC_LEGACY);
        builder.put("bitcoincash-mainnet", AddressScheme.BTC_LEGACY);
        builder.put("ethereum-mainnet", AddressScheme.ETH_DEFAULT);
        builder.put("ripple-mainnet", AddressScheme.GEN_DEFAULT);

        builder.put("bitcoin-testnet", AddressScheme.BTC_SEGWIT);
        builder.put("bitcoin-testnet", AddressScheme.BTC_LEGACY);
        builder.put("bitcoincash-testnet", AddressScheme.BTC_LEGACY);
        builder.put("ethereum-ropsten", AddressScheme.ETH_DEFAULT);
        builder.put("ripple-testnet", AddressScheme.GEN_DEFAULT);
        SUPPORTED_ADDRESS_SCHEMES = builder.build();
    }

    /* package */
    static final ImmutableMap<String, AddressScheme> DEFAULT_ADDRESS_SCHEMES;

    static {
        ImmutableMap.Builder<String, AddressScheme> builder = new ImmutableMap.Builder<>();
        builder.put("bitcoin-mainnet", AddressScheme.BTC_SEGWIT);
        builder.put("bitcoincash-mainnet", AddressScheme.BTC_LEGACY);
        builder.put("ethereum-mainnet", AddressScheme.ETH_DEFAULT);
        builder.put("ripple-mainnet", AddressScheme.GEN_DEFAULT);

        builder.put("bitcoin-testnet", AddressScheme.BTC_SEGWIT);
        builder.put("bitcoincash-testnet", AddressScheme.BTC_LEGACY);
        builder.put("ethereum-ropsten", AddressScheme.ETH_DEFAULT);
        builder.put("ripple-testnet", AddressScheme.GEN_DEFAULT);
        DEFAULT_ADDRESS_SCHEMES = builder.build();
    }

    ///
    /// Wallet Manager Modes
    ///
    /// Blockchains with built-in P2P support (BTC, BCH, and ETH) may support `.p2p_only`.
    /// Intermediate modes (.api_with_p2p_submit, .p2p_with_api_sync) are suppored on a case-by-case
    /// basis. API mode is supported if BRD infrastructure supports that blockchain (for example,
    /// BCH is not at the moment)
    ///
    /// It is possible that the `.api_only` mode does not work - for exmaple, the BDB is down.  In
    /// that case it is an App issue to report and resolve the issue by: waiting out the outage;
    /// selecting another mode if available.
    ///
    /// These values are updated whenever the BDB support updates.  However, a given WalletKit
    /// distribution in the wild might be out of date with the current BDB support.  That can mean
    /// that some API mode is missing here that a new BDB support (like when BCH comes online) or
    /// that a mode has disappeared (maybe a blockchain is dropped).  These cases are not
    /// destructive.
    ///

    /* package */
    static final ImmutableMultimap<String, WalletManagerMode> SUPPORTED_MODES;

    static {
        ImmutableMultimap.Builder<String, WalletManagerMode> builder = new ImmutableMultimap.Builder<>();
        builder.put("bitcoin-mainnet", WalletManagerMode.P2P_ONLY);
        builder.put("bitcoin-mainnet", WalletManagerMode.API_ONLY);
        builder.put("bitcoincash-mainnet", WalletManagerMode.P2P_ONLY);
        builder.put("bitcoincash-mainnet", WalletManagerMode.API_ONLY);
        builder.put("ethereum-mainnet", WalletManagerMode.API_ONLY);
        builder.put("ethereum-mainnet", WalletManagerMode.API_WITH_P2P_SUBMIT);
        builder.put("ethereum-mainnet", WalletManagerMode.P2P_ONLY);

        builder.put("bitcoin-testnet", WalletManagerMode.P2P_ONLY);
        builder.put("bitcoin-testnet", WalletManagerMode.API_ONLY);
        builder.put("bitcoincash-testnet", WalletManagerMode.P2P_ONLY);
        builder.put("bitcoincash-testnet", WalletManagerMode.API_ONLY);
        builder.put("ethereum-ropsten", WalletManagerMode.API_ONLY);
        builder.put("ethereum-ropsten", WalletManagerMode.API_WITH_P2P_SUBMIT);
        builder.put("ethereum-ropsten", WalletManagerMode.P2P_ONLY);
        SUPPORTED_MODES = builder.build();
    }

    /* package */
    static final ImmutableMap<String, WalletManagerMode> DEFAULT_MODES;

    static {
        ImmutableMap.Builder<String, WalletManagerMode> builder = new ImmutableMap.Builder<>();
        builder.put("bitcoin-mainnet", WalletManagerMode.P2P_ONLY);
        builder.put("bitcoincash-mainnet", WalletManagerMode.P2P_ONLY);
        builder.put("ethereum-mainnet", WalletManagerMode.API_ONLY);

        builder.put("bitcoin-testnet", WalletManagerMode.P2P_ONLY);
        builder.put("bitcoincash-testnet", WalletManagerMode.P2P_ONLY);
        builder.put("ethereum-ropsten", WalletManagerMode.API_ONLY);
        DEFAULT_MODES = builder.build();
    }
}
