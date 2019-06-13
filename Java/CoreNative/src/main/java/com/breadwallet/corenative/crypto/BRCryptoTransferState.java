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

public class BRCryptoTransferState extends Structure {

	public int type;
	public u_union u;

	public static class u_union extends Union {

		public included_struct included;
		public errorred_struct errorred;

		public static class included_struct extends Structure {

			public long blockNumber;
			public long transactionIndex;
			public long timestamp;
			public BRCryptoAmount fee;

			public included_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("blockNumber", "transactionIndex", "timestamp", "fee");
			}

			public included_struct(long blockNumber, long transactionIndex, long timestamp, BRCryptoAmount fee) {
				super();
				this.blockNumber = blockNumber;
				this.transactionIndex = transactionIndex;
				this.timestamp = timestamp;
				this.fee = fee;
			}

			public included_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends included_struct implements Structure.ByReference {

			}
			public static class ByValue extends included_struct implements Structure.ByValue {

			}
		}

		public static class errorred_struct extends Structure {

			public byte[] message = new byte[128 + 1];

			public errorred_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("message");
			}

			public errorred_struct(byte[] message) {
				super();
				if ((message.length != this.message.length)) {
					throw new IllegalArgumentException("Wrong array size!");
				}
				this.message = message;
			}

			public errorred_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends errorred_struct implements Structure.ByReference {

			}

			public static class ByValue extends errorred_struct implements Structure.ByValue {

			}
		}

		public u_union() {
			super();
		}

		public u_union(included_struct state) {
			super();
			this.included = state;
			setType(included_struct.class);
		}

		public u_union(errorred_struct confirmation) {
			super();
			this.errorred = confirmation;
			setType(errorred_struct.class);
		}

		public u_union(Pointer peer) {
			super(peer);
		}

		public static class ByReference extends u_union implements Structure.ByReference {

		}

		public static class ByValue extends u_union implements Structure.ByValue {

		}
	}

	public BRCryptoTransferState() {
		super();
	}

	protected List<String> getFieldOrder() {
		return Arrays.asList("type", "u");
	}

	public BRCryptoTransferState(int type, u_union u) {
		super();
		this.type = type;
		this.u = u;
	}

	public BRCryptoTransferState(Pointer peer) {
		super(peer);
	}

	@Override
	public void read() {
		super.read();
		switch (type){
			case BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_INCLUDED:
				u.setType(u_union.included_struct.class);
				u.read();
				break;
			case BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_ERRORRED:
				u.setType(u_union.errorred_struct.class);
				u.read();
				break;
		}
	}

	public static class ByReference extends BRCryptoTransferState implements Structure.ByReference {
		
	}

	public static class ByValue extends BRCryptoTransferState implements Structure.ByValue {
		
	}
}
