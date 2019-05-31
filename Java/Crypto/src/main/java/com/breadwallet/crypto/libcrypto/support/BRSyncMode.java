package com.breadwallet.crypto.libcrypto.support;

/**
 * <i>native declaration : support/BRSyncMode.h:5</i><br>
 * enum values
 */
public interface BRSyncMode {
    /** <i>native declaration : support/BRSyncMode.h:1</i> */
    int SYNC_MODE_BRD_ONLY = 0;
    /** <i>native declaration : support/BRSyncMode.h:2</i> */
    int SYNC_MODE_BRD_WITH_P2P_SEND = 1;
    /** <i>native declaration : support/BRSyncMode.h:3</i> */
    int SYNC_MODE_P2P_WITH_BRD_SYNC = 2;
    /** <i>native declaration : support/BRSyncMode.h:4</i> */
    int SYNC_MODE_P2P_ONLY = 3;
}
