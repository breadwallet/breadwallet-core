package com.breadwallet.crypto.libcrypto.bitcoin;

/**
 * <i>native declaration : bitcoin/BRWalletManager.h:45</i><br>
 * enum values
 */
public interface BRWalletEventType {
    /** <i>native declaration : bitcoin/BRWalletManager.h:41</i> */
    int BITCOIN_WALLET_CREATED = 0;
    /** <i>native declaration : bitcoin/BRWalletManager.h:42</i> */
    int BITCOIN_WALLET_BALANCE_UPDATED = 1;
    /** <i>native declaration : bitcoin/BRWalletManager.h:43</i> */
    int BITCOIN_WALLET_TRANSACTION_SUBMITTED = 2;
    /** <i>native declaration : bitcoin/BRWalletManager.h:44</i> */
    int BITCOIN_WALLET_DELETED = 3;
}
