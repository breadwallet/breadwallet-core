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

public class BRCryptoWalletEstimateFeeBasisResult extends Structure {

	public int success;
	public u_union u;

	public static class u_union extends Union {

		public success_struct success;

		public static class success_struct extends Structure {

			public BRCryptoFeeBasis feeBasis;

			public success_struct() {
				super();
			}

			protected List<String> getFieldOrder() {
				return Arrays.asList("feeBasis");
			}

			public success_struct(BRCryptoFeeBasis feeBasis) {
				super();
				this.feeBasis = feeBasis;
			}

			public success_struct(Pointer peer) {
				super(peer);
			}

			public static class ByReference extends success_struct implements Structure.ByReference {

			}
			public static class ByValue extends success_struct implements Structure.ByValue {

			}
		}

		public u_union() {
			super();
		}

		public u_union(success_struct success) {
			super();
			this.success = success;
			setType(success_struct.class);
		}

		public u_union(Pointer peer) {
			super(peer);
		}

		public static class ByReference extends u_union implements Structure.ByReference {

		}

		public static class ByValue extends u_union implements Structure.ByValue {

		}
	}

	public BRCryptoWalletEstimateFeeBasisResult() {
		super();
	}

	protected List<String> getFieldOrder() {
		return Arrays.asList("success", "u");
	}

	public BRCryptoWalletEstimateFeeBasisResult(int success, u_union u) {
		super();
		this.success = success;
		this.u = u;
	}

	public BRCryptoWalletEstimateFeeBasisResult(Pointer peer) {
		super(peer);
	}

	@Override
	public void read() {
		super.read();
		switch (success){
			case BRCryptoBoolean.CRYPTO_TRUE:
				u.setType(u_union.success_struct.class);
				u.read();
				break;
		}
	}

	public static class ByReference extends BRCryptoWalletEstimateFeeBasisResult implements Structure.ByReference {
		
	}

	public static class ByValue extends BRCryptoWalletEstimateFeeBasisResult implements Structure.ByValue {
		
	}
}
