package com.breadwallet.crypto.jni;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;

public interface CryptoLibrary extends Library {
    String JNA_LIBRARY_NAME = "crypto";
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);

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
     * Original signature : <code>BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:8</i>
     */
    CryptoLibrary.BRCryptoAddress cryptoAddressCreateAsBTC(com.breadwallet.crypto.jni.BRAddress.ByValue btc);
    /**
     * Original signature : <code>BRCryptoAddress cryptoAddressCreateAsEth(BREthereumAddress)</code><br>
     * <i>native declaration : crypto/BRCryptoPrivate.h:8</i>
     */
    CryptoLibrary.BRCryptoAddress cryptoAddressCreateAsETH(com.breadwallet.crypto.jni.BREthereumAddress.ByValue address);

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
     * Original signature : <code>char * coerceStringPrefaced (UInt256, int base, const char *)</code><br>
     * <i>native declaration : ethereum/util/BRUtilMath.h</i>
     */
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

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


    // TODO: Why are these not in their corresponding header file
    CryptoLibrary.BRCryptoCurrency cryptoCurrencyCreate(String name, String code, String type);
    CryptoLibrary.BRCryptoUnit cryptoUnitCreateAsBase(CryptoLibrary.BRCryptoCurrency currency, String name, String symbol);
    CryptoLibrary.BRCryptoUnit cryptoUnitCreate(CryptoLibrary.BRCryptoCurrency currency, String name, String symbol, CryptoLibrary.BRCryptoUnit base, byte decimals);
}
