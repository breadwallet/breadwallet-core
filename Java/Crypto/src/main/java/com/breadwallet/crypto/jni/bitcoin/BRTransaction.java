package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.SizeT;
import com.breadwallet.crypto.jni.support.UInt256;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;

public class BRTransaction extends Structure {

	public UInt256 txHash;
	public UInt256 wtxHash;
	public int version;
	public BRTxInput.ByReference inputs;
	public SizeT inCount;
	public BRTxOutput.ByReference outputs;
	public SizeT outCount;
	public int lockTime;
	public int blockHeight;
	public int timestamp;

	protected List<String> getFieldOrder() {
		return Arrays.asList("txHash", "wtxHash", "version", "inputs", "inCount", "outputs", "outCount", "lockTime", "blockHeight", "timestamp");
	}

	public BRTransaction() {
		super();
	}

	public BRTransaction(Pointer peer) {
		super(peer);
	}

	public static class ByReference extends BRTransaction implements Structure.ByReference {
		
	}

	public static class ByValue extends BRTransaction implements Structure.ByValue {
		
	}
}
