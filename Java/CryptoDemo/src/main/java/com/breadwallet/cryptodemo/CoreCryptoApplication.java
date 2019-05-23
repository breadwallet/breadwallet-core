package com.breadwallet.cryptodemo;

import android.app.Application;
import android.arch.lifecycle.Lifecycle;
import android.arch.lifecycle.LifecycleObserver;
import android.arch.lifecycle.OnLifecycleEvent;
import android.arch.lifecycle.ProcessLifecycleOwner;
import android.os.StrictMode;

public class CoreCryptoApplication extends Application {

    private SharedSystem system;
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

        system = SharedSystem.getInstance(getApplicationContext());
        ProcessLifecycleOwner.get().getLifecycle().addObserver(observer);
    }
}
