/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import java.util.Date;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;

import static com.google.common.base.Preconditions.checkState;

public final class CryptoApi {

    public interface AccountProvider {
        Account createFromPhrase(byte[] phraseUtf8, Date timestamp, String uids);
        Optional<Account> createFromSerialization(byte[] serialization, String uids);
        byte[] generatePhrase(List<String> words);
        boolean validatePhrase(byte[] phraseUtf8, List<String> words);
    }

    public interface AddressProvider {
        Optional<Address> create(String address, Network network);
    }

    public interface AmountProvider {
        Amount create(long value, Unit unit);
        Amount create(double value, Unit unit);
        Optional<Amount> create(String value, boolean isNegative, Unit unit);
    }

    public interface SystemProvider {
        System create(ScheduledExecutorService executor, SystemListener listener, Account account, boolean isMainnet, String path, BlockchainDb query);
        Optional<Currency> asBDBCurrency(String uids, String name, String code, String type, UnsignedInteger decimals);
        Optional<byte[]> migrateBRCoreKeyCiphertext(Key key, byte[] nonce12, byte[] authenticatedData, byte[] ciphertext);
        void wipe(System system);
        void wipeAll(String path, List<System> exemptSystems);
    }

    public interface PaymentProvider {
        Optional<PaymentProtocolRequest> createRequestForBip70(Wallet wallet, byte[] serialization);
        Optional<PaymentProtocolRequest> createRequestForBitPay(Wallet wallet, String json);
        Optional<PaymentProtocolPaymentAck> createAckForBip70(byte[] serialization);
        Optional<PaymentProtocolPaymentAck> createAckForBitPay(String json);
    }

    public interface CoderProvider {
        Coder createCoderForAlgorithm(Coder.Algorithm algorithm);
    }

    public interface CipherProvider {
        Cipher createCipherForAesEcb(byte[] key);
        Cipher createCipherForChaCha20Poly1305(Key key, byte[] nonce12, byte[] ad);
        Cipher createCipherForPigeon(Key privKey, Key pubKey, byte[] nonce12);
    }

    public interface HasherProvider {
        Hasher createHasherForAlgorithm(Hasher.Algorithm algorithm);
    }

    public interface KeyProvider {
        void setDefaultWordList(List<String> wordList);
        List<String> getDefaultWordList();
        boolean isProtectedPrivateKeyString(byte[] keyStringUtf8);
        Optional<Key> createFromPhrase(byte[] phraseUtf8, List<String> words);
        Optional<Key> createFromPrivateKeyString(byte[] keyStringUtf8);
        Optional<Key> createFromPrivateKeyString(byte[] keyStringUtf8, byte[] passphraseUtf8);
        Optional<Key> createFromPublicKeyString(byte[] keyStringUtf8);
        Optional<Key> createForPigeon(Key key, byte[] nonce);
        Optional<Key> createForBIP32ApiAuth(byte[] phraseUtf8, List<String> words);
        Optional<Key> createForBIP32BitID(byte[] phraseUtf8, int index, String uri, List<String> words);
    }

    public interface SignerProvider {
        Signer createSignerForAlgorithm(Signer.Algorithm algorithm);
    }

    public interface Provider {
        AccountProvider accountProvider();
        AddressProvider addressProvider();
        AmountProvider amountProvider();
        SystemProvider systemProvider();
        PaymentProvider paymentProvider();

        CoderProvider coderPrivider();
        CipherProvider cipherProvider();
        HasherProvider hasherProvider();
        KeyProvider keyProvider();
        SignerProvider signerProvider();
    }

    private static Provider provider;

    public static void initialize(Provider provider) {
        checkState(null == CryptoApi.provider);
        CryptoApi.provider = provider;
    }

    /* package */
    static Provider getProvider() {
        checkState(null != CryptoApi.provider);
        return CryptoApi.provider;
    }
}
