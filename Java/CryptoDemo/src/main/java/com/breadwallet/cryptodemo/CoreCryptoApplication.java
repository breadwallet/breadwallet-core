package com.breadwallet.cryptodemo;

import android.app.Application;
import android.arch.lifecycle.Lifecycle;
import android.arch.lifecycle.LifecycleObserver;
import android.arch.lifecycle.OnLifecycleEvent;
import android.arch.lifecycle.ProcessLifecycleOwner;
import android.os.StrictMode;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.SystemImpl;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.System;
import com.google.common.base.Optional;

import java.io.File;
import java.util.Arrays;
import java.util.concurrent.Executors;

import okhttp3.OkHttpClient;

import static com.google.common.base.Preconditions.checkState;

public class CoreCryptoApplication extends Application {

    // TODO: Make these build dependent
    private static final String BDB_BASE_URL = "https://test-blockchaindb-api.brd.tools";
    private static final String API_BASE_URL = "https://stage2.breadwallet.com";

    public static System system;
    public static CoreSystemListener listener;

    private LifecycleObserver observer = new LifecycleObserver() {
        @OnLifecycleEvent(Lifecycle.Event.ON_START)
        void onEnterForeground() {
            system.start();
        }

        @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
        void onEnterBackground() {
            system.stop();
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();

        StrictMode.enableDefaults();

        String paperKey = "boring head harsh green empty clip fatal typical found crane dinner timber";

        File storageFile = new File (getFilesDir(), "core");
        if (storageFile.exists()) deleteRecursively(storageFile);
        checkState(storageFile.mkdirs());

        Optional<Account> optAccount = Account.createFrom(paperKey, "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e97");
        checkState(optAccount.isPresent());
        Account account = optAccount.get();
        account.setTimestamp(1514764800);

        BlockchainDb query = new BlockchainDb(new OkHttpClient(), BDB_BASE_URL, API_BASE_URL);
        listener = new CoreSystemListener();
        system = SystemImpl.create(Executors.newSingleThreadExecutor(), listener, account , storageFile.getAbsolutePath(), query);
        system.initialize(Arrays.asList("bitcoin-mainnet", "ethereum-mainnet"));

        ProcessLifecycleOwner.get().getLifecycle().addObserver(observer);
    }

    private void deleteRecursively (File file) {
        if (file.isDirectory())
            for (File child : file.listFiles()) {
                deleteRecursively(child);
            }
        file.delete();
    }
}
