package com.breadwallet.crypto.jni;

import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

public interface CryptoLibrary extends Library {
    String JNA_LIBRARY_NAME = "crypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);


    /** <i>native declaration : bitcoin/BRWallet.h</i> */
    public static final long DEFAULT_FEE_PER_KB = (long)(1000L * 10);


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
     * <i>native declaration : bitcoin/BRWalletManager.h:6</i><br>
     * enum values
     */
    interface BRWalletForkId {
        /** <i>native declaration : bitcoin/BRWalletManager.h:3</i> */
        public static final int WALLET_FORKID_BITCOIN = 0x00;
        /** <i>native declaration : bitcoin/BRWalletManager.h:4</i> */
        public static final int WALLET_FORKID_BITCASH = 0x40;
        /** <i>native declaration : bitcoin/BRWalletManager.h:5</i> */
        public static final int WALLET_FORKID_BITGOLD = 0x4f;
    };
    /**
     * <i>native declaration : bitcoin/BRWalletManager.h:27</i><br>
     * enum values
     */
    interface BRTransactionEventType {
        /** <i>native declaration : bitcoin/BRWalletManager.h:24</i> */
        public static final int BITCOIN_TRANSACTION_ADDED = 0;
        /** <i>native declaration : bitcoin/BRWalletManager.h:25</i> */
        public static final int BITCOIN_TRANSACTION_UPDATED = 1;
        /** <i>native declaration : bitcoin/BRWalletManager.h:26</i> */
        public static final int BITCOIN_TRANSACTION_DELETED = 2;
    };
    /**
     * <i>native declaration : bitcoin/BRWalletManager.h:45</i><br>
     * enum values
     */
    interface BRWalletEventType {
        /** <i>native declaration : bitcoin/BRWalletManager.h:41</i> */
        public static final int BITCOIN_WALLET_CREATED = 0;
        /** <i>native declaration : bitcoin/BRWalletManager.h:42</i> */
        public static final int BITCOIN_WALLET_BALANCE_UPDATED = 1;
        /** <i>native declaration : bitcoin/BRWalletManager.h:43</i> */
        public static final int BITCOIN_WALLET_TRANSACTION_SUBMITTED = 2;
        /** <i>native declaration : bitcoin/BRWalletManager.h:44</i> */
        public static final int BITCOIN_WALLET_DELETED = 3;
    };
    /**
     * <i>native declaration : bitcoin/BRWalletManager.h:70</i><br>
     * enum values
     */
    interface BRWalletManagerEventType {
        /** <i>native declaration : bitcoin/BRWalletManager.h:64</i> */
        public static final int BITCOIN_WALLET_MANAGER_CREATED = 0;
        /** <i>native declaration : bitcoin/BRWalletManager.h:65</i> */
        public static final int BITCOIN_WALLET_MANAGER_CONNECTED = 1;
        /** <i>native declaration : bitcoin/BRWalletManager.h:66</i> */
        public static final int BITCOIN_WALLET_MANAGER_DISCONNECTED = 2;
        /** <i>native declaration : bitcoin/BRWalletManager.h:67</i> */
        public static final int BITCOIN_WALLET_MANAGER_SYNC_STARTED = 3;
        /** <i>native declaration : bitcoin/BRWalletManager.h:68</i> */
        public static final int BITCOIN_WALLET_MANAGER_SYNC_STOPPED = 4;
        /** <i>native declaration : bitcoin/BRWalletManager.h:69</i> */
        public static final int BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED = 5;
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
    com.breadwallet.crypto.jni.UInt512.ByValue cryptoAccountDeriveSeed(String phrase);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountCreate(const char*)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:4</i>
     */
    CryptoLibrary.BRCryptoAccount cryptoAccountCreate(String paperKey);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountCreateFromSeed(UInt512)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:6</i>
     */
    CryptoLibrary.BRCryptoAccount cryptoAccountCreateFromSeed(com.breadwallet.crypto.jni.UInt512.ByValue seed);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountCreateFromSeedBytes(const uint8_t*)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:8</i>
     */
    CryptoLibrary.BRCryptoAccount cryptoAccountCreateFromSeedBytes(byte bytes[]);
    /**
     * Original signature : <code>uint64_t cryptoAccountGetTimestamp(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:10</i>
     */
    long cryptoAccountGetTimestamp(CryptoLibrary.BRCryptoAccount account);
    /**
     * Original signature : <code>void cryptoAccountSetTimestamp(BRCryptoAccount, uint64_t)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:12</i>
     */
    void cryptoAccountSetTimestamp(CryptoLibrary.BRCryptoAccount account, long timestamp);
    /**
     * Original signature : <code>BRCryptoAccount cryptoAccountTake(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:14</i>
     */
    CryptoLibrary.BRCryptoAccount cryptoAccountTake(CryptoLibrary.BRCryptoAccount obj);
    /**
     * Original signature : <code>void cryptoAccountGive(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoAccount.h:16</i>
     */
    void cryptoAccountGive(CryptoLibrary.BRCryptoAccount obj);


    /**
     * Original signature : <code>char* cryptoCurrencyGetName(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:2</i>
     */
    String cryptoCurrencyGetName(CryptoLibrary.BRCryptoCurrency currency);
    /**
     * Original signature : <code>char* cryptoCurrencyGetCode(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:4</i>
     */
    String cryptoCurrencyGetCode(CryptoLibrary.BRCryptoCurrency currency);
    /**
     * Original signature : <code>char* cryptoCurrencyGetType(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:6</i>
     */
    String cryptoCurrencyGetType(CryptoLibrary.BRCryptoCurrency currency);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoCurrencyIsIdentical(BRCryptoCurrency, BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:8</i>
     */
    int cryptoCurrencyIsIdentical(CryptoLibrary.BRCryptoCurrency c1, CryptoLibrary.BRCryptoCurrency c2);
    /**
     * Original signature : <code>BRCryptoCurrency cryptoCurrencyTake(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:10</i>
     */
    CryptoLibrary.BRCryptoCurrency cryptoCurrencyTake(CryptoLibrary.BRCryptoCurrency obj);
    /**
     * Original signature : <code>void cryptoCurrencyGive(BRCryptoCurrency)</code><br>
     * <i>native declaration : crypto/BRCryptoCurrency.h:12</i>
     */
    void cryptoCurrencyGive(CryptoLibrary.BRCryptoCurrency obj);


    /**
     * Original signature : <code>char* cryptoUnitGetName(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:2</i>
     */
    String cryptoUnitGetName(CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>char* cryptoUnitGetSymbol(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:4</i>
     */
    String cryptoUnitGetSymbol(CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoCurrency cryptoUnitGetCurrency(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:6</i>
     */
    CryptoLibrary.BRCryptoCurrency cryptoUnitGetCurrency(CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoUnit cryptoUnitGetBaseUnit(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:8</i>
     */
    CryptoLibrary.BRCryptoUnit cryptoUnitGetBaseUnit(CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>uint8_t cryptoUnitGetBaseDecimalOffset(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:10</i>
     */
    byte cryptoUnitGetBaseDecimalOffset(CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoUnitIsCompatible(BRCryptoUnit, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:12</i>
     */
    int cryptoUnitIsCompatible(CryptoLibrary.BRCryptoUnit u1, CryptoLibrary.BRCryptoUnit u2);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoUnitIsIdentical(BRCryptoUnit, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:14</i>
     */
    int cryptoUnitIsIdentical(CryptoLibrary.BRCryptoUnit u1, CryptoLibrary.BRCryptoUnit u2);
    /**
     * Original signature : <code>BRCryptoUnit cryptoUnitTake(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:16</i>
     */
    CryptoLibrary.BRCryptoUnit cryptoUnitTake(CryptoLibrary.BRCryptoUnit obj);
    /**
     * Original signature : <code>void cryptoUnitGive(BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoUnit.h:18</i>
     */
    void cryptoUnitGive(CryptoLibrary.BRCryptoUnit obj);


    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountCreateDouble(double, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:7</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountCreateDouble(double value, CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountCreateInteger(int64_t, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:9</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountCreateInteger(long value, CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountCreateString(const char*, BRCryptoBoolean, BRCryptoUnit)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:11</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountCreateString(String value, int isNegative, CryptoLibrary.BRCryptoUnit unit);
    /**
     * Original signature : <code>BRCryptoCurrency cryptoAmountGetCurrency(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:13</i>
     */
    CryptoLibrary.BRCryptoCurrency cryptoAmountGetCurrency(CryptoLibrary.BRCryptoAmount amount);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoAmountIsNegative(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:15</i>
     */
    int cryptoAmountIsNegative(CryptoLibrary.BRCryptoAmount amount);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoAmountIsCompatible(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:17</i>
     */
    int cryptoAmountIsCompatible(CryptoLibrary.BRCryptoAmount a1, CryptoLibrary.BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoComparison cryptoAmountCompare(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:19</i>
     */
    int cryptoAmountCompare(CryptoLibrary.BRCryptoAmount a1, CryptoLibrary.BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountAdd(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:21</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountAdd(CryptoLibrary.BRCryptoAmount a1, CryptoLibrary.BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountSub(BRCryptoAmount, BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:23</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountSub(CryptoLibrary.BRCryptoAmount a1, CryptoLibrary.BRCryptoAmount a2);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountNegate(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:25</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountNegate(CryptoLibrary.BRCryptoAmount amount);
    /**
     * Original signature : <code>double cryptoAmountGetDouble(BRCryptoAmount, BRCryptoUnit, BRCryptoBoolean*)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:27</i>
     */
    double cryptoAmountGetDouble(CryptoLibrary.BRCryptoAmount amount, CryptoLibrary.BRCryptoUnit unit, IntByReference overflow);
    /**
     * Original signature : <code>uint64_t cryptoAmountGetIntegerRaw(BRCryptoAmount, BRCryptoBoolean*)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:29</i>
     */
    long cryptoAmountGetIntegerRaw(CryptoLibrary.BRCryptoAmount amount, IntByReference overflow);
    /**
     * Original signature : <code>UInt256 cryptoAmountGetValue(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:31</i>
     */
    UInt256.ByValue cryptoAmountGetValue(CryptoLibrary.BRCryptoAmount amount);
    /**
     * Original signature : <code>BRCryptoAmount cryptoAmountTake(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:33</i>
     */
    CryptoLibrary.BRCryptoAmount cryptoAmountTake(CryptoLibrary.BRCryptoAmount obj);
    /**
     * Original signature : <code>void cryptoAmountGive(BRCryptoAmount)</code><br>
     * <i>native declaration : crypto/BRCryptoAmount.h:35</i>
     */
    void cryptoAmountGive(CryptoLibrary.BRCryptoAmount obj);


    /**
     * Original signature : <code>char* cryptoAddressAsString(BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:2</i>
     */
    Pointer cryptoAddressAsString(CryptoLibrary.BRCryptoAddress address);
    /**
     * Original signature : <code>BRCryptoBoolean cryptoAddressIsIdentical(BRCryptoAddress, BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:4</i>
     */
    int cryptoAddressIsIdentical(CryptoLibrary.BRCryptoAddress a1, CryptoLibrary.BRCryptoAddress a2);
    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressTake(BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:6</i>
     */
    CryptoLibrary.BRCryptoAddress cryptoAddressTake(CryptoLibrary.BRCryptoAddress obj);
    /**
     * Original signature : <code>void cryptoAddressGive(BRCryptoAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoAddress.h:8</i>
     */
    void cryptoAddressGive(CryptoLibrary.BRCryptoAddress obj);


    /**
     * Original signature : <code>BRMasterPubKey cryptoAccountAsBTC(BRCryptoAccount)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:23</i>
     */
    BRMasterPubKey.ByValue cryptoAccountAsBTC(CryptoLibrary.BRCryptoAccount account);


    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:8</i>
     */
    CryptoLibrary.BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress.ByValue btc);
    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressCreateAsEth(BREthereumAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:8</i>
     */
    CryptoLibrary.BRCryptoAddress cryptoAddressCreateAsETH(BREthereumAddress.ByValue address);

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
    BRAddress.ByValue BRWalletLegacyAddress(CryptoLibrary.BRWallet wallet);
    /**
     * current wallet balance, not including transactions known to be invalid<br>
     * Original signature : <code>uint64_t BRWalletBalance(BRWallet*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:67</i>
     */
    long BRWalletBalance(CryptoLibrary.BRWallet wallet);
    /**
     * fee-per-kb of transaction size to use when creating a transaction<br>
     * Original signature : <code>uint64_t BRWalletFeePerKb(BRWallet*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:87</i>
     */
    long BRWalletFeePerKb(CryptoLibrary.BRWallet wallet);
    /**
     * Original signature : <code>void BRWalletSetFeePerKb(BRWallet*, uint64_t)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:89</i>
     */
    void BRWalletSetFeePerKb(CryptoLibrary.BRWallet wallet, long feePerKb);
    /**
     * fee that will be added for a transaction of the given amount<br>
     * Original signature : <code>uint64_t BRWalletFeeForTxAmount(BRWallet*, uint64_t)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:179</i>
     */
    long BRWalletFeeForTxAmount(CryptoLibrary.BRWallet wallet, long amount);

    /**
     * Original signature : <code>char * coerceStringPrefaced (UInt256, int base, const char *)</code><br>
     * <i>native declaration : ethereum/util/BRUtilMath.h</i>
     */
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

    /** <i>native declaration : bitcoin/BRWalletManager.h:9</i> */
    interface BRGetBlockNumberCallback extends Callback {
        void apply(Pointer context, Pointer manager, int rid);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:12</i> */
    interface BRGetTransactionsCallback extends Callback {
        void apply(Pointer context, Pointer manager, long begBlockNumber, long endBlockNumber, int rid);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:20</i> */
    interface BRSubmitTransactionCallback extends Callback {
        void apply(Pointer context, Pointer manager, Pointer wallet, BRTransaction transaction, int rid);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:39</i> */
    interface BRTransactionEventCallback extends Callback {
        void apply(Pointer context, Pointer manager, Pointer wallet, Pointer transaction, BRTransactionEvent.ByValue event);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:62</i> */
    interface BRWalletEventCallback extends Callback {
        void apply(Pointer context, Pointer manager, Pointer wallet, BRWalletEvent.ByValue event);
    };
    /** <i>native declaration : bitcoin/BRWalletManager.h:85</i> */
    interface BRWalletManagerEventCallback extends Callback {
        void apply(Pointer context, Pointer manager, BRWalletManagerEvent.ByValue event);
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
    void BRWalletManagerFree(CryptoLibrary.BRWalletManager manager);
    /**
     * Original signature : <code>void BRWalletManagerConnect(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:100</i>
     */
    void BRWalletManagerConnect(CryptoLibrary.BRWalletManager manager);
    /**
     * Original signature : <code>void BRWalletManagerDisconnect(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:102</i>
     */
    void BRWalletManagerDisconnect(CryptoLibrary.BRWalletManager manager);
    /**
     * Original signature : <code>void BRWalletManagerScan(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:104</i>
     */
    void BRWalletManagerScan(CryptoLibrary.BRWalletManager manager);
    /**
     * Original signature : <code>BRWallet* BRWalletManagerGetWallet(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:117</i>
     */
    BRWallet BRWalletManagerGetWallet(CryptoLibrary.BRWalletManager manager);


    class BRCryptoAccount extends PointerType {
        public BRCryptoAccount(Pointer address) {
            super(address);
        }
        public BRCryptoAccount() {
            super();
        }
    };

    class BRCryptoAddress extends PointerType {
        public BRCryptoAddress(Pointer address) {
            super(address);
        }
        public BRCryptoAddress() {
            super();
        }
    };

    class BRCryptoCurrency extends PointerType {
        public BRCryptoCurrency(Pointer address) {
            super(address);
        }
        public BRCryptoCurrency() {
            super();
        }
    };

    class BRCryptoUnit extends PointerType {
        public BRCryptoUnit(Pointer address) {
            super(address);
        }
        public BRCryptoUnit() {
            super();
        }
    };

    class BRCryptoAmount extends PointerType {
        public BRCryptoAmount(Pointer address) {
            super(address);
        }
        public BRCryptoAmount() {
            super();
        }
    };

    class BRCryptoWallet extends PointerType {
        public BRCryptoWallet(Pointer address) {
            super(address);
        }
        public BRCryptoWallet() {
            super();
        }
    };

    class BRCryptoWalletManager extends PointerType {
        public BRCryptoWalletManager(Pointer address) {
            super(address);
        }
        public BRCryptoWalletManager() {
            super();
        }
    };

    class BRCryptoSystem extends PointerType {
        public BRCryptoSystem(Pointer address) {
            super(address);
        }
        public BRCryptoSystem() {
            super();
        }
    };


    class BRChainParams extends PointerType {
        public BRChainParams(Pointer address) {
            super(address);
        }
        public BRChainParams() {
            super();
        }
    }
    class BRWallet extends PointerType {
        public BRWallet(Pointer address) {
            super(address);
        }
        public BRWallet() {
            super();
        }
    };
    class BRWalletManager extends PointerType {
        public BRWalletManager(Pointer address) {
            super(address);
        }
        public BRWalletManager() {
            super();
        }
    };


    // TODO: Why are these not in their corresponding header file
    CryptoLibrary.BRCryptoCurrency cryptoCurrencyCreate(String name, String code, String type);
    CryptoLibrary.BRCryptoUnit cryptoUnitCreateAsBase(CryptoLibrary.BRCryptoCurrency currency, String name, String symbol);
    CryptoLibrary.BRCryptoUnit cryptoUnitCreate(CryptoLibrary.BRCryptoCurrency currency, String name, String symbol, CryptoLibrary.BRCryptoUnit base, byte decimals);
}
