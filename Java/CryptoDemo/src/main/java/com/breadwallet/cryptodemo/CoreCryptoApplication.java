package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.app.Application;
import android.arch.lifecycle.Lifecycle;
import android.arch.lifecycle.LifecycleObserver;
import android.arch.lifecycle.OnLifecycleEvent;
import android.arch.lifecycle.ProcessLifecycleOwner;
import android.content.Intent;
import android.os.StrictMode;
import android.util.Log;

import com.breadwallet.corecrypto.CryptoApiProvider;
import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.CryptoApi;
import com.breadwallet.crypto.DispatchingSystemListener;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.System;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import okhttp3.OkHttpClient;

import static com.google.common.base.Preconditions.checkState;

public class CoreCryptoApplication extends Application {

    private static final String TAG = CoreCryptoApplication.class.getName();

    private static final String BDB_BASE_URL = BuildConfig.BDB_BASE_URL;
    private static final String API_BASE_URL = BuildConfig.API_BASE_URL;

    private static final String EXTRA_WIPE = "WIPE";
    private static final String EXTRA_TIMESTAMP = "TIMESTAMP";
    private static final String EXTRA_PAPER_KEY = "PAPER_KEY";
    private static final String EXTRA_MODE = "MODE";
    private static final String EXTRA_IS_MAINNET = "IS_MAINNET";

    private static final boolean DEFAULT_IS_MAINNET = true;
    private static final boolean DEFAULT_WIPE = true;
    private static final long DEFAULT_TIMESTAMP = 0;
    private static final String DEFAULT_PAPER_KEY = "boring head harsh green empty clip fatal typical found crane dinner timber";
    private static final WalletManagerMode DEFAULT_MODE = WalletManagerMode.API_ONLY;

    private static System system;
    private static DispatchingSystemListener systemListener;
    private static byte[] paperKey;

    private static AtomicBoolean runOnce = new AtomicBoolean(false);

    private static LifecycleObserver observer = new LifecycleObserver() {
        @OnLifecycleEvent(Lifecycle.Event.ON_START)
        void onEnterForeground() {
            for (WalletManager manager: system.getWalletManagers()) {
                manager.connect();
            }
        }

        @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
        void onEnterBackground() {
            system.stop();
        }
    };

    public static void initialize(Activity startingActivity) {
        if (!runOnce.getAndSet(true)) {
            Intent intent = startingActivity.getIntent();

            String paperKeyString = (intent.hasExtra(EXTRA_PAPER_KEY) ? intent.getStringExtra(EXTRA_PAPER_KEY) : DEFAULT_PAPER_KEY);
            paperKey = paperKeyString.getBytes(StandardCharsets.UTF_8);

            boolean isMainnet = intent.getBooleanExtra(EXTRA_IS_MAINNET, DEFAULT_IS_MAINNET);

            boolean wipe = intent.getBooleanExtra(EXTRA_WIPE, DEFAULT_WIPE);
            long timestamp = intent.getLongExtra(EXTRA_TIMESTAMP, DEFAULT_TIMESTAMP);
            WalletManagerMode mode = intent.hasExtra(EXTRA_MODE) ? WalletManagerMode.valueOf(intent.getStringExtra(EXTRA_MODE)) : DEFAULT_MODE;

            File storageFile = new File(startingActivity.getFilesDir(), "core");
            if (wipe) {
                if (storageFile.exists()) deleteRecursively(storageFile);
                checkState(storageFile.mkdirs());
            }

            Log.d(TAG, String.format("Account PaperKey: %s", paperKeyString));
            Log.d(TAG, String.format("Account Timestamp: %s", timestamp));
            Log.d(TAG, String.format("StoragePath:       %s", storageFile.getAbsolutePath()));
            Log.d(TAG, String.format("Mainnet:           %s", isMainnet));

            CryptoApi.initialize(CryptoApiProvider.getInstance());

            List<String> currencyCodesNeeded = Arrays.asList("btc", "eth", "brd");
            systemListener = new DispatchingSystemListener();
            systemListener.addSystemListener(new CoreSystemListener(mode, isMainnet, currencyCodesNeeded));

            String uids = UUID.nameUUIDFromBytes(paperKey).toString();
            Account account = Account.createFromPhrase(paperKey, new Date(TimeUnit.SECONDS.toMillis(timestamp)), uids);

            BlockchainDb query = new BlockchainDb(new OkHttpClient(), BDB_BASE_URL, API_BASE_URL);
            system = System.create(Executors.newSingleThreadScheduledExecutor(), systemListener, account, isMainnet, storageFile.getAbsolutePath(), query);
            system.configure();

            ProcessLifecycleOwner.get().getLifecycle().addObserver(observer);
        }
    }

    public static System getSystem() {
        return system;
    }

    public static DispatchingSystemListener getDispatchingSystemListener() {
        return systemListener;
    }

    public static byte[] getPaperKey() {
        return paperKey;
    }

    private static void deleteRecursively (File file) {
        if (file.isDirectory()) {
            for (File child : file.listFiles()) {
                deleteRecursively(child);
            }
        }
        file.delete();
    }

    @Override
    public void onCreate() {
        super.onCreate();

        StrictMode.enableDefaults();
    }
}
