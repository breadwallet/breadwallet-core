package com.breadwallet.corecrypto;

import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.CryptoApi;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletManagerSyncStoppedReason;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncProgressEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletAddedEvent;
import com.google.common.io.Files;
import com.google.common.util.concurrent.Uninterruptibles;

import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import okhttp3.OkHttpClient;

public class SbiAIT {

    private static final boolean MAINNET = true;
    private static final int COUNT = 50;
    private static final long EVENT_WINDOW_MS = 2 * 60 * 1000;
    private static final String SEPARATOR = " ";

    @Before
    public void setup() {
        try {
            CryptoApi.initialize(CryptoApiProvider.getInstance());
        } catch (IllegalStateException e) {
            // ignore, good to go
        }
    }

    @Test
    public void sbi() throws Exception {
        List<SbiSystem> sbiSystems = new ArrayList<>(COUNT);
        Set<SbiSystem> activeSystems = new HashSet<>(COUNT);

        // create systems
        for (int i = 0; i < COUNT; i++) {
            final String id = "SYSTEM-" + i;
            sbiSystems.add(new SbiSystem(id));
        }
        activeSystems.addAll(sbiSystems);

        // start systems
        for (SbiSystem sbiSystem: sbiSystems) {
            sbiSystem.start();
        }

        // poll systems
        logSeparator();
        while (!activeSystems.isEmpty()) {
            Uninterruptibles.sleepUninterruptibly(10, TimeUnit.SECONDS);

            for (SbiSystem sbiSystem : sbiSystems) {
                SbiSystemState state = sbiSystem.poll();
                log(sbiSystem.name, state.toString());

                if (!activeSystems.contains(sbiSystem)) {
                    // do nothing

                } else if (state.isSyncFailed()) {
                    sbiSystem.stop();
                    activeSystems.remove(sbiSystem);
                    log(sbiSystem.name, "sync failed, removing from active systems");

                } else if (state.isSyncComplete()) {
                    sbiSystem.stop();
                    activeSystems.remove(sbiSystem);
                    log(sbiSystem.name, "sync completed, removing from active systems");
                }
            }

            logSeparator();
        }

        // stop systems
        for (SbiSystem sbiSystem: sbiSystems) {
            sbiSystem.stop();
        }
    }

    private static class SbiSystem implements SystemListener {

        private final String name;
        private final System system;
        private final ExecutorService executor;

        private long eventTimestamp;
        private WalletManagerState currentState;
        private float syncProgress;
        private long syncProgressTimestamp;
        private boolean syncError;
        private boolean syncStarted;
        private boolean syncStopped;
        private WalletManagerSyncStoppedReason syncStoppedReason;

        SbiSystem(String name) throws IOException {
            this.name = name;
            this.executor = Executors.newSingleThreadExecutor();

            File storageDir = Files.createTempDir();
            deleteRecursively(storageDir);
            storageDir.mkdirs();

            String storagePath = storageDir.getAbsolutePath();
            Account account = createDefaultAccount();
            BlockchainDb query = createDefaultBlockchainDbWithToken();

            ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
            system = System.create(executor, this, account, MAINNET, storagePath, query);
        }

        void start() {
            system.configure(Collections.emptyList());
        }

        void stop() {
            system.disconnectAll();
        }

        synchronized SbiSystemState poll() {
            return new SbiSystemState(
                    eventTimestamp,
                    syncProgressTimestamp,
                    syncError,
                    syncStarted,
                    syncStopped,
                    syncStoppedReason,
                    syncProgress,
                    currentState
            );
        }

        synchronized void updateForStateChange(WalletManagerState newState) {
            currentState = newState;
        }

        synchronized void updateForSyncStart() {
            // mark an error if we have started more than once (this test scenario only tests a single sync)
            syncError |= syncStarted;

            syncStarted = true;
        }

        synchronized void updateForSyncStop(WalletManagerSyncStoppedReason reason) {
            // mark an error if we have stopped more than once (this test scenario only tests a single sync)
            syncError |= syncStopped;

            syncStopped = true;
            syncStoppedReason = reason;
        }

        synchronized void updateForProgress(float newProgress) {
            if (newProgress != syncProgress) {
                syncProgress = newProgress;
                syncProgressTimestamp = java.lang.System.currentTimeMillis();
            }
        }

        synchronized void updateEventTimestamp() {
            eventTimestamp = java.lang.System.currentTimeMillis();
        }

        void runOnAnotherThread(Runnable runnable) {
            executor.submit(runnable);
        }

        @Override
        public void handleSystemEvent(System system, SystemEvent event) {
            // don't call updateEventTimestamp(); only interested in those associated with the wallet manager

            // create wallet manager for BTC network using P2P mode
            if (event instanceof SystemNetworkAddedEvent) {
                SystemNetworkAddedEvent e = (SystemNetworkAddedEvent) event;
                if (e.getNetwork().getCurrency().getCode().equals(Currency.CODE_AS_BTC)) {
                    system.createWalletManager(e.getNetwork(), WalletManagerMode.P2P_ONLY, AddressScheme.BTC_LEGACY, Collections.emptySet());
                }
            }
        }

        @Override
        public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
            updateEventTimestamp();

            // connect wallet manager when wallet is added (this should be on WalletManagerCreatedEvent)
            if (event instanceof WalletManagerWalletAddedEvent) {
                runOnAnotherThread(() -> manager.connect(null));
            }

            // track all state changes of the wallet manager
            if (event instanceof WalletManagerChangedEvent) {
                WalletManagerChangedEvent e = (WalletManagerChangedEvent) event;
                updateForStateChange(e.getNewState());
            }

            // track all sync start events
            if (event instanceof WalletManagerSyncStartedEvent) {
                updateForSyncStart();
            }

            // track all sync stop events
            if (event instanceof WalletManagerSyncStoppedEvent) {
                WalletManagerSyncStoppedEvent e = (WalletManagerSyncStoppedEvent) event;
                updateForSyncStop(e.getReason());
            }

            // track all sync progress events
            if (event instanceof WalletManagerSyncProgressEvent) {
                WalletManagerSyncProgressEvent e = (WalletManagerSyncProgressEvent) event;
                updateForProgress(e.getPercentComplete());
            }
        }

        @Override
        public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
            updateEventTimestamp();
        }

        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            updateEventTimestamp();
        }

        @Override
        public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
            // don't call updateEventTimestamp(); only interested in those associated with the wallet manager
        }
    }

    // System State

    private static class SbiSystemState {

        private final long eventTimestamp;
        private final long progressTimestamp;

        private final boolean syncError;
        private final boolean syncStarted;
        private final boolean syncStopped;
        private final WalletManagerSyncStoppedReason syncStoppedReason;
        private final float syncProgress;
        private final WalletManagerState currentState;

        SbiSystemState(long eventTimestamp,
                       long progressTimestamp,
                       boolean syncError,
                       boolean syncStarted,
                       boolean syncStopped,
                       WalletManagerSyncStoppedReason syncStoppedReason,
                       float syncProgress,
                       WalletManagerState currentState) {
            this.eventTimestamp = eventTimestamp;
            this.progressTimestamp = progressTimestamp;
            this.syncError = syncError;
            this.syncStarted = syncStarted;
            this.syncStopped = syncStopped;
            this.syncStoppedReason = syncStoppedReason;
            this.syncProgress = syncProgress;
            this.currentState = currentState;
        }

        boolean isSyncStalled(long eventWindowMs) {
            boolean eventInWindow = eventTimestamp > (java.lang.System.currentTimeMillis() - eventWindowMs);
            boolean progressInWindow = progressTimestamp > (java.lang.System.currentTimeMillis() - eventWindowMs);

            boolean syncStalled = (0 != eventTimestamp && !eventInWindow) ||
                    (0 != progressTimestamp) && !progressInWindow;

            return !syncError && syncStarted && !syncStopped && syncStalled;
        }

        boolean isSyncFailed() {
            return syncError;
        }

        boolean isSyncPending() {
            return !syncError && !syncStarted && !syncStopped;
        }

        boolean isSyncProgressing() {
            return !syncError && syncStarted && !syncStopped;
        }

        boolean isSyncComplete() {
            return !syncError && syncStarted && syncStopped;
        }

        @Override
        public String toString() {
            StringBuilder descBuilder = new StringBuilder(
                    (isSyncFailed() ? "Failed" :
                            (isSyncPending() ? "Pending" :
                                    (isSyncProgressing() ? "Progressing" :
                                            (isSyncComplete() ? "Complete" : "Fatal"))))
            );
            if (isSyncProgressing()) {
                descBuilder.append(" (");
                descBuilder.append(syncProgress);
                if (isSyncStalled(EVENT_WINDOW_MS)) descBuilder.append(" STALLED");
                descBuilder.append(')');
            }
            if (isSyncComplete() || isSyncFailed()) {
                descBuilder.append(" (");
                descBuilder.append(syncStoppedReason.toString());
                descBuilder.append(')');
            }
            String desc = descBuilder.toString();

            SimpleDateFormat eventFormatter = new SimpleDateFormat("HH:mm:ss");
            String eventTimestamp = eventFormatter.format(new Date(this.eventTimestamp));
            String progressTimestamp = eventFormatter.format(new Date(this.progressTimestamp));
            return String.format(Locale.ROOT,
                    "desc = %-40s lastState = %-32s lastEventTs = %-16s lastProgressTs = %-16s",
                    desc, currentState, eventTimestamp, progressTimestamp);
        }
    }

    // Accounts

    private static final byte[] DEFAULT_ACCOUNT_KEY = "ginger settle marine tissue robot crane night number ramp coast roast critic"
            .getBytes(StandardCharsets.UTF_8);

    private static final Date DEFAULT_ACCOUNT_TIMESTAMP = new Date(TimeUnit.SECONDS.toMillis(1507328506));

    private static final String DEFAULT_ACCOUNT_UIDS = "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96";

    /* package */
    static Account createDefaultAccount() {
        return Account.createFromPhrase(DEFAULT_ACCOUNT_KEY, DEFAULT_ACCOUNT_TIMESTAMP, DEFAULT_ACCOUNT_UIDS);
    }

    // BlockchainDB

    /* package */
    static final OkHttpClient DEFAULT_HTTP_CLIENT = new OkHttpClient();

    private static String DEFAULT_TOKEN = "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9." +
            "eyJzdWIiOiJjNzQ5NTA2ZS02MWUzLTRjM2UtYWNiNS00OTY5NTM2ZmRhMTAiLCJpYXQiOjE1N" +
            "zI1NDY1MDAuODg3LCJleHAiOjE4ODAxMzA1MDAuODg3LCJicmQ6Y3QiOiJ1c3IiLCJicmQ6Y2" +
            "xpIjoiZGViNjNlMjgtMDM0NS00OGY2LTlkMTctY2U4MGNiZDYxN2NiIn0." +
            "460_GdAWbONxqOhWL5TEbQ7uEZi3fSNrl0E_Zg7MAg570CVcgO7rSMJvAPwaQtvIx1XFK_QZjcoNULmB8EtOdg";

    /* package */
    private static BlockchainDb createDefaultBlockchainDbWithToken() {
        return BlockchainDb.createForTest(DEFAULT_HTTP_CLIENT, DEFAULT_TOKEN);
    }

    // Helpers


    /* package */
    private static boolean deleteRecursively(File file) {
        if (file.isDirectory()) {
            File[] fileList = file.listFiles();
            if (null != fileList) {
                for (File child : fileList) {
                    if (!deleteRecursively(child)) {
                        return false;
                    }
                }
            }
        }
        return file.delete();
    }


    // Logging

    private static void logSeparator() {
        log(SEPARATOR);
    }

    private static void log(String prefix, String message) {
        log(String.format(Locale.ROOT, "%-12s -> %s", prefix, message));
    }

    private static void log(String message) {
        java.lang.System.out.println("SBI:" + message);
//        android.util.Log.d("SBI", message);
    }
}
