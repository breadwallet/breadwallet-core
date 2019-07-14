package com.breadwallet.corecrypto;

import com.google.common.base.Optional;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Date;

import static org.junit.Assert.*;

public class AccountAIT {

    @Test
    public void testAccountCreateFromPhrase() {
        byte[] phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date timestamp = new Date(0);
        Account account = Account.createFromPhrase(phrase, timestamp, uids);
        assertEquals(timestamp.getTime(), account.getTimestamp().getTime());
    }

    @Test
    public void testAccountSerialization() {
        byte[] phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date timestamp = new Date(0);
        Account accountFromPhrase = Account.createFromPhrase(phrase, timestamp, uids);
        assertEquals(timestamp.getTime(), accountFromPhrase.getTimestamp().getTime());

        byte[] serialization = accountFromPhrase.serialize();
        assertArrayEquals(serialization, accountFromPhrase.serialize());

        Optional<Account> optAccount = Account.createFromSerialization(serialization, uids);
        assertTrue(optAccount.isPresent());

        Account accountFromSerialization = optAccount.get();
        assertEquals(accountFromPhrase.getTimestamp(), accountFromSerialization.getTimestamp());
        assertArrayEquals(accountFromPhrase.serialize(), accountFromSerialization.serialize());
    }
}
