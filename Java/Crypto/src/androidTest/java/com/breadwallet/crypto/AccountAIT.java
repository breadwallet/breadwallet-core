package com.breadwallet.crypto;

import com.google.common.base.Optional;

import org.junit.Test;
import static org.junit.Assert.*;

public class AccountAIT {

    @Test
    public void testAccountCreateFromPhrase() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Optional<Account> account = Account.createFrom(phrase, uids);
        assertTrue(account.isPresent());
        assertEquals(0, account.get().getTimestamp());

        // TODO: Add addressAsETH
    }

    @Test
    public void testAccountDeriveSeed() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        byte[] seed = Account.deriveSeed(phrase);
        String uids = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";
        Optional<Account> account = Account.createFrom(seed, uids);
        assertTrue(account.isPresent());
        assertEquals(0, account.get().getTimestamp());
        // TODO: Add addressAsETH
    }
}
