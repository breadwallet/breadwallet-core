package com.breadwallet.coredemo;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.breadwallet.core.ethereum.BREthereumNetwork;

import java.io.File;

public class WalletNavigationActivity extends AppCompatActivity {
    static { System.loadLibrary("core"); }

    static CoreDemoEthereumClient client = null;

    private void deleteRecursively (File file) {
        if (file.isDirectory())
            for (File child : file.listFiles()) {
                deleteRecursively(child);
            }
        file.delete();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        System.out.println ("Starting");

        File storageFile = new File (getFilesDir(), "core");
        if (storageFile.exists()) deleteRecursively(storageFile);
        storageFile.mkdirs();

        client = new CoreDemoEthereumClient(BREthereumNetwork.mainnet,
            storageFile.getAbsolutePath(),
            "boring head harsh green empty clip fatal typical found crane dinner timber");

        client.ewm.connect();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wallet_navigation);
    }
}
