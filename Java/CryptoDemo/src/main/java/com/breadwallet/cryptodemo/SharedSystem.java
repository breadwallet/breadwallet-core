package com.breadwallet.cryptodemo;

import android.arch.lifecycle.Lifecycle;
import android.arch.lifecycle.LifecycleObserver;
import android.arch.lifecycle.OnLifecycleEvent;
import android.content.Context;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.SystemImpl;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.google.common.base.Optional;

import java.io.File;
import java.util.Arrays;
import java.util.concurrent.Executors;

import okhttp3.OkHttpClient;

import static com.google.common.base.Preconditions.checkState;

public class SharedSystem {

    private static final String TAG = SharedSystem.class.getName();

    // TODO: Make these build dependent
    private static final String BDB_BASE_URL = "https://test-blockchaindb-api.brd.tools";
    private static final String API_BASE_URL = "https://stage2.breadwallet.com";

    private static volatile SharedSystem INSTANCE;

    public static SharedSystem getInstance(Context context) {
        if (INSTANCE == null) {
            synchronized (SharedSystem.class) {
                if (INSTANCE == null) {
                    INSTANCE = new SharedSystem(context);
                }
            }
        }
        return INSTANCE;
    }

    private final String paperKey;
    private final SharedSystemListener listener;
    private final System system;

    private SharedSystem(Context context) {
        paperKey = "boring head harsh green empty clip fatal typical found crane dinner timber";


        File storageFile = new File (context.getFilesDir(), "core");
        if (storageFile.exists()) deleteRecursively(storageFile);
        checkState(storageFile.mkdirs());

        Optional<Account> optAccount = Account.createFrom(paperKey, "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e97");
        checkState(optAccount.isPresent());
        Account account = optAccount.get();
        account.setTimestamp(1514764800);

        BlockchainDb query = new BlockchainDb(new OkHttpClient(), BDB_BASE_URL, API_BASE_URL);
        listener = new SharedSystemListener();
        system = SystemImpl.create(Executors.newSingleThreadExecutor(), listener, account , storageFile.getAbsolutePath(), query);
        system.initialize(Arrays.asList("bitcoin-mainnet", "ethereum-mainnet"));
    }

    public void start() {
        system.start();
    }

    public void stop() {
        system.stop();
    }

    public void sync() {
        system.sync();
    }

    public String getPaperKey() {
        return paperKey;
    }

    private void deleteRecursively (File file) {
        if (file.isDirectory())
            for (File child : file.listFiles()) {
                deleteRecursively(child);
            }
        file.delete();
    }
}
