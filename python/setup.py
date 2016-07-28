import os
from setuptools import setup, Extension

def here(*args):
    return os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), *args))

def fromroot(*args):
    return here(os.pardir, *args)

breadwallet = Extension(
    'breadwallet',
    sources=[
        fromroot('BRAddress.c'),
        fromroot('BRBase58.c'),
        fromroot('BRBIP32Sequence.c'),
        fromroot('BRBIP38Key.c'),
        fromroot('BRBIP39Mnemonic.c'),
        fromroot('BRBloomFilter.c'),
        fromroot('BRCrypto.c'),
        fromroot('BRKey.c'),
        fromroot('BRMerkleBlock.c'),
        fromroot('BRPaymentProtocol.c'),
        fromroot('BRPeer.c'),
        fromroot('BRPeerManager.c'),
        fromroot('BRSet.c'),
        fromroot('BRTransaction.c'),
        fromroot('BRWallet.c'),
        'bindings.c'
    ],
    include_dirs=[fromroot(), fromroot('secp256k1')]
)

setup(
    name='breadwallet-core',
    version='0.1',
    description='A simple, easy to use SPV wallet.',
    ext_modules=[breadwallet],
    test_suite='nose.collector',
    tests_require=['nose']
)
