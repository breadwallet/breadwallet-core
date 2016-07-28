#include <Python.h>
#include "structmember.h"
#include "BRInt.h"
#include "BRBIP32Sequence.h"
#include "BRBIP39Mnemonic.h"
#include "BRKey.h"
#include "BRTransaction.h"
#include "BRWallet.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ints
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
    PyObject_HEAD
    UInt512 ob_fval;
} b_UInt512;

static PyObject *b_UInt512New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    b_UInt512 *self = (b_UInt512 *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->ob_fval = UINT512_ZERO;
    }
    return (PyObject *)self;
}

static PyTypeObject b_UInt512Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "breadwallet.UInt12",      /* tp_name */
    sizeof(b_UInt512),         /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_as_async */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "UInt512 Object",          /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    b_UInt512New,              /* tp_new */
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Address
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
    PyObject_HEAD
    BRAddress ob_fval;
} b_Address;

static PyObject *b_AddressNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    b_Address *self = (b_Address *)type->tp_alloc(type, 0);
    if (self != NULL) {
    }
    return (PyObject *)self;
}

static PyObject *b_AddresToStr(PyObject *self) {
   return PyUnicode_FromString(((b_Address *)self)->ob_fval.s);
}

static PyMethodDef b_AddressMethods[] = {
  {NULL}
};

static PyTypeObject b_AddressType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "breadwallet.Address",     /* tp_name */
    sizeof(b_Address),         /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_as_async */
    (reprfunc)b_AddresToStr,   /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc)b_AddresToStr,   /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "Address Object",          /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    b_AddressMethods,          /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    b_AddressNew,              /* tp_new */
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Transaction
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
    PyObject_HEAD
    BRTransaction *ob_fval;
} b_Transaction;

static void b_TransactionDealloc(b_Transaction *self) {
  BRTransactionFree(self->ob_fval);
  self->ob_fval = NULL;
  Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *b_TransactionNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    b_Transaction *self = (b_Transaction *)type->tp_alloc(type, 0);
    if (self != NULL) {
      self->ob_fval = NULL;
    }
    return (PyObject *)self;
}

static PyObject *b_TransactionInit(PyObject *self, PyObject *args, PyObject *kwds) {
    b_Transaction *obj = (b_Transaction *)self;
    obj->ob_fval = BRTransactionNew();
    return 0;
}

static PyObject *b_TransactionToStr(PyObject *self) {
    return PyUnicode_FromString(u256_hex_encode(((b_Transaction *)self)->ob_fval->txHash));
}

static PyMethodDef b_TransactionMethods[] = {
    {NULL}
};

static PyTypeObject b_TransactionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "breadwallet.Transaction", /* tp_name */
    sizeof(b_Transaction),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)b_TransactionDealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_as_async */
    (reprfunc)b_TransactionToStr, /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc)b_TransactionToStr, /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "Transaction Object",      /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    b_TransactionMethods,      /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)b_TransactionInit, /* tp_init */
    0,                         /* tp_alloc */
    b_TransactionNew,          /* tp_new */
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Keys
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
    PyObject_HEAD
    BRMasterPubKey ob_fval;
} b_MasterPubKey;

static PyObject *b_MasterPubKeyNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    b_MasterPubKey *self = (b_MasterPubKey *)type->tp_alloc(type, 0);
    if (self != NULL) {
      self->ob_fval = BR_MASTER_PUBKEY_NONE;
    }
    return (PyObject *)self;
}

static PyObject *b_MasterPubKeyFromPhrase(PyObject *cls, PyObject *args, PyObject *kwds) {
    PyObject *result = NULL;
    char *phrase = "";
    // parse args
    static char *kwlist[] = { "phrase", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &phrase)) {
      return NULL;
    }
    // derive
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, phrase, NULL);
    // allocate
    result = PyObject_CallFunction(cls, "");
    // set value
    if (result != NULL) {
      ((b_MasterPubKey *)result)->ob_fval = BRBIP32MasterPubKey(&seed, sizeof(seed));
    }
    return result;
}

static PyMethodDef b_MasterPubKeyMethods[] = {
    /* Class Methods */
    {"from_phrase", (PyCFunction)b_MasterPubKeyFromPhrase, (METH_VARARGS | METH_KEYWORDS | METH_CLASS),
     "generate a MasterPubKey from a phrase"},
    {NULL}
};

static PyTypeObject b_MasterPubKeyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "breadwallet.MasterPubKey", /* tp_name */
    sizeof(b_MasterPubKey),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_as_async */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "MasterPubKey Object",     /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    b_MasterPubKeyMethods,     /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    b_MasterPubKeyNew,         /* tp_new */
};

static PyObject *b_DeriveKey(PyObject *self, PyObject *args) {
    const char *phrase;
    if (!PyArg_ParseTuple(args, "s", &phrase)) return NULL;
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, phrase, NULL);
    b_UInt512 *obj = PyObject_New(b_UInt512, &b_UInt512Type);
    obj->ob_fval = seed;
    Py_INCREF(obj);
    return (PyObject *)obj;
}

typedef struct {
    PyObject_HEAD
    BRKey ob_fval;
} b_Key;

static PyObject *b_KeyNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    b_Key *self = (b_Key *)type->tp_alloc(type, 0);
    if (self != NULL) {
    }
    return (PyObject *)self;
}

static PyObject *b_KeyFromBitID(PyObject *cls, PyObject *args, PyObject *kwds) {
    PyObject *result = NULL;
    PyObject *seedObj = NULL;
    int index = 0;
    char *endpoint = NULL;
    // parse args
    static char *kwlist[] = { "seed", "index", "endpoint", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Ois", kwlist, &seedObj, &index, &endpoint)) {
      return NULL;
    }
    if (!PyObject_IsInstance(seedObj, (PyObject *)&b_UInt512Type)) {
      // TODO: set correct error
      return NULL;
    }
    b_UInt512 *seed = (b_UInt512 *)seedObj;

    // create
    BRKey key;
    BRBIP32BitIDKey(&key, seed->ob_fval.u8, sizeof(seed->ob_fval.u8), index, endpoint);

    // allocate
    result = PyObject_CallFunction(cls, "");
    // set value
    if (result != NULL) {
      ((b_Key *)result)->ob_fval = key;
    }
    return result;
}

static PyObject *b_KeyAddress(PyObject *self, PyObject *args) {
    BRAddress address;
    b_Key *bkey = (b_Key *)self;
    BRKeyAddress(&bkey->ob_fval, address.s, sizeof(address));
    b_Address *ret = (b_Address *)PyObject_New(b_Address, &b_AddressType);
    ret->ob_fval = address;
    return (PyObject *)ret;
}

static PyMethodDef b_KeyMethods[] = {
    /* Class Methods */
    {"from_bitid", (PyCFunction)b_KeyFromBitID, (METH_VARARGS | METH_KEYWORDS | METH_CLASS),
     "generate a bitid Key from a seed and some bitid parameters"},
    /* Instance Methods */
    {"address", (PyCFunction)b_KeyAddress, METH_NOARGS,
     "get the address from the key"},
    {NULL}
};

static PyTypeObject b_KeyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "breadwallet.Key",         /* tp_name */
    sizeof(b_Key),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_as_async */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "Key Object",              /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    b_KeyMethods,              /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    b_KeyNew,                  /* tp_new */
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Wallet
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
    PyObject_HEAD
    b_MasterPubKey *mpk;
    BRWallet *ob_fval;
    PyObject *onSyncStarted;
    PyObject *onSyncSucceeded;
    PyObject *onSyncFailed;
    PyObject *onTxStatusUpdate;
} b_Wallet;

static PyObject *b_WalletNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    b_Wallet *self = (b_Wallet *)type->tp_alloc(type, 0);
    if (self != NULL) {
      self->mpk = NULL;
      self->ob_fval = NULL;
      self->onSyncStarted = NULL;
      self->onSyncSucceeded = NULL;
      self->onSyncFailed = NULL;
      self->onTxStatusUpdate = NULL;
    }
    return (PyObject *)self;
}

static void b_WalletDealloc(b_Wallet *self) {
  Py_XDECREF(self->mpk);
  self->mpk = NULL;
  BRWalletFree(self->ob_fval);
  self->ob_fval = NULL;
  if (self->onSyncStarted != NULL) {
    Py_XDECREF(self->onSyncStarted);
    self->onSyncStarted = NULL;
  }
  if (self->onSyncSucceeded != NULL) {
    Py_XDECREF(self->onSyncSucceeded);
    self->onSyncSucceeded = NULL;
  }
  if (self->onSyncFailed != NULL) {
    Py_XDECREF(self->onSyncFailed);
    self->onSyncFailed = NULL;
  }
  if (self->onTxStatusUpdate != NULL) {
    Py_XDECREF(self->onTxStatusUpdate);
    self->onTxStatusUpdate = NULL;
  }
  Py_TYPE(self)->tp_free((PyObject*)self);
}

static int b_WalletInit(b_Wallet *self, PyObject *args, PyObject *kwds) {
    PyObject *mpk = NULL, *txList = NULL;
    static char *kwlist[] = {"master_pubkey", "tx_list", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &mpk, &txList)) return -1;

    if (!mpk) {
      // TODO: set correct error
      return -1;
    }

    if (!PyObject_IsInstance(mpk, (PyObject *)&b_MasterPubKeyType)) {
      // TODO: set correct error
      return -1;
    }

    // build instance data
    self->mpk = (b_MasterPubKey *)mpk;
    Py_INCREF(self->mpk);
    self->ob_fval = BRWalletNew(NULL, 0, self->mpk->ob_fval);

    // TODO: parse transaction list

    return 0;
}

PyObject *b_WalletGetSyncStarted(b_Wallet *self, void *closure){
    if (self->onSyncStarted != NULL) {
        Py_INCREF(self->onSyncStarted);
        return self->onSyncStarted;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static int b_WalletSetSyncStarted(b_Wallet *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the on_sync_started attribute");
        return -1;
    }

    if (value != Py_None && !PyFunction_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The on_sync_started object must be a function");
        return -1;
    }

    if (self->onSyncStarted != NULL) {
        Py_DECREF(self->onSyncStarted);
    }
    Py_INCREF(value);
    self->onSyncStarted = value;

    return 0;
}

PyObject *b_WalletGetSyncSucceeded(b_Wallet *self, void *closure){
    if (self->onSyncSucceeded) {
        Py_INCREF(self->onSyncSucceeded);
        return self->onSyncSucceeded;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static int b_WalletSetSyncSucceeded(b_Wallet *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the on_sync_succeeded attribute");
        return -1;
    }

    if (value != Py_None && !PyFunction_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The on_sync_succeeded object must be a function");
        return -1;
    }

    if (self->onSyncSucceeded != NULL) {
        Py_DECREF(self->onSyncSucceeded);
    }
    Py_INCREF(value);
    self->onSyncSucceeded = value;

    return 0;
}

PyObject *b_WalletGetSyncFailed(b_Wallet *self, void *closure){
    if (self->onSyncFailed != NULL) {
        Py_INCREF(self->onSyncFailed);
        return self->onSyncFailed;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static int b_WalletSetSyncFailed(b_Wallet *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the on_sync_failed  attribute");
        return -1;
    }

    if (value != Py_None && !PyFunction_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The on_sync_failed object must be a function");
        return -1;
    }

    if (self->onSyncFailed != NULL) {
        Py_DECREF(self->onSyncFailed);
    }
    Py_INCREF(value);
    self->onSyncFailed = value;

    return 0;
}

PyObject *b_WalletGetTxStatusUpdate(b_Wallet *self, void *closure){
    if (self->onTxStatusUpdate != NULL) {
        Py_INCREF(self->onTxStatusUpdate);
        return self->onTxStatusUpdate;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static int b_WalletSetTxStatusUpdate(b_Wallet *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the on_tx_status_update attribute");
        return -1;
    }

    if (value != Py_None && !PyFunction_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "The on_tx_status_update object must be a function");
      return -1;
    }

    if (self->onTxStatusUpdate != NULL) {
      Py_DECREF(self->onTxStatusUpdate);
    }
    Py_INCREF(value);
    self->onTxStatusUpdate = value;

    return 0;
}

static PyGetSetDef b_WalletGetSetters[] = {
    {"on_sync_started",
     (getter)b_WalletGetSyncStarted, (setter)b_WalletSetSyncStarted,
     "callback fired when sync is started",
     NULL},
    {"on_sync_succeeded",
     (getter)b_WalletGetSyncSucceeded, (setter)b_WalletSetSyncSucceeded,
     "callback fired when sync finishes successfully",
     NULL},
     {"on_sync_failed",
      (getter)b_WalletGetSyncFailed, (setter)b_WalletSetSyncFailed,
      "callback fired when sync finishes with a failure",
      NULL},
    {"on_tx_status_update",
     (getter)b_WalletGetTxStatusUpdate, (setter)b_WalletSetTxStatusUpdate,
     "callback fired when transaction status is updated",
     NULL},
    {NULL}  /* Sentinel */
};

static PyTypeObject b_WalletType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "breadwallet.Wallet",      /* tp_name */
    sizeof(b_Wallet),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)b_WalletDealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_as_async */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "Wallet Object",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    b_WalletGetSetters,        /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)b_WalletInit,    /* tp_init */
    0,                         /* tp_alloc */
    b_WalletNew,               /* tp_new */
};

static PyMethodDef bmodulemethods[] = {
  {"derive_key", b_DeriveKey, METH_VARARGS, "Derive a key from a seed phrase"},
  {NULL},
};

static PyModuleDef bmodule = {
    PyModuleDef_HEAD_INIT,
    "breadwallet",
    "A simple, lightweight, performant SPV wallet",
    -1,
    bmodulemethods, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_breadwallet(void) {
    PyObject* m;

    if (PyType_Ready(&b_UInt512Type) < 0) return NULL;
    if (PyType_Ready(&b_MasterPubKeyType) < 0) return NULL;
    if (PyType_Ready(&b_KeyType) < 0) return NULL;
    if (PyType_Ready(&b_AddressType) < 0) return NULL;
    if (PyType_Ready(&b_TransactionType) < 0) return NULL;
    if (PyType_Ready(&b_WalletType) < 0) return NULL;

    m = PyModule_Create(&bmodule);
    if (m == NULL) return NULL;

    Py_INCREF(&b_UInt512Type);
    Py_INCREF(&b_MasterPubKeyType);
    Py_INCREF(&b_KeyType);
    Py_INCREF(&b_AddressType);
    Py_INCREF(&b_TransactionType);
    Py_INCREF(&b_WalletType);
    PyModule_AddObject(m, "UInt512", (PyObject *)&b_UInt512Type);
    PyModule_AddObject(m, "MasterPubKey", (PyObject *)&b_MasterPubKeyType);
    PyModule_AddObject(m, "Key", (PyObject *)&b_KeyType);
    PyModule_AddObject(m, "Address", (PyObject *)&b_KeyType);
    PyModule_AddObject(m, "Transaction", (PyObject *)&b_TransactionType);
    PyModule_AddObject(m, "Wallet", (PyObject *)&b_WalletType);
    return m;
}
