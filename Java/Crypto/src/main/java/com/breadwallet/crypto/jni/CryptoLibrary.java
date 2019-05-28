package com.breadwallet.crypto.jni;

import com.breadwallet.crypto.jni.bitcoin.BRChainParams;
import com.breadwallet.crypto.jni.crypto.BRCryptoAccount;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.BRWalletEvent;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManager;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManagerClient;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManagerEvent;
import com.breadwallet.crypto.jni.crypto.BRCryptoAddress;
import com.breadwallet.crypto.jni.crypto.BRCryptoAmount;
import com.breadwallet.crypto.jni.crypto.BRCryptoCurrency;
import com.breadwallet.crypto.jni.crypto.BRCryptoUnit;
import com.breadwallet.crypto.jni.support.BRAddress;
import com.breadwallet.crypto.jni.bitcoin.BRTransaction;
import com.breadwallet.crypto.jni.bitcoin.BRTransactionEvent;
import com.breadwallet.crypto.jni.ethereum.BREthereumAddress;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;
import com.breadwallet.crypto.jni.support.UInt256;
import com.breadwallet.crypto.jni.support.UInt512;
import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;

public interface CryptoLibrary extends Library {
    String JNA_LIBRARY_NAME = "crypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);


    /** <i>native declaration : bitcoin/BRWallet.h</i> */
    long DEFAULT_FEE_PER_KB = (long)(1000L * 10);


    /**
     * <i>native declaration : support/BRSyncMode.h:5</i><br>
     * enum values
     */
    interface BRSyncMode {
        /** <i>native declaration : support/BRSyncMode.h:1</i> */
        int SYNC_MODE_BRD_ONLY = 0;
        /** <i>native declaration : support/BRSyncMode.h:2</i> */
        int SYNC_MODE_BRD_WITH_P2P_SEND = 1;
        /** <i>native declaration : support/BRSyncMode.h:3</i> */
        int SYNC_MODE_P2P_WITH_BRD_SYNC = 2;
        /** <i>native declaration : support/BRSyncMode.h:4</i> */
        int SYNC_MODE_P2P_ONLY = 3;
    };

    /**
     * <i>native declaration : bitcoin/BRWalletManager.h:27</i><br>
     * enum values
     */
    interface BRTransactionEventType {
        /** <i>native declaration : bitcoin/BRWalletManager.h:24</i> */
        int BITCOIN_TRANSACTION_ADDED = 0;
        /** <i>native declaration : bitcoin/BRWalletManager.h:25</i> */
        int BITCOIN_TRANSACTION_UPDATED = 1;
        /** <i>native declaration : bitcoin/BRWalletManager.h:26</i> */
        int BITCOIN_TRANSACTION_DELETED = 2;
    };
    /**
     * <i>native declaration : bitcoin/BRWalletManager.h:45</i><br>
     * enum values
     */
    interface BRWalletEventType {
        /** <i>native declaration : bitcoin/BRWalletManager.h:41</i> */
        int BITCOIN_WALLET_CREATED = 0;
        /** <i>native declaration : bitcoin/BRWalletManager.h:42</i> */
        int BITCOIN_WALLET_BALANCE_UPDATED = 1;
        /** <i>native declaration : bitcoin/BRWalletManager.h:43</i> */
        int BITCOIN_WALLET_TRANSACTION_SUBMITTED = 2;
        /** <i>native declaration : bitcoin/BRWalletManager.h:44</i> */
        int BITCOIN_WALLET_DELETED = 3;
    };
    /**
     * <i>native declaration : bitcoin/BRWalletManager.h:70</i><br>
     * enum values
     */
    interface BRWalletManagerEventType {
        /** <i>native declaration : bitcoin/BRWalletManager.h:64</i> */
        int BITCOIN_WALLET_MANAGER_CREATED = 0;
        /** <i>native declaration : bitcoin/BRWalletManager.h:65</i> */
        int BITCOIN_WALLET_MANAGER_CONNECTED = 1;
        /** <i>native declaration : bitcoin/BRWalletManager.h:66</i> */
        int BITCOIN_WALLET_MANAGER_DISCONNECTED = 2;
        /** <i>native declaration : bitcoin/BRWalletManager.h:67</i> */
        int BITCOIN_WALLET_MANAGER_SYNC_STARTED = 3;
        /** <i>native declaration : bitcoin/BRWalletManager.h:68</i> */
        int BITCOIN_WALLET_MANAGER_SYNC_STOPPED = 4;
        /** <i>native declaration : bitcoin/BRWalletManager.h:69</i> */
        int BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED = 5;
    };

    /**
     * <i>native declaration : crypto/BRCryptoBase.h:3</i><br>
     * enum values
     */
    interface BREthereumBoolean {
        /** <i>native declaration : crypto/BRCryptoBase.h:1</i> */
        int ETHEREUM_BOOLEAN_TRUE = 0;
        /** <i>native declaration : crypto/BRCryptoBase.h:2</i> */
        int ETHEREUM_BOOLEAN_FALSE = 1;
    };

    /**
     * <i>native declaration : crypto/BRCryptoBase.h:3</i><br>
     * enum values
     */
    interface BRCryptoBoolean {
        /** <i>native declaration : crypto/BRCryptoBase.h:1</i> */
        int CRYPTO_FALSE = 0;
        /** <i>native declaration : crypto/BRCryptoBase.h:2</i> */
        int CRYPTO_TRUE = 1;
    };

    /**
     * <i>native declaration : crypto/BRCryptoAmount.h:4</i><br>
     * enum values
     */
    interface BRCryptoComparison {
        /** <i>native declaration : crypto/BRCryptoAmount.h:1</i> */
        int CRYPTO_COMPARE_LT = 0;
        /** <i>native declaration : crypto/BRCryptoAmount.h:2</i> */
        int CRYPTO_COMPARE_EQ = 1;
        /** <i>native declaration : crypto/BRCryptoAmount.h:3</i> */
        int CRYPTO_COMPARE_GT = 2;
    };


    /**
     * Original signature : <code>UInt512 cryptoAccountDeriveSeed(const char*)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:2</i>
     */
    UInt512.ByValue cryptoAccountDeriveSeed(String phrase);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountCreate(const char*)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:4</i>
     */
    BRCryptoAccount cryptoAccountCreate(String paperKey);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountCreateFromSeed(UInt512)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:6</i>
     */
    BRCryptoAccount cryptoAccountCreateFromSeed(UInt512.ByValue seed);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountCreateFromSeedBytes(const uint8_t*)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:8</i>
     */
    BRCryptoAccount cryptoAccountCreateFromSeedBytes(byte bytes[]);
    /**
     * Original signature : <code>uint64_t cryptoAccountGetTimestamp(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:10</i>
     */
    long cryptoAccountGetTimestamp(BRCryptoAccount account);
    /**
     * Original signature : <code>void cryptoAccountSetTimestamp(BRCryptoAccount, uint64_t)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:12</i>
     */
    void cryptoAccountSetTimestamp(BRCryptoAccount account, long timestamp);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountTake(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:14</i>
     */
    BRCryptoAccount cryptoAccountTake(BRCryptoAccount obj);
    /**
     * Original signature : <code>void cryptoAccountGive(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:16</i>
     */
    void cryptoAccountGive(BRCryptoAccount obj);


    /**
     * Original signature : <code>char* cryptoCurrencyGetName(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:2</i>
     */
    String cryptoCurrencyGetName(BRCryptoCurrency currency);
    /**
     * Original signature : <code>char* cryptoCurrencyGetCode(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:4</i>
     */
    String cryptoCurrencyGetCode(BRCryptoCurrency currency);
    /**
     * Original signature : <code>char* cryptoCurrencyGetType(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:6</i>
     */
    String cryptoCurrencyGetType(BRCryptoCurrency currency);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoCurrencyIsIdentical(BRCryptoCurrency, BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:8</i>
     */
    int cryptoCurrencyIsIdentical(BRCryptoCurrency c1, BRCryptoCurrency c2);
    /**
     * Original signature : <code>BRCryptoCurrency cryptoCurrencyTake(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:10</i>
     */
    BRCryptoCurrency cryptoCurrencyTake(BRCryptoCurrency obj);
    /**
     * Original signature : <code>void cryptoCurrencyGive(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:12</i>
     */
    void cryptoCurrencyGive(BRCryptoCurrency obj);


    /**
     * Original signature : <code>char* cryptoUnitGetName(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:2</i>
     */
    String cryptoUnitGetName(BRCryptoUnit unit);
    /**
     * Original signature : <code>char* cryptoUnitGetSymbol(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:4</i>
     */
    String cryptoUnitGetSymbol(BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoCurrency cryptoUnitGetCurrency(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:6</i>
     */
    BRCryptoCurrency cryptoUnitGetCurrency(BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoUnit cryptoUnitGetBaseUnit(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:8</i>
     */
    BRCryptoUnit cryptoUnitGetBaseUnit(BRCryptoUnit unit);
    /**
     * Original signature : <code>uint8_t cryptoUnitGetBaseDecimalOffset(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:10</i>
     */
    byte cryptoUnitGetBaseDecimalOffset(BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoUnitIsCompatible(BRCryptoUnit, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:12</i>
     */
    int cryptoUnitIsCompatible(BRCryptoUnit u1, BRCryptoUnit u2);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoUnitIsIdentical(BRCryptoUnit, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:14</i>
     */
    int cryptoUnitIsIdentical(BRCryptoUnit u1, BRCryptoUnit u2);
    /**
     * Original signature : <code>BRCryptoUnit cryptoUnitTake(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:16</i>
     */
    BRCryptoUnit cryptoUnitTake(BRCryptoUnit obj);
    /**
     * Original signature : <code>void cryptoUnitGive(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:18</i>
     */
    void cryptoUnitGive(BRCryptoUnit obj);


    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountCreateDouble(double, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:7</i>
     */
    BRCryptoAmount cryptoAmountCreateDouble(double value, BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountCreateInteger(int64_t, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:9</i>
     */
    BRCryptoAmount cryptoAmountCreateInteger(long value, BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountCreateString(const char*, BRCryptoBoolean, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:11</i>
     */
    BRCryptoAmount cryptoAmountCreateString(String value, int isNegative, BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoCurrency cryptoAmountGetCurrency(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:13</i>
     */
    BRCryptoCurrency cryptoAmountGetCurrency(BRCryptoAmount amount);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoAmountIsNegative(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:15</i>
     */
    int cryptoAmountIsNegative(BRCryptoAmount amount);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoAmountIsCompatible(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:17</i>
     */
    int cryptoAmountIsCompatible(BRCryptoAmount a1, BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoComparison cryptoAmountCompare(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:19</i>
     */
    int cryptoAmountCompare(BRCryptoAmount a1, BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountAdd(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:21</i>
     */
    BRCryptoAmount cryptoAmountAdd(BRCryptoAmount a1, BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountSub(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:23</i>
     */
    BRCryptoAmount cryptoAmountSub(BRCryptoAmount a1, BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountNegate(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:25</i>
     */
    BRCryptoAmount cryptoAmountNegate(BRCryptoAmount amount);
    /**
     * Original signature : <code>double cryptoAmountGetDouble(BRCryptoAmount, BRCryptoUnit, BRCryptoBoolean*)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:27</i>
     */
    double cryptoAmountGetDouble(BRCryptoAmount amount, BRCryptoUnit unit, IntByReference overflow);
    /**
     * Original signature : <code>uint64_t cryptoAmountGetIntegerRaw(BRCryptoAmount, BRCryptoBoolean*)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:29</i>
     */
    long cryptoAmountGetIntegerRaw(BRCryptoAmount amount, IntByReference overflow);
    /**
     * Original signature : <code>UInt256 cryptoAmountGetValue(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:31</i>
     */
    UInt256.ByValue cryptoAmountGetValue(BRCryptoAmount amount);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountTake(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:33</i>
     */
    BRCryptoAmount cryptoAmountTake(BRCryptoAmount obj);
    /**
     * Original signature : <code>void cryptoAmountGive(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:35</i>
     */
    void cryptoAmountGive(BRCryptoAmount obj);


    /**
     * Original signature : <code>char* cryptoAddressAsString(BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:2</i>
     */
    Pointer cryptoAddressAsString(BRCryptoAddress address);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoAddressIsIdentical(BRCryptoAddress, BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:4</i>
     */
    int cryptoAddressIsIdentical(BRCryptoAddress a1, BRCryptoAddress a2);
    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressTake(BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:6</i>
     */
    BRCryptoAddress cryptoAddressTake(BRCryptoAddress obj);
    /**
     * Original signature : <code>void cryptoAddressGive(BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:8</i>
     */
    void cryptoAddressGive(BRCryptoAddress obj);


    /**
     * Original signature : <code>BRMasterPubKey cryptoAccountAsBTC(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:23</i>
     */
    BRMasterPubKey.ByValue cryptoAccountAsBTC(BRCryptoAccount account);


    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:8</i>
     */
    BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress.ByValue btc);
    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressCreateAsEth(BREthereumAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:8</i>
     */
    BRCryptoAddress cryptoAddressCreateAsETH(BREthereumAddress.ByValue address);

    /**
     * Original signature : <code>int BRAddressIsValid(const char*)</code><br>
     * <i>native declaration : support/BRAddress.h</i>
     */
    int BRAddressIsValid(String addr);

    /**
     * Original signature : <code>BREthereumAddress addressCreate(const char*)</code><br>
     * <i>native declaration : ethereum/base/BREthereumAddress.h</i>
     */
    BREthereumAddress.ByValue addressCreate(String address);

    /**
     * Original signature : <code>int addressValidateString(const char*)</code><br>
     * <i>native declaration : ethereum/base/BREthereumAddress.h</i>
     */
    int addressValidateString(String addr);


    /**
     * returns the first unused external address (legacy pay-to-pubkey-hash)<br>
     * Original signature : <code>BRAddress BRWalletLegacyAddress(BRWallet*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:37</i>
     */
    BRAddress.ByValue BRWalletLegacyAddress(BRWallet wallet);
    /**
     * true if the address was previously generated by BRWalletUnusedAddrs() (even if it's now used)<br>
     * Original signature : <code>int BRWalletContainsAddress(BRWallet*, const char*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:47</i>
     */
    int BRWalletContainsAddress(BRWallet wallet, String addr);
    /**
     * current wallet balance, not including transactions known to be invalid<br>
     * Original signature : <code>uint64_t BRWalletBalance(BRWallet*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:67</i>
     */
    long BRWalletBalance(BRWallet wallet);
    /**
     * fee-per-kb of transaction size to use when creating a transaction<br>
     * Original signature : <code>uint64_t BRWalletFeePerKb(BRWallet*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:87</i>
     */
    long BRWalletFeePerKb(BRWallet wallet);
    /**
     * Original signature : <code>void BRWalletSetFeePerKb(BRWallet*, uint64_t)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:89</i>
     */
    void BRWalletSetFeePerKb(BRWallet wallet, long feePerKb);
    /**
     * returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)<br>
     * Original signature : <code>uint64_t BRWalletAmountReceivedFromTx(BRWallet*, const BRTransaction*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:154</i>
     */
    long BRWalletAmountReceivedFromTx(BRWallet wallet, BRTransaction tx);
    /**
     * returns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)<br>
     * Original signature : <code>uint64_t BRWalletAmountSentByTx(BRWallet*, const BRTransaction*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:159</i>
     */
    long BRWalletAmountSentByTx(BRWallet wallet, BRTransaction tx);
    /**
     * fee that will be added for a transaction of the given amount<br>
     * Original signature : <code>uint64_t BRWalletFeeForTx(BRWallet*, const BRTransaction *)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:179</i>
     */
    long BRWalletFeeForTx(BRWallet wallet, BRTransaction tx);
    /**
     * fee that will be added for a transaction of the given amount<br>
     * Original signature : <code>uint64_t BRWalletFeeForTxAmount(BRWallet*, uint64_t)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:179</i>
     */
    long BRWalletFeeForTxAmount(BRWallet wallet, long amount);
    /**
     * frees memory allocated for wallet, and calls BRTransactionFree() for all registered transactions<br>
     * Original signature : <code>void BRWalletFree(BRWallet*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:194</i>
     */
    void BRWalletFree(BRWallet wallet);


    /**
     * Original signature : <code>UInt256 createUInt256 (uint64_t)</code><br>
     * <i>native declaration : ethereum/util/BRUtilMath.h</i>
     */
    UInt256.ByValue createUInt256(long value);
    /**
     * Original signature : <code>char * coerceStringPrefaced (UInt256, int base, const char *)</code><br>
     * <i>native declaration : ethereum/util/BRUtilMath.h</i>
     */
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);


    /** <i>native declaration : bitcoin/BRWalletManager.h:9</i> */
    interface BRGetBlockNumberCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, int rid);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:12</i> */
    interface BRGetTransactionsCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, long begBlockNumber, long endBlockNumber, int rid);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:20</i> */
    interface BRSubmitTransactionCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRTransaction transaction, int rid);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:39</i> */
    interface BRTransactionEventCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRTransaction transaction, BRTransactionEvent.ByValue event);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:62</i> */
    interface BRWalletEventCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRWalletEvent.ByValue event);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:85</i> */
    interface BRWalletManagerEventCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWalletManagerEvent.ByValue event);
    };
    /**
     * Original signature : <code>BRWalletManager BRWalletManagerNew(BRWalletManagerClient, BRMasterPubKey, const BRChainParams*, uint32_t, BRSyncMode, const char*)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:96</i>
     */
    BRWalletManager BRWalletManagerNew(BRWalletManagerClient.ByValue client, BRMasterPubKey.ByValue mpk, BRChainParams params, int earliestKeyTime, int mode, String storagePath);
    /**
     * Original signature : <code>void BRWalletManagerFree(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:98</i>
     */
    void BRWalletManagerFree(BRWalletManager manager);
    /**
     * Original signature : <code>void BRWalletManagerConnect(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:100</i>
     */
    void BRWalletManagerConnect(BRWalletManager manager);
    /**
     * Original signature : <code>void BRWalletManagerDisconnect(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:102</i>
     */
    void BRWalletManagerDisconnect(BRWalletManager manager);
    /**
     * Original signature : <code>void BRWalletManagerScan(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:104</i>
     */
    void BRWalletManagerScan(BRWalletManager manager);
    /**
     * Original signature : <code>BRWallet* BRWalletManagerGetWallet(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:117</i>
     */
    BRWallet BRWalletManagerGetWallet(BRWalletManager manager);


    // TODO: Why are these not in their corresponding header file
    BRCryptoAmount cryptoAmountCreate (BRCryptoCurrency currency, int isNegative, UInt256.ByValue value);
    BRCryptoCurrency cryptoCurrencyCreate(String name, String code, String type);
    BRCryptoUnit cryptoUnitCreateAsBase(BRCryptoCurrency currency, String name, String symbol);
    BRCryptoUnit cryptoUnitCreate(BRCryptoCurrency currency, String name, String symbol, BRCryptoUnit base, byte decimals);
}
