package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.util.SortedList;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.support.v7.widget.util.SortedListAdapterCallback;
import android.text.Html;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.events.system.DefaultSystemListener;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.transfer.DefaultTransferEventVisitor;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.google.common.base.Optional;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import javax.annotation.Nullable;

public class TransferListActivity extends AppCompatActivity {

    private static final DateFormat DATE_FORMAT = DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo.TransferListActivity.EXTRA_WALLET_NAME";

    private static final Comparator<Transfer> OLDEST_FIRST_COMPARATOR = (o1, o2) -> {
        Optional<TransferConfirmation> oc1 = o1.getConfirmation();
        Optional<TransferConfirmation> oc2 = o2.getConfirmation();

        if (oc1.isPresent() && oc2.isPresent()) {
            TransferConfirmation c1 = oc1.get();
            TransferConfirmation c2 = oc2.get();

            int blockCompare = c1.getBlockNumber().compareTo(c2.getBlockNumber());
            int indexCompare = c1.getTransactionIndex().compareTo(c2.getTransactionIndex());
            return (blockCompare != 0 ? blockCompare : indexCompare);

        } else if (oc1.isPresent()) {
            return -1;
        } else if (oc2.isPresent()) {
            return 1;
        } else {
            return o1.hashCode() - o2.hashCode();
        }
    };

    private static final Comparator<Transfer> NEWEST_FIRST_COMPARATOR = Collections.reverseOrder(OLDEST_FIRST_COMPARATOR);

    private static final Comparator<Transfer> DEFAULT_COMPARATOR = NEWEST_FIRST_COMPARATOR;

    public static void start(Activity callerActivity, Wallet wallet) {
        Intent intent = new Intent(callerActivity, TransferListActivity.class);
        intent.putExtra(EXTRA_WALLET_NAME, wallet.getName());
        callerActivity.startActivity(intent);
    }

    @Nullable
    private static Wallet getWallet(Intent intent) {
        String walletName = intent.getStringExtra(EXTRA_WALLET_NAME);
        for(Wallet wallet: CoreCryptoApplication.getSystem().getWallets()) {
            if (wallet.getName().equals(walletName)) {
                return wallet;
            }
        }
        return null;
    }

    private static WalletManagerMode getNextMode(WalletManager wm) {
        Network net = wm.getNetwork();
        System sys = wm.getSystem();

        List<WalletManagerMode> modes = sys.getSupportedWalletManagerModes(net);
        WalletManagerMode mode = wm.getMode();
        mode = modes.get((modes.indexOf(mode) + 1) % modes.size());
        return mode;
    }

    private Wallet wallet;

    private Adapter transferAdapter;

    private ClipboardManager clipboardManager;

    private final SystemListener walletListener = new DefaultSystemListener() {
        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            runOnUiThread(() -> {
                event.accept(new DefaultTransferEventVisitor<Void>() {
                    @Override
                    public Void visit(TransferCreatedEvent event) {
                        transferAdapter.add(transfer);
                        return null;
                    }

                    @Override
                    public Void visit(TransferChangedEvent event) {
                        transferAdapter.changed(transfer);
                        return null;
                    }


                    @Override
                    public Void visit(TransferDeletedEvent event) {
                        transferAdapter.remove(transfer);
                        return null;
                    }
                });
            });
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_list);

        CoreCryptoApplication.initialize(this);

        wallet = getWallet(getIntent());
        if (null == wallet) {
            finish();
            return;
        }

        clipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);

        Button sendView = findViewById(R.id.send_view);
        sendView.setOnClickListener(v -> TransferCreateSendActivity.start(TransferListActivity.this, wallet));

        Button recvView = findViewById(R.id.receive_view);
        recvView.setOnClickListener(v -> copyReceiveAddress());

        Button sweepView = findViewById(R.id.sweep_view);
        sweepView .setOnClickListener(v -> TransferCreateSweepActivity.start(TransferListActivity.this, wallet));

        RecyclerView transfersView = findViewById(R.id.transfer_recycler_view);
        transfersView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        RecyclerView.LayoutManager transferLayoutManager = new LinearLayoutManager(this);
        transfersView.setLayoutManager(transferLayoutManager);

        transferAdapter = new Adapter(DEFAULT_COMPARATOR, (transfer) -> TransferDetailsActivity.start(this, wallet, transfer));
        transfersView.setAdapter(transferAdapter);

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        toolbar.setTitle(String.format("Wallet: %s", wallet.getName()));
        setSupportActionBar(toolbar);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getDispatchingSystemListener().addWalletListener(wallet, walletListener);

        transferAdapter.set(new ArrayList<>(wallet.getTransfers()));
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getDispatchingSystemListener().removeWalletListener(wallet, walletListener);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_transfer_list, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        WalletManager wm = wallet.getWalletManager();
        menu.findItem(R.id.action_toggle_mode).setTitle("Switch to " + getNextMode(wm).name());

        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_connect:
                wallet.getWalletManager().connect();
                return true;
            case R.id.action_sync:
                wallet.getWalletManager().sync();
                return true;
            case R.id.action_disconnect:
                wallet.getWalletManager().disconnect();
                return true;
            case R.id.action_toggle_mode:
                WalletManager wm = wallet.getWalletManager();
                wm.setMode(getNextMode(wm));
                return true;
        }
        return false;
    }

    private void copyReceiveAddress() {
        String value = wallet.getTarget().toString();
        clipboardManager.setPrimaryClip(ClipData.newPlainText("ReceiveAddress", value));

        String escapedValue = Html.escapeHtml(value);
        Spanned message = Html.fromHtml(String.format("Copied receive address <b>%s</b> to clipboard", escapedValue));

        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private final OnItemClickListener<Transfer> listener;
        private final SortedList<Transfer> transfers;

        Adapter(Comparator<Transfer> comparator, OnItemClickListener<Transfer> listener) {
            this.listener = listener;
            this.transfers = new SortedList<>(Transfer.class, new SortedListAdapterCallback<Transfer>(this) {
                @Override
                public int compare(Transfer t1, Transfer t2) {
                    return comparator.compare(t1, t2);
                }

                @Override
                public boolean areContentsTheSame(Transfer t1, Transfer t2) {
                    return false;
                }

                @Override
                public boolean areItemsTheSame(Transfer t1, Transfer t2) {
                    return t1.equals(t2);
                }
            });
        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
            View v = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.layout_transfer_item, viewGroup, false);
            return new ViewHolder(v);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder vh, int i) {
            Transfer transfer = transfers.get(i);

            String dateText = transfer.getConfirmation()
                    .transform((c) -> DATE_FORMAT.format(c.getConfirmationTime())).or("<pending>");

            String addressText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
            addressText = String.format("Hash: %s", addressText);

            String amountText = transfer.getAmountDirected().toString();
            String feeText = String.format("Fee: %s", transfer.getFee());

            String stateText = String.format("State: %s", transfer.getState());

            vh.itemView.setOnClickListener(v -> listener.onItemClick(transfer));
            vh.dateView.setText(dateText);
            vh.amountView.setText(amountText);
            vh.addressView.setText(addressText);
            vh.feeView.setText(feeText);
            vh.stateView.setText(stateText);
        }

        @Override
        public int getItemCount() {
            return transfers.size();
        }

        private void set(List<Transfer> newTransfers) {
            transfers.replaceAll(newTransfers);
        }

        private void add(Transfer transfer) {
            transfers.add(transfer);
        }

        private void remove(Transfer transfer) {
            transfers.remove(transfer);
        }

        private void changed(Transfer transfer) {
            int index = transfers.indexOf(transfer);
            if (index != -1) {
                transfers.updateItemAt(index, transfer);
            }
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        private TextView dateView;
        private TextView amountView;
        private TextView addressView;
        private TextView feeView;
        private TextView stateView;

        private ViewHolder(@NonNull View view) {
            super(view);

            dateView = view.findViewById(R.id.item_date);
            amountView = view.findViewById(R.id.item_amount);
            addressView = view.findViewById(R.id.item_address);
            feeView = view.findViewById(R.id.item_fee);
            stateView = view.findViewById(R.id.item_state);
        }
    }
}
