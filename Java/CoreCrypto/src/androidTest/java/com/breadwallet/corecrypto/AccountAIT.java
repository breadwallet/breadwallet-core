package com.breadwallet.corecrypto;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
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

        // TODO: Add addressAsETH
    }

    @Test
    public void testAccountDeriveSeed() {
        byte[] phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8);
        byte[] seed = Account.deriveSeed(phrase);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date timestamp = new Date(0);
        Account account = Account.createFromSeed(seed, timestamp, uids);
        assertEquals(timestamp.getTime(), account.getTimestamp().getTime());

        // TODO: Add addressAsETH
    }
}
