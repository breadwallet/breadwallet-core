package com.breadwallet.crypto;

import com.google.common.base.Optional;

import org.junit.Test;
import static org.junit.Assert.*;

public class AccountAIT {

    @Test
    public void testAccountCreateFromPhrase() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        Optional<Account> account = Account.createFrom(phrase);
        assertTrue(account.isPresent());
        assertEquals(0, account.get().getTimestamp());

        // TODO: Add addressAsETH
    }

    @Test
    public void testAccountDeriveSeed() {
        String phrase = "ginger settle marine tissue robot crane night number ramp coast roast critic";
        byte[] seed = Account.deriveSeed(phrase);
        Optional<Account> account = Account.createFrom(seed);
        assertTrue(account.isPresent());
        assertEquals(0, account.get().getTimestamp());
        // TODO: Add addressAsETH
    }
}
