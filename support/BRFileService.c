//
//  BRFileService.c
//  Core
//
//  Created by Richard Evers on 1/4/19.
//  Copyright © 2019 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "BRFileService.h"
#include "BRArray.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include "../vendor/sqlite3/sqlite3.h"
typedef int sqlite3_status_code;

#define FILE_SERVICE_INITIAL_TYPE_COUNT    (5)
#define FILE_SERVICE_INITIAL_HANDLER_COUNT    (2)

#define FILE_SERVICE_SDB_FILENAME      "entities.db"

#define FILE_SERVICE_SDB_ENTITY_TABLE     \
"CREATE TABLE IF NOT EXISTS Entity(     \n\
  Type      CHAR(64)    NOT NULL,       \n\
  Hash      CHAR(64)    NOT NULL,       \n\
  Data      TEXT        NOT NULL,       \n\
  PRIMARY KEY (Type, Hash));"

typedef char FileServiceSQL[1024];

#define FILE_SERVICE_SDB_INSERT_ENTITY    \
"INSERT OR REPLACE INTO Entity (Type, Hash, Data) VALUES (?, ?, ?);"

#define FILE_SERVICE_SDB_QUERY_ENTITY     \
"SELECT Data FROM Entity WHERE Type = ? AND Hash = ?;"

#define FILE_SERVICE_SDB_QUERY_ALL_ENTITY     \
"SELECT Hash, Data FROM Entity WHERE Type = ?;"

#define FILE_SERVICE_SDB_UPDATE_ENTITY     \
"UPDATE Entity SET Data = ? WHERE Type = ? AND Hash = ?;"

#define FILE_SERVICE_SDB_DELETE_ENTITY     \
"DELETE FROM Entity WHERE Type = ? AND Hash = ?;"

#define FILE_SERVICE_SDB_DELETE_ALL_TYPE_ENTITY     \
"DELETE FROM Entity WHERE Type = ?;"

#define FILE_SERVICE_SDB_DELETE_ALL_ENTITY     \
"DELETE FROM Entity;"

#if defined(DEBUG)
static int needSQLiteCompileOptions = 1;
#endif
// HEX Encode/Decode - Cribbed from ethereum/util/BRUtilHex.c

// Convert a char into uint8_t (decode)
#define decodeChar(c)           ((uint8_t) _hexu(c))

// Convert a uint8_t into a char (encode)
#define encodeChar(u)           ((char)    _hexc(u))

static void
decodeHex (uint8_t *target, size_t targetLen, const char *source, size_t sourceLen) {
    //
    assert (0 == sourceLen % 2);
    assert (2 * targetLen == sourceLen);

    for (int i = 0; i < targetLen; i++) {
        target[i] = (uint8_t) ((decodeChar(source[2*i]) << 4) | decodeChar(source[(2*i)+1]));
    }
}

static void
encodeHex (char *target, size_t targetLen, const uint8_t *source, size_t sourceLen) {
    assert (targetLen == 2 * sourceLen  + 1);

    for (int i = 0; i < sourceLen; i++) {
        target[2*i + 0] = encodeChar (source[i] >> 4);
        target[2*i + 1] = encodeChar (source[i]);
    }
    target[2*sourceLen] = '\0';
}

/** Forward Declarations */
static int
fileServiceFailedSDB (BRFileService fs,
                      int releaseLock,
                      sqlite3_status_code code);

/// Return 0 on success, -1 otherwise
static int directoryMake (const char *path) {
    struct stat dirStat;
    if (0 == stat  (path, &dirStat)) return 0;  // if exists, success
    if (0 != mkdir (path, 0700))     return -1; // if can't create, error
    if (0 == stat  (path, &dirStat)) return 0;  // if exists, success
    return -1; // otherwise error
}

// This must be coercible to/from a uint8_t forever.
typedef enum {
    HEADER_FORMAT_1
} BRFileServiceHeaderFormatVersion;

static BRFileServiceHeaderFormatVersion currentHeaderFormatVersion = HEADER_FORMAT_1;

///
/// The handlers for a particular entity's version
///
typedef struct {
    BRFileServiceVersion version;
    BRFileServiceContext context;
    BRFileServiceIdentifier identifier;
    BRFileServiceReader reader;
    BRFileServiceWriter writer;
} BRFileServiceEntityHandler;

///
/// The set of handlers, by version, for a particular entity.
///
typedef struct {
    char *type;
    BRFileServiceVersion currentVersion;
    BRArrayOf(BRFileServiceEntityHandler) handlers;
} BRFileServiceEntityType;

static void
fileServiceEntityTypeRelease (const BRFileServiceEntityType *entityType) {
    free (entityType->type);
    if (NULL != entityType->handlers)
        array_free(entityType->handlers);
}

static BRFileServiceEntityHandler *
fileServiceEntityTypeLookupHandler (const BRFileServiceEntityType *entityType,
                                    BRFileServiceVersion version) {
    size_t handlersCount = array_count(entityType->handlers);
    for (size_t index = 0; index < handlersCount; index++)
        if (version == entityType->handlers[index].version)
            return &entityType->handlers[index];
    return NULL;
}

static void
fileServiceEntityTypeAddHandler (BRFileServiceEntityType *entityType,
                                 const BRFileServiceEntityHandler *handler) {
    // Lookup an existing handler:
    BRFileServiceEntityHandler *existingHandler = fileServiceEntityTypeLookupHandler(entityType, handler->version);

    if (NULL == existingHandler) // if none, add one
        array_add (entityType->handlers, *handler);
    else // if some, update
        *existingHandler = *handler;
}

///
///
///
struct BRFileServiceRecord {
    char    *sdbPath;
    sqlite3 *sdb;
    sqlite3_stmt *sdbInsertStmt;
    sqlite3_stmt *sdbSelectStmt;
    sqlite3_stmt *sdbSelectAllStmt;
    sqlite3_stmt *sdbUpdateStmt;
    sqlite3_stmt *sdbDeleteStmt;
    sqlite3_stmt *sdbDeleteAllTypeStmt;
    sqlite3_stmt *sdbDeleteAllStmt;
    uint8_t  sdbClosed;

    char *currency;
    char *network;

    pthread_mutex_t lock;
    
    BRArrayOf(BRFileServiceEntityType) entityTypes;
    BRFileServiceContext context;
    BRFileServiceErrorHandler handler;
};

static BRFileService
fileServiceCreateReturnError (BRFileService fs,
                              int releaseLock,
                              BRFileServiceError error) {
    // Nothing with 'error' at this point; a placeholder for now.
    if (releaseLock) pthread_mutex_unlock (&fs->lock);
    fileServiceRelease (fs);
    return NULL;
}

static char *
fileServiceCreateFilePath (const char *basePath,
                           const char *currency,
                           const char *network,
                           const char *filename) {
    size_t sdbPathLength = strlen (basePath) + 1 + strlen(currency) + 1 + strlen(network) + 1 + strlen (filename) + 1;
    char   *sdbPath      = malloc (sdbPathLength);
    sprintf (sdbPath, "%s/%s-%s-%s", basePath, currency, network, filename);
    return sdbPath;
}

extern BRFileService
fileServiceCreate (const char *basePath,
                   const char *currency,
                   const char *network,
                   BRFileServiceContext context,
                   BRFileServiceErrorHandler handler) {
    if (NULL == basePath || 0 == strlen(basePath)) return NULL;
    if (NULL == currency || 0 == strlen(currency)) return NULL;
    if (NULL == network  || 0 == strlen(network))  return NULL;

    // Reasonable limits on `network` and `currency` (ensure subsequent stack allocation works).
    if (strlen(network) > FILENAME_MAX || strlen(currency) > FILENAME_MAX)
        return NULL;

    // Make directory if needed.
    if (-1 == directoryMake(basePath)) return NULL;

    // Require `basePath` to be an existing directory.
    DIR *dir = opendir(basePath);
    if (NULL == dir) return NULL;
    closedir(dir);

    // Require SQLite to support 'MULTI_THREADED' or 'SERIALIZED'.  We'll lock our connection.
    // and thus 'MULTI_THREADED' is appropriate.
    if (0 == sqlite3_threadsafe()) return NULL;

    // Create the file service itself
    BRFileService fs = calloc (1, sizeof (struct BRFileServiceRecord));

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

        pthread_mutex_init(&fs->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Set the error handler - early
    fileServiceSetErrorHandler (fs, context, handler);

    fs->sdb = NULL;
    fs->sdbPath = NULL;
    fs->sdbClosed = 0;

    // Save currency and network
    fs->currency = strdup (currency);
    fs->network  = strdup (network);

    // Locate the SQLITE Database
    fs->sdbPath = fileServiceCreateFilePath (basePath, currency, network, FILE_SERVICE_SDB_FILENAME);

    // Create/Open the SQLITE Database
    sqlite3_status_code status = sqlite3_open(fs->sdbPath, &fs->sdb);
    if (SQLITE_OK != status) {
        fileServiceRelease (fs);
        return NULL;
    }

    // Create the SQLite 'Entity' Table
    sqlite3_stmt *sdbCreateTableStmt;
    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_ENTITY_TABLE, -1, &sdbCreateTableStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    if (SQLITE_DONE != sqlite3_step(sdbCreateTableStmt))
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });
    sqlite3_finalize(sdbCreateTableStmt);

    // Create the SQLITE 'Insert into Entity' Statement
    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_INSERT_ENTITY, -1, &fs->sdbInsertStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    // Create the SQLITE "Select Entity By Hash' Statement
    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_QUERY_ENTITY, -1, &fs->sdbSelectStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    // Create the SQLITE "Select Entity ' Statement
    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_QUERY_ALL_ENTITY, -1, &fs->sdbSelectAllStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_UPDATE_ENTITY, -1, &fs->sdbUpdateStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_DELETE_ENTITY, -1, &fs->sdbDeleteStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_DELETE_ALL_TYPE_ENTITY, -1, &fs->sdbDeleteAllTypeStmt, NULL);
    if (SQLITE_OK != status)
        return fileServiceCreateReturnError (fs, 1, (BRFileServiceError) {
            FILE_SERVICE_SDB,
            { .sdb = { status }}
        });

    // Allocate the `entityTypes` array
    array_new (fs->entityTypes, FILE_SERVICE_INITIAL_TYPE_COUNT);

#if defined(DEBUG)
    if (needSQLiteCompileOptions) {
        needSQLiteCompileOptions = 0;
        printf ("SQLITE ThreadSafe Mutex: %d\n", sqlite3_threadsafe());
        printf ("SQLITE Compile Options:\n");
        const char *option = NULL;
        for (int index = 0;
             NULL != (option = sqlite3_compileoption_get(index));
             index++) {
            printf ("-DSQLITE_%s\n", option);
        }
    }
#endif
    return fs;
}

static void
_fileServiceFinalizeStmt (BRFileService fs, sqlite3_stmt **stmt) {
    if (NULL != stmt && NULL != *stmt) {
        sqlite3_finalize (*stmt);
        *stmt = NULL;
    }
}

static void
_fileServiceCloseInternal (BRFileService fs) {
    if (fs->sdbClosed) return;

    fs->sdbClosed = 1;
    _fileServiceFinalizeStmt (fs, &fs->sdbInsertStmt);
    _fileServiceFinalizeStmt (fs, &fs->sdbSelectStmt);
    _fileServiceFinalizeStmt (fs, &fs->sdbSelectAllStmt);
    _fileServiceFinalizeStmt (fs, &fs->sdbUpdateStmt);
    _fileServiceFinalizeStmt (fs, &fs->sdbDeleteStmt);
    _fileServiceFinalizeStmt (fs, &fs->sdbDeleteAllTypeStmt);
    _fileServiceFinalizeStmt (fs, &fs->sdbDeleteAllStmt);

    if (NULL != fs->sdb) sqlite3_close (fs->sdb);
    fs->sdb = NULL;
}

extern void
fileServiceClose (BRFileService fs) {
    pthread_mutex_lock (&fs->lock);
    _fileServiceCloseInternal(fs);
    pthread_mutex_unlock (&fs->lock);
}

// This is callable with a partially allocated BRFileService.  So, be
// careful with fields that might not yet exist.
extern void
fileServiceRelease (BRFileService fs) {
    pthread_mutex_lock (&fs->lock);

    _fileServiceCloseInternal(fs);

    if (NULL != fs->entityTypes) {
        size_t typesCount = array_count(fs->entityTypes);
        for (size_t index = 0; index < typesCount; index++)
            fileServiceEntityTypeRelease (&fs->entityTypes[index]);
        array_free(fs->entityTypes);
    }

    if (NULL != fs->network)  free (fs->network);
    if (NULL != fs->currency) free (fs->currency);
    if (NULL != fs->sdbPath)  free (fs->sdbPath);

    pthread_mutex_unlock (&fs->lock);
    pthread_mutex_destroy(&fs->lock);

    free (fs);
}

extern void
fileServiceSetErrorHandler (BRFileService fs,
                            BRFileServiceContext context,
                            BRFileServiceErrorHandler handler) {
    fs->context = context;
    fs->handler = handler;
}

static BRFileServiceEntityType *
fileServiceLookupType (const BRFileService fs,
                       const char *type) {
    size_t typeCount = array_count(fs->entityTypes);
    for (size_t index = 0; index < typeCount; index++)
        if (0 == strcmp (type, fs->entityTypes[index].type))
            return &fs->entityTypes[index];
    return NULL;
}

static BRFileServiceEntityType *
fileServiceAddType (const BRFileService fs,
                    const char *type,
                    BRFileServiceVersion version) {
    BRFileServiceEntityType entityType = {
        strdup (type),
        version,
        NULL
    };
    array_new (entityType.handlers, FILE_SERVICE_INITIAL_HANDLER_COUNT);

    array_add (fs->entityTypes, entityType);
    return &fs->entityTypes[array_count(fs->entityTypes) - 1];
}

static BRFileServiceEntityHandler *
fileServiceLookupEntityHandler (const BRFileService fs,
                                const char *type,
                                BRFileServiceVersion version) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    return (NULL == entityType ? NULL : fileServiceEntityTypeLookupHandler (entityType, version));
}

/// MARK: - Failure Reporting

static int
fileServiceFailedInternal (BRFileService fs,
                           int releaseLock,
                           void* bufferToFree,
                           FILE* fileToClose,
                           BRFileServiceError error) {
    if (NULL != bufferToFree) free (bufferToFree);
    if (NULL != fileToClose)  fclose (fileToClose);
    if (releaseLock) pthread_mutex_unlock (&fs->lock);

    // Handler invoked w/o the lock.  Avoid a possible recursive use of FS.
    if (NULL != fs->handler)
        fs->handler (fs->context, fs, error);

    return 0;
}

static int
fileServiceFailedImpl(BRFileService fs,
                      int releaseLock,
                      void* bufferToFree,
                      FILE* fileToClose,
                      const char *reason) {
    return fileServiceFailedInternal (fs, releaseLock, bufferToFree, fileToClose,
                                      (BRFileServiceError) {
                                          FILE_SERVICE_IMPL,
                                          { .impl = { reason }}
                                      });
}

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-function"
static int
fileServiceFailedUnix(BRFileService fs,
                      int releaseLock,
                      void* bufferToFree,
                      FILE* fileToClose,
                      int error) {
    return fileServiceFailedInternal (fs, releaseLock, bufferToFree, fileToClose,
                                      (BRFileServiceError) {
                                          FILE_SERVICE_UNIX,
                                          { .unix = { error }}
                                      });
}
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

static int
fileServiceFailedSDB (BRFileService fs,
                      int releaseLock,
                      sqlite3_status_code code) {
    return fileServiceFailedInternal (fs, releaseLock, NULL, NULL,
                                      (BRFileServiceError) {
                                          FILE_SERVICE_SDB,
                                          { .sdb = { code, sqlite3_errstr(code) }}
                                      });
}

static int
fileServiceFailedEntity(BRFileService fs,
                        int releaseLock,
                        void* bufferToFree,
                        FILE* fileToClose,
                        const char *type,
                        const char *reason) {
    return fileServiceFailedInternal (fs, releaseLock, bufferToFree, fileToClose,
                                      (BRFileServiceError) {
                                          FILE_SERVICE_ENTITY,
                                          { .entity = { type, reason }}
                                      });
}

/// MARK: - Save

static int
_fileServiceSave (BRFileService fs,
                  const char *type,  /* block, peers, transactions, logs, ... */
                  const void *entity,
                  int needLock) {     /* BRMerkleBlock*, BRTransaction, BREthereumTransaction, ... */

    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) { fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type"); return 0; };

    BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == handler) { fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type handler"); return 0; };

    // Get then hex-encode the identifer
    UInt256 identifier = handler->identifier (handler->context, fs, entity);
    const char *hash = u256hex(identifier);

    // Get the entity bytes
    uint32_t entityBytesCount;
    uint8_t *entityBytes = handler->writer (handler->context, fs, entity, &entityBytesCount);

    // Always, always write the header for the currentHeaderFormatVersion

    // Extend the entity bytes with the current header format, which is:
    //   {HeaderFormatVersion, Current(Type)Version, EntityBytesCount, EntityBytes}
    size_t  offset = 0;
    size_t  bytesCount = 1 + 1 + sizeof(uint32_t) + entityBytesCount;
    uint8_t *bytes = malloc (bytesCount);

    bytes[offset] = (uint8_t) currentHeaderFormatVersion;
    offset += 1;

    bytes[offset] = (uint8_t) entityType->currentVersion;
    offset += 1;

    UInt32SetBE (&bytes[offset], entityBytesCount);
    offset += sizeof (uint32_t);

    memcpy (&bytes[offset], entityBytes, entityBytesCount);
    free (entityBytes);

    // Nex encode bytes
    size_t dataCount = 2 * bytesCount + 1;
    char *data = malloc (dataCount);
    encodeHex (data, dataCount, bytes, bytesCount);
    free (bytes);

    // Fill out the SQL statement
    sqlite3_status_code status;

    if (needLock)
        pthread_mutex_lock (&fs->lock);

    if (fs->sdbClosed)
        return fileServiceFailedImpl (fs, needLock, NULL, NULL, "closed");

    sqlite3_reset (fs->sdbInsertStmt);
    sqlite3_clear_bindings(fs->sdbInsertStmt);

    status = sqlite3_bind_text (fs->sdbInsertStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
        free (data);
        return fileServiceFailedSDB (fs, needLock, status);
    }

    status = sqlite3_bind_text (fs->sdbInsertStmt, 2, hash, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
        free (data);
        return fileServiceFailedSDB (fs, needLock, status);
    }

    status = sqlite3_bind_text (fs->sdbInsertStmt, 3, data, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
        free (data);
        return fileServiceFailedSDB (fs, needLock, status);
    }

    status = sqlite3_step (fs->sdbInsertStmt);
    if (SQLITE_DONE != status) {
        free (data);
        return fileServiceFailedSDB (fs, needLock, status);
    }

    // Ensure the 'implicit DB transaction' is committed.
    sqlite3_reset (fs->sdbInsertStmt);

    if (needLock)
        pthread_mutex_unlock (&fs->lock);

    free (data);
    return 1;
}

extern int
fileServiceSave (BRFileService fs,
                 const char *type,  /* block, peers, transactions, logs, ... */
                 const void *entity) {     /* BRMerkleBlock*, BRTransaction, BREthereumTransaction, ... */
    return _fileServiceSave (fs, type, entity, 1);
}

/// MARK: - Load

extern int
fileServiceLoad (BRFileService fs,
                 BRSet *results,
                 const char *type,
                 int updateVersion) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type");

    BRFileServiceEntityHandler *entityHandlerCurrent = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == entityHandlerCurrent) return fileServiceFailedImpl (fs,  0, NULL, NULL, "missed type handler");

    sqlite3_status_code status;

    pthread_mutex_lock (&fs->lock);
    if (fs->sdbClosed)
        return fileServiceFailedImpl (fs, 1, NULL, NULL, "closed");

    sqlite3_reset (fs->sdbSelectAllStmt);
    sqlite3_clear_bindings (fs->sdbSelectAllStmt);

    status = sqlite3_bind_text (fs->sdbSelectAllStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status)
        return fileServiceFailedSDB (fs, 1, status);

    uint8_t  dataBytesBuffer[8196];
    uint8_t *dataBytes = dataBytesBuffer;
    size_t   dataBytesCount = 8196;

    while (SQLITE_ROW == sqlite3_step(fs->sdbSelectAllStmt)) {
        const char *hash = (const char *) sqlite3_column_text (fs->sdbSelectAllStmt, 0);
        const char *data = (const char *) sqlite3_column_text (fs->sdbSelectAllStmt, 1);

        if (NULL == hash || NULL == data)
            return fileServiceFailedImpl (fs, 1, (dataBytes == dataBytesBuffer ? NULL : dataBytes), NULL,
                                          "missed query `hash` or `data`");

        assert (64 == strlen (hash));
        
        // Ensure `dataBytes` is large enough for hex-decoded `data`
        size_t dataCount = strlen (data);
        assert (0 == dataCount % 2);  // Surely 'even'
        if ((dataCount/2) > dataBytesCount) {
            if (dataBytes != dataBytesBuffer) free (dataBytes);
            dataBytesCount = dataCount/2;
            dataBytes = malloc (dataBytesCount);
        }

        // Actually decode `data` into `dataBytes`
        decodeHex (dataBytes, dataCount/2, data, dataCount);

        size_t offset = 0;
        BRFileServiceVersion version;
        uint32_t  entityBytesCount;
        uint8_t  *entityBytes;

        BRFileServiceHeaderFormatVersion headerVersion = dataBytes[offset];
        offset += 1;

        switch (headerVersion) {
            case HEADER_FORMAT_1:
                version = dataBytes[offset];
                offset += 1;

                entityBytesCount = UInt32GetBE (&dataBytes[offset]);
                offset += sizeof (uint32_t);

                break;
        }

        // Assert entityBytesCount remain in dataBytes
        if (offset + entityBytesCount > dataBytesCount) {
            assert (0); // In DEBUG builds.
            return fileServiceFailedImpl (fs, 1, (dataBytes == dataBytesBuffer ? NULL : dataBytes), NULL,
                                          "missed bytes count");
        }

        entityBytes = &dataBytes[offset];

        switch (headerVersion) {
            case HEADER_FORMAT_1:
                // compute then compare checksum
                break;
        }

        // Look up the entity handler
        BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, version);
        if (NULL == handler)
            return fileServiceFailedImpl (fs, 1, (dataBytes == dataBytesBuffer ? NULL : dataBytes), NULL,
                                          "missed type handler");

        // Read the entity from buffer and add to results.
        void *entity = handler->reader (handler->context, fs, entityBytes, entityBytesCount);
        if (NULL == entity)
            return fileServiceFailedEntity (fs, 1, (dataBytes == dataBytesBuffer ? NULL : dataBytes), NULL,
                                            type, "reader");

        // Update restuls with the newly restored entity
        BRSetAdd (results, entity);

        // If the read version is not the current version, update
        if (updateVersion &&
            (version != entityType->currentVersion ||
             headerVersion != currentHeaderFormatVersion))
            // This could signal an error.  Perhaps we should test the return result and
            // if `0` skip out here?  We won't - we couldn't save the entity in the new format
            // but we'll continue and will try next time we load it.
            _fileServiceSave (fs, type, entity, 0);
    }

    // Ensure the 'implicit DB transaction' is committed.
    sqlite3_reset (fs->sdbSelectAllStmt);

    pthread_mutex_unlock (&fs->lock);

    if (dataBytes != dataBytesBuffer) free (dataBytes);

    return 1;
}

/// MARK: - Remove, Clear

extern int
fileServiceRemove (BRFileService fs,
                   const char *type,
                   UInt256 identifier) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType)
        return fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type");

    // Hex-Encode identifier
    const char *hash = u256hex(identifier);

    sqlite3_status_code status;

    pthread_mutex_lock (&fs->lock);
    if (fs->sdbClosed)
        return fileServiceFailedImpl (fs, 1, NULL, NULL, "closed");

    sqlite3_reset (fs->sdbDeleteStmt);
    sqlite3_clear_bindings (fs->sdbDeleteStmt);

    status = sqlite3_bind_text (fs->sdbDeleteStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status)
        return fileServiceFailedSDB (fs, 1, status);

    status = sqlite3_bind_text (fs->sdbDeleteStmt, 2, hash, -1, SQLITE_STATIC);
    if (SQLITE_OK != status)
        return fileServiceFailedSDB (fs, 1, status);

    status = sqlite3_step (fs->sdbDeleteStmt);
    if (SQLITE_DONE != status)
        return fileServiceFailedSDB (fs, 1, status);

    // Ensure the 'implicit DB transaction' is committed.
    sqlite3_reset (fs->sdbDeleteStmt);

    pthread_mutex_unlock (&fs->lock);

    return 1;
}

static int
fileServiceClearForType (BRFileService fs,
                         BRFileServiceEntityType *entityType,
                         int needLock) {
    const char *type = entityType->type;

    sqlite3_status_code status;

    if (needLock) pthread_mutex_lock (&fs->lock);
    if (fs->sdbClosed)
        return fileServiceFailedImpl (fs, needLock, NULL, NULL, "closed");

    sqlite3_reset (fs->sdbDeleteAllTypeStmt);
    sqlite3_clear_bindings (fs->sdbDeleteAllTypeStmt);

    status = sqlite3_bind_text (fs->sdbDeleteAllTypeStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status)
        return fileServiceFailedSDB (fs, needLock, status);

    status = sqlite3_step (fs->sdbDeleteAllTypeStmt);
    if (SQLITE_DONE != status)
        return fileServiceFailedSDB (fs, needLock, status);

    // Ensure the 'implicit DB transaction' is committed.
    sqlite3_reset (fs->sdbDeleteAllTypeStmt);

    if (needLock) pthread_mutex_unlock (&fs->lock);

    return 1;
}

extern int
fileServiceClear (BRFileService fs,
                  const char *type) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType)
        return fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type");

    return fileServiceClearForType(fs, entityType, 1);
}

extern int
fileServiceClearAll (BRFileService fs) {
    int success = 1;
    size_t typeCount = array_count(fs->entityTypes);
    for (size_t index = 0; index < typeCount; index++)
        success &= fileServiceClearForType (fs, &fs->entityTypes[index], 1);
    return success;
}

static int
fileServiceReplaceFailed (BRFileService fs, int needUnlock) {
    if (needUnlock) pthread_mutex_unlock (&fs->lock);
    return 0;
}

extern int
fileServiceReplace (BRFileService fs,
                    const char *type,
                    const void **entities,
                    size_t entitiesCount) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType)
        return fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type");

    sqlite3_status_code status;

    pthread_mutex_lock (&fs->lock);
    if (fs->sdbClosed)
        return fileServiceFailedImpl (fs, 1, NULL, NULL, "closed");

    status = sqlite3_exec (fs->sdb, "BEGIN", NULL, NULL, NULL);
    if (SQLITE_OK != status)
        return fileServiceFailedSDB (fs, 1, status);

    if (0 == fileServiceClearForType (fs, entityType, 0))
        return fileServiceReplaceFailed (fs, 1);

    for (size_t index = 0; index < entitiesCount; index++)
        if (0 == _fileServiceSave (fs, type, entities[index], 0))
            return fileServiceReplaceFailed (fs, 1);

    status = sqlite3_exec (fs->sdb, "COMMIT", NULL, NULL, NULL);
    if (SQLITE_OK != status)
        return fileServiceFailedSDB (fs, 1, status);

    pthread_mutex_unlock (&fs->lock);

    return 1;
}

extern int
fileServiceWipe (const char *basePath,
                 const char *currency,
                 const char *network) {

    // Locate the SQLITE Database
    char *sdbPath = fileServiceCreateFilePath (basePath, currency, network, FILE_SERVICE_SDB_FILENAME);

    // Remove it.
    int   result  = 0 == remove (sdbPath) ? 0 : errno;
    free (sdbPath);
    return result;
}

extern UInt256
fileServiceGetIdentifier (BRFileService fs,
                          const char *type,
                          const void *entity) {

    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) { fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type"); return UINT256_ZERO; };

    BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == handler) { fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type handler"); return UINT256_ZERO; };

    return handler->identifier (handler->context, fs, entity);
}

extern int
fileServiceDefineType (BRFileService fs,
                       const char *type,
                       BRFileServiceVersion version,
                       BRFileServiceContext context,
                       BRFileServiceIdentifier identifier,
                       BRFileServiceReader reader,
                       BRFileServiceWriter writer) {
    // Lookup the entityType for `type`
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);

    // If there isn't an entityType, create one.
    if (NULL == entityType)
        entityType = fileServiceAddType (fs, type, version);

    // Create a handler for the entity
    BRFileServiceEntityHandler newEntityHander = {
        version,
        context,
        identifier,
        reader,
        writer
    };

    // Lookup an existing entityHandler for `version`
    BRFileServiceEntityHandler *entityHandler = fileServiceLookupEntityHandler (fs, type, version);

    // If there is an entityHandler, update it.
    if (NULL != entityHandler)
        *entityHandler = newEntityHander;
    
    // otherwise add one
    else {
        fileServiceEntityTypeAddHandler (entityType, &newEntityHander);
    }

    return 1;
}

extern int
fileServiceDefineCurrentVersion (BRFileService fs,
                                 const char *type,
                                 BRFileServiceVersion version) {
    // Find the entityType for `type`
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type");

    // Find the entityHandler, for version.
    BRFileServiceEntityHandler *entityHandler = fileServiceEntityTypeLookupHandler (entityType, version);
    if (NULL == entityHandler) return fileServiceFailedImpl (fs, 0, NULL, NULL, "missed type handler");

    // We have a handler, therefore it can be the current one.
    entityType->currentVersion = version;

    return 1;
}

extern BRFileService
fileServiceCreateFromTypeSpecfications (const char *basePath,
                                        const char *currency,
                                        const char *network,
                                        BRFileServiceContext context,
                                        BRFileServiceErrorHandler handler,
                                        size_t specificationsCount,
                                        BRFileServiceTypeSpecification *specfications) {
    int success = 1;

    BRFileService fileService = fileServiceCreate (basePath,
                                                   currency,
                                                   network,
                                                   context,
                                                   handler);
    if (NULL == fileService) return NULL;

    for (size_t index = 0; index < specificationsCount; index++) {
        BRFileServiceTypeSpecification *specification = &specfications[index];
        for (size_t vindex = 0; vindex < specification->versionsCount; vindex++) {
            success &= fileServiceDefineType (fileService,
                                              specification->type,
                                              specification->versions[vindex].version,
                                              context,
                                              specification->versions[vindex].identifier,
                                              specification->versions[vindex].reader,
                                              specification->versions[vindex].writer);
            if (!success) break;
        }

        success &= fileServiceDefineCurrentVersion (fileService,
                                                    specification->type,
                                                    specification->defaultVersion);
        if (!success) break;
    }

    if (success) return fileService;
    else { fileServiceRelease (fileService); return NULL; }
}
