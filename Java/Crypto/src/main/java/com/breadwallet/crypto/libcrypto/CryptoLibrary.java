package com.breadwallet.crypto.libcrypto;

import com.breadwallet.crypto.libcrypto.bitcoin.BRChainParams;
import com.breadwallet.crypto.libcrypto.bitcoin.BRPeerManager;
import com.breadwallet.crypto.libcrypto.bitcoin.BRPeerManager.BRPeerManagerPublishTxCallback;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoAccount;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWallet;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManager;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManagerClient;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoAddress;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoAmount;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoCurrency;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoUnit;
import com.breadwallet.crypto.libcrypto.support.BRAddress;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransaction;
import com.breadwallet.crypto.libcrypto.ethereum.BREthereumAddress;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;
import com.breadwallet.crypto.libcrypto.support.UInt256;
import com.breadwallet.crypto.libcrypto.support.UInt512;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;

public interface CryptoLibrary extends Library {

    String JNA_LIBRARY_NAME = "crypto";

    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);

    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);

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
    BRCryptoAccount cryptoAccountCreateFromSeedBytes(byte[] bytes);
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
     * result must be freed using BRTransactionFree()<br>
     * Original signature : <code>BRTransaction* BRWalletCreateTransaction(BRWallet*, uint64_t, const char*)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:94</i>
     */
    BRTransaction BRWalletCreateTransaction(BRWallet wallet, long amount, String addr);
    /**
     * returns true if all inputs were signed, or false if there was an error or not all inputs were able to be signed<br>
     * Original signature : <code>int BRWalletSignTransaction(BRWallet*, BRTransaction*, const void*, size_t)</code><br>
     * <i>native declaration : bitcoin/BRWallet.h:104</i>
     */
    int BRWalletSignTransaction(BRWallet wallet, BRTransaction tx, byte[] seed, SizeT seedLen);
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
     * retruns a transaction that must be freed by calling BRTransactionFree()<br>
     * Original signature : <code>BRTransaction* BRTransactionParse(const uint8_t*, size_t)</code><br>
     * <i>native declaration : bitcoin/BRTransaction.h:61</i>
     */
    BRTransaction BRTransactionParse(byte[] buf, SizeT bufLen);
    /**
     * returns a deep copy of tx and that must be freed by calling BRTransactionFree()<br>
     * Original signature : <code>BRTransaction* BRTransactionCopy(const BRTransaction*)</code><br>
     * <i>native declaration : bitcoin/BRTransaction.h:56</i>
     */
    BRTransaction BRTransactionCopy(BRTransaction tx);
    /**
     * (tx->blockHeight and tx->timestamp are not serialized)<br>
     * Original signature : <code>size_t BRTransactionSerialize(const BRTransaction*, uint8_t*, size_t)</code><br>
     * <i>native declaration : bitcoin/BRTransaction.h:66</i>
     */
    SizeT BRTransactionSerialize(BRTransaction tx, byte[] buf, SizeT bufLen);
    /**
     * frees memory allocated for tx<br>
     * Original signature : <code>void BRTransactionFree(BRTransaction*)</code><br>
     * <i>native declaration : bitcoin/BRTransaction.h:130</i>
     */
    void BRTransactionFree(BRTransaction tx);


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


    /**
     * Original signature : <code>int bwmAnnounceBlockNumber(BRWalletManager, int, uint64_t)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:11</i>
     */
    int bwmAnnounceBlockNumber(BRWalletManager manager, int rid, long blockNumber);
    /**
     * success - data is valid<br>
     * Original signature : <code>int bwmAnnounceTransaction(BRWalletManager, int, BRTransaction*)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:17</i>
     */
    int bwmAnnounceTransaction(BRWalletManager manager, int id, BRTransaction transaction);
    /**
     * Original signature : <code>void bwmAnnounceTransactionComplete(BRWalletManager, int, int)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:19</i>
     */
    void bwmAnnounceTransactionComplete(BRWalletManager manager, int id, int success);
    /**
     * Original signature : <code>void bwmAnnounceSubmit(BRWalletManager, int, BRTransaction*, int)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:22</i>
     */
    void bwmAnnounceSubmit(BRWalletManager manager, int rid, BRTransaction transaction, int error);
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
    /**
     * Original signature : <code>BRAddress* BRWalletManagerGetUnusedAddrsLegacy(BRWalletManager, uint32_t)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:115</i>
     */
    BRAddress BRWalletManagerGetUnusedAddrsLegacy(BRWalletManager manager, int limit);
    /**
     * Original signature : <code>BRPeerManager* BRWalletManagerGetPeerManager(BRWalletManager)</code><br>
     * <i>native declaration : bitcoin/BRWalletManager.h:119</i>
     */
    BRPeerManager BRWalletManagerGetPeerManager(BRWalletManager manager);


    /**
     * publishes tx to bitcoin network (do not call BRTransactionFree() on tx afterward)<br>
     * Original signature : <code>void BRPeerManagerPublishTx(BRPeerManager*, BRTransaction*, void*, BRPeerManagerPublishTxCallback*)</code><br>
     * <i>native declaration : bitcoin/BRPeerManager.h:80</i>
     */
    void BRPeerManagerPublishTx(BRPeerManager manager, BRTransaction tx, Pointer info, BRPeerManagerPublishTxCallback callback);


    /**
     * <i>native declaration : crypto/BRCryptoPrivate.h</i>
     */
    BRCryptoAmount cryptoAmountCreate (BRCryptoCurrency currency, int isNegative, UInt256.ByValue value);
    /**
     * <i>native declaration : crypto/BRCryptoPrivate.h</i>
     */
    BRCryptoCurrency cryptoCurrencyCreate(String name, String code, String type);
    /**
     * <i>native declaration : crypto/BRCryptoPrivate.h</i>
     */
    BRCryptoUnit cryptoUnitCreateAsBase(BRCryptoCurrency currency, String name, String symbol);
    /**
     * <i>native declaration : crypto/BRCryptoPrivate.h</i>
     */
    BRCryptoUnit cryptoUnitCreate(BRCryptoCurrency currency, String name, String symbol, BRCryptoUnit base, byte decimals);
}
