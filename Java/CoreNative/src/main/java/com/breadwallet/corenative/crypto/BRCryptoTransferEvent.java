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

public class BRCryptoTransferEvent extends Structure {

	public int type;
	public u_union u;

	public static class u_union extends Union {

		public state_struct state;
		public confirmation_struct confirmation;

		public static class state_struct extends Structure {

			public int oldState;
			public int newState;

			public state_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("oldState", "newState");
			}

			public state_struct(int oldState, int newState) {
				super();
				this.oldState = oldState;
				this.newState = newState;
			}

			public state_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends state_struct implements Structure.ByReference {
				
			}
			public static class ByValue extends state_struct implements Structure.ByValue {
				
			}
		}

		public static class confirmation_struct extends Structure {

			public long count;
			public confirmation_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("count");
			}

			public confirmation_struct(long count) {
				super();
				this.count = count;
			}

			public confirmation_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends confirmation_struct implements Structure.ByReference {
				
			}

			public static class ByValue extends confirmation_struct implements Structure.ByValue {
				
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

		public u_union(confirmation_struct confirmation) {
			super();
			this.confirmation = confirmation;
			setType(confirmation_struct.class);
		}

		public u_union(Pointer peer) {
			super(peer);
		}

		public static class ByReference extends u_union implements Structure.ByReference {
			
		}

		public static class ByValue extends u_union implements Structure.ByValue {
			
		}
	}

	public BRCryptoTransferEvent() {
		super();
	}

	protected List<String> getFieldOrder() {
		return Arrays.asList("type", "u");
	}

	public BRCryptoTransferEvent(int type, u_union u) {
		super();
		this.type = type;
		this.u = u;
	}

	public BRCryptoTransferEvent(Pointer peer) {
		super(peer);
	}

	@Override
	public void read() {
		super.read();
		switch (type){
			case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CHANGED:
				u.setType(u_union.state_struct.class);
				u.read();
				break;
			case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CONFIRMED:
				u.setType(u_union.confirmation_struct.class);
				u.read();
				break;
		}
	}

	public static class ByReference extends BRCryptoTransferEvent implements Structure.ByReference {
		
	}

	public static class ByValue extends BRCryptoTransferEvent implements Structure.ByValue {
		
	}
}
