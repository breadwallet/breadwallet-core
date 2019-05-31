package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Account;

import org.junit.Test;
import static org.junit.Assert.*;

public class AccountAIT {

    @Test
    public void testAccountCreateFromPhrase() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Account account = AccountImpl.createFrom(phrase, uids);
        assertEquals(0, account.getTimestamp());

        // TODO: Add addressAsETH
    }

    @Test
    public void testAccountDeriveSeed() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        byte[] seed = AccountImpl.deriveSeed(phrase);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Account account = AccountImpl.createFrom(seed, uids);
        assertEquals(0, account.getTimestamp());
        // TODO: Add addressAsETH
    }
}
