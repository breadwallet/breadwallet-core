/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

public class BRCryptoWalletManagerEvent extends Structure {

	public int type;
	public u_union u;

	public static class u_union extends Union {

		public state_struct state;
		public wallet_struct wallet;
		public sync_struct sync;
		public blockHeight_struct blockHeight;

		public static class state_struct extends Structure {

			public int oldValue;
			public int newValue;

			public state_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("oldValue", "newValue");
			}

			public state_struct(int oldValue, int newValue) {
				super();
				this.oldValue = oldValue;
				this.newValue = newValue;
			}

			public state_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends state_struct implements Structure.ByReference {
				
			}

			public static class ByValue extends state_struct implements Structure.ByValue {
				
			}
		}

		public static class wallet_struct extends Structure {

			public BRCryptoWallet value;

			public wallet_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("value");
			}

			public wallet_struct(BRCryptoWallet value) {
				super();
				this.value = value;
			}

			public wallet_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends wallet_struct implements Structure.ByReference {
				
			}

			public static class ByValue extends wallet_struct implements Structure.ByValue {
				
			}
		}

		public static class sync_struct extends Structure {

			public int timestamp;
			public float percentComplete;
			public sync_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("timestamp", "percentComplete");
			}

			public sync_struct(int timestamp, float percentComplete) {
				super();
				this.timestamp = timestamp;
				this.percentComplete = percentComplete;
			}

			public sync_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends sync_struct implements Structure.ByReference {
				
			}

			public static class ByValue extends sync_struct implements Structure.ByValue {
				
			}
		}

		public static class blockHeight_struct extends Structure {

			public long value;

			public blockHeight_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("value");
			}

			public blockHeight_struct(long value) {
				super();
				this.value = value;
			}

			public blockHeight_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends blockHeight_struct implements Structure.ByReference {
				
			}

			public static class ByValue extends blockHeight_struct implements Structure.ByValue {
				
			}
		}

		public u_union() {
			super();
		}

		public u_union(state_struct state) {
			super();
			this.state = state;
			setType(state_struct.class);
		}

		public u_union(wallet_struct wallet) {
			super();
			this.wallet = wallet;
			setType(wallet_struct.class);
		}

		public u_union(sync_struct sync) {
			super();
			this.sync = sync;
			setType(sync_struct.class);
		}

		public u_union(blockHeight_struct blockHeight) {
			super();
			this.blockHeight = blockHeight;
			setType(blockHeight_struct.class);
		}

		public u_union(Pointer peer) {
			super(peer);
		}

		public static class ByReference extends u_union implements Structure.ByReference {
			
		}

		public static class ByValue extends u_union implements Structure.ByValue {
			
		}
	}

	public BRCryptoWalletManagerEvent() {
		super();
	}

	protected List<String> getFieldOrder() {
		return Arrays.asList("type", "u");
	}

	public BRCryptoWalletManagerEvent(int type, u_union u) {
		super();
		this.type = type;
		this.u = u;
	}

	public BRCryptoWalletManagerEvent(Pointer peer) {
		super(peer);
	}

	@Override
	public void read() {
		super.read();
		switch (type){
			case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
				u.setType(u_union.blockHeight_struct.class);
				u.read();
				break;
			case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
				u.setType(u_union.state_struct.class);
				u.read();
				break;
			case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
				u.setType(u_union.sync_struct.class);
				u.read();
				break;
			case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
			case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
			case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
				u.setType(u_union.wallet_struct.class);
				u.read();
				break;
		}
	}

	public static class ByReference extends BRCryptoWalletManagerEvent implements Structure.ByReference {
		
	}

	public static class ByValue extends BRCryptoWalletManagerEvent implements Structure.ByValue {
		
	}
}
