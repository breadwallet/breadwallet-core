/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.google.common.base.Optional;

import org.junit.Ignore;
import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.Date;

import static org.junit.Assert.*;

@Ignore
public class AccountAIT {

    @Test
    public void testAccountCreateFromPhrase() {
        byte[] phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date timestamp = new Date(0);

        Account account = Account.createFromPhrase(phrase, timestamp, uids);
        assertEquals(timestamp.getTime(), account.getTimestamp().getTime());

        // check the uids matches the one provided on creation
        assertEquals(uids, account.getUids());
    }

    @Test
    public void testAccountSerialization() {
        byte[] phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date timestamp = new Date(0);
        Account accountFromPhrase = Account.createFromPhrase(phrase, timestamp, uids);

        // check that serialization is repeatable and valid
        byte[] serializationFromPhrase = accountFromPhrase.serialize();
        assertArrayEquals(serializationFromPhrase, accountFromPhrase.serialize());
        accountFromPhrase.validate(serializationFromPhrase);

        // check that account can be deserialized
        Optional<Account> optAccount = Account.createFromSerialization(serializationFromPhrase, uids);
        assertTrue(optAccount.isPresent());

        // check that public fields of the account are properly deserialized
        Account accountFromSerialization = optAccount.get();
        assertEquals(accountFromPhrase.getTimestamp(), accountFromSerialization.getTimestamp());

        // check that serialization is identical to original serialization and valid
        byte[] serializationFromSerialization = accountFromSerialization.serialize();
        assertArrayEquals(serializationFromPhrase, serializationFromSerialization);
        accountFromSerialization.validate(serializationFromSerialization);

        // check that validity is transitive
        accountFromPhrase.validate(serializationFromSerialization);
        accountFromSerialization.validate(serializationFromPhrase);

        // check the uids matches the one provided on creation
        assertEquals(uids, accountFromSerialization.getUids());
    }

    @Test
    public void testAccountPhrase() {
        byte[] phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8);
        assertTrue(Account.validatePhrase(phrase, HelpersAIT.BIP39_WORDS_EN));

        phrase = Account.generatePhrase(HelpersAIT.BIP39_WORDS_EN);
        assertTrue(Account.validatePhrase(phrase, HelpersAIT.BIP39_WORDS_EN));

        phrase = "Ask @jmo for a pithy quote".getBytes(StandardCharsets.UTF_8);
        assertFalse(Account.validatePhrase(phrase, HelpersAIT.BIP39_WORDS_EN));
    }

}
