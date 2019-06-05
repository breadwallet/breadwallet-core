package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Account;

import org.junit.Test;

import java.util.Date;

import static org.junit.Assert.*;

public class AccountAIT {

    @Test
    public void testAccountCreateFromPhrase() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date earliestKeyTime = new Date(0);
        Account account = AccountImpl.createFrom(phrase, uids, earliestKeyTime);
        assertEquals(earliestKeyTime.getTime(), account.getEarliestKeyTime().getTime());

        // TODO: Add addressAsETH
    }

    @Test
    public void testAccountDeriveSeed() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        byte[] seed = AccountImpl.deriveSeed(phrase);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Date earliestKeyTime = new Date(0);
        Account account = AccountImpl.createFrom(seed, uids, earliestKeyTime);
        assertEquals(earliestKeyTime.getTime(), account.getEarliestKeyTime().getTime());

        // TODO: Add addressAsETH
    }
}
