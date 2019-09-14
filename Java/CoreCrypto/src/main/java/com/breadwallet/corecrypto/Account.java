/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoAccount;
import com.google.common.base.Optional;

import java.util.Date;
import java.util.List;
import java.lang.String;

/* package */
final class Account implements com.breadwallet.crypto.Account {

    /**
     * Recover an account from a BIP-39 'paper key'.
     *
     * @apiNote The caller should take appropriate security measures, like enclosing this method's call in a
     * try-finally block that wipes the phraseUtf8 value, to ensure that it is purged from memory
     * upon completion.
     *
     * @param phraseUtf8 The UTF-8 NFKD normalized BIP-39 paper key
     * @param timestamp The timestamp of when this account was first created
     * @param uids The unique identifier of this account
     */
    static Account createFromPhrase(byte[] phraseUtf8, Date timestamp, String uids) {
        return new Account(CoreBRCryptoAccount.createFromPhrase(phraseUtf8, timestamp));
    }

    /**
     * Create an account based on an account serialization.
     *
     * @param serialization The result of a prior call to {@link Account#serialize()}
     * @param uids The unique identifier of this account
     *
     * @return The serialization's corresponding account or {@link Optional#absent()} if the serialization is invalid.
     *         If the serialization is invalid then the account <b>must be recreated</b> from the `phrase`
     *         (aka 'Paper Key').  A serialization will be invald when the serialization format changes
     *         which will <b>always occur</b> when a new blockchain is added.  For example, when XRP is added
     *         the XRP public key must be serialized; the old serialization w/o the XRP public key will
     *         be invalid and the `phrase` is <b>required</b> in order to produce the XRP public key.
     */
    static Optional<Account> createFromSerialization(byte[] serialization, String uids) {
        return CoreBRCryptoAccount.createFromSerialization(serialization).transform(Account::new);
    }

    /**
     * Generate a BIP-39 'paper Key'
     *
     * Use {@link Account#createFromPhrase(byte[], Date, String)} to get the account
     *
     * @return A UTF-8 NFKD normalized BIP-39 paper key
     */
    static byte[] generatePhrase(List<String> words) {
        return CoreBRCryptoAccount.generatePhrase(words);
    }

    /**
     * Validate a phrase as a BIP-39 'paper key'
     *
     * @param phraseUtf8 The UTF-8 NFKD normalized BIP-39 paper key
     * @param words A locale-specific BIP-39-defined array of 2048 words.
     *
     * @return true is a valid paper key; false otherwise
     */
    static boolean validatePhrase(byte[] phraseUtf8, List<String> words) {
        return CoreBRCryptoAccount.validatePhrase(phraseUtf8, words);
    }

    static Account create(CoreBRCryptoAccount core) {
        return new Account(core);
    }

    /* package */
    static Account from(com.breadwallet.crypto.Account account) {
        if (account instanceof Account) {
            return (Account) account;
        }
        throw new IllegalArgumentException("Unsupported account instance");
    }

    private final CoreBRCryptoAccount core;

    private Account(CoreBRCryptoAccount core) {
        this.core = core;
    }

    @Override
    public Date getTimestamp() {
        return core.getTimestamp();
    }

    @Override
    public byte[] serialize() {
        return core.serialize();
    }

    @Override
    public boolean validate(byte[] serialization) {
        return core.validate(serialization);
    }

    /* package */
    CoreBRCryptoAccount getCoreBRCryptoAccount() {
        return core;
    }
}
