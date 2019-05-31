package com.breadwallet.crypto.jni.bitcoin;

/**
 * <i>native declaration : bitcoin/BRWalletManager.h:70</i><br>
 * enum values
 */
public interface BRWalletManagerEventType {
    /** <i>native declaration : bitcoin/BRWalletManager.h:64</i> */
    int BITCOIN_WALLET_MANAGER_CREATED = 0;
    /** <i>native declaration : bitcoin/BRWalletManager.h:65</i> */
    int BITCOIN_WALLET_MANAGER_CONNECTED = 1;
    /** <i>native declaration : bitcoin/BRWalletManager.h:66</i> */
    int BITCOIN_WALLET_MANAGER_DISCONNECTED = 2;
    /** <i>native declaration : bitcoin/BRWalletManager.h:67</i> */
    int BITCOIN_WALLET_MANAGER_SYNC_STARTED = 3;
    /** <i>native declaration : bitcoin/BRWalletManager.h:68</i> */
    int BITCOIN_WALLET_MANAGER_SYNC_STOPPED = 4;
    /** <i>native declaration : bitcoin/BRWalletManager.h:69</i> */
    int BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED = 5;
}
