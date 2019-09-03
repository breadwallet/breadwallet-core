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
#include <sqlite3.h>
typedef int sqlite3_status_code;

#define FILE_SERVICE_INITIAL_TYPE_COUNT    (5)
#define FILE_SERVICE_INITIAL_HANDLER_COUNT    (2)

#define FILE_SERVICE_SDB_FILENAME      "entities.db"

#define FILE_SERVICE_SDB_ENTITY_TABLE     \
"CREATE TABLE IF NOT EXISTS Entity(     \n\
  Currency  CHAR(64)    NOT NULL,       \n\
  Network   CHAR(64)    NOT NULL,       \n\
  Type      CHAR(64)    NOT NULL,       \n\
  Hash      TEXT        PRIMARY KEY,    \n\
  Data      TEXT        NOT NULL);"

typedef char FileServiceSQL[1024];

#define FILE_SERVICE_SDB_INSERT_ENTITY_TEMPLATE    \
"INSERT OR REPLACE INTO Entity (Currency, Network, Type, Hash, Data) VALUES (\"%s\", \"%s\", ?, ?, ?);"

#define FILE_SERVICE_SDB_QUERY_ENTITY_TEMPLATE     \
"SELECT Data FROM Entity WHERE Currency = \"%c\" AND Network = \"%s\" AND Type = ? AND Hash = ?;"

#define FILE_SERVICE_SDB_QUERY_ALL_ENTITY_TEMPLATE     \
"SELECT Hash, Data FROM Entity WHERE Currency = \"%s\" AND Network = \"%s\" AND Type = ?;"

#define FILE_SERVICE_SDB_UPDATE_ENTITY_TEMPLATE     \
"UPDATE Entity SET Data = ? WHERE Currency = \"%s\" AND Network = \"%s\" AND Type = ? AND Hash = ?;"

#define FILE_SERVICE_SDB_DELETE_ENTITY_TEMPLATE     \
"DELETE FROM Entity WHERE Currency = \"%s\" AND Network = \"%s\" AND Type = ? AND Hash = ?;"

#define FILE_SERVICE_SDB_DELETE_ALL_TYPE_ENTITY_TEMPLATE     \
"DELETE FROM Entity WHERE Currency = \"%s\" AND Network = \"%s\" AND Type = ?;"

#define FILE_SERVICE_SDB_DELETE_ALL_ENTITY_TEMPLATE     \
"DELETE FROM Entity WHERE Currency = \"%s\" AND Network = \"%s\";"

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

extern void
encodeHex (char *target, size_t targetLen, const uint8_t *source, size_t sourceLen) {
    assert (targetLen == 2 * sourceLen  + 1);

    for (int i = 0; i < sourceLen; i++) {
        target[2*i + 0] = encodeChar (source[i] >> 4);
        target[2*i + 1] = encodeChar (source[i]);
    }
    target[2*sourceLen] = '\0';
}

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

    char *currency;
    char *network;

    BRArrayOf(BRFileServiceEntityType) entityTypes;
    BRFileServiceContext context;
    BRFileServiceErrorHandler handler;
};

static void
fileServiceSQLFill (BRFileService fs, FileServiceSQL sql, const char *templete) {
    sprintf(sql, templete, fs->currency, fs->network);
}

extern BRFileService
fileServiceCreate (const char *basePath,
                   const char *currency,
                   const char *network,
                   BRFileServiceContext context,
                   BRFileServiceErrorHandler handler) {
    if (NULL == currency || 0 == strlen(currency)) return NULL;
    if (NULL == network  || 0 == strdup(network))  return NULL;

    // Reasonable limits on `network` and `currency` (ensure subsequent stack allocation works).
    if (strlen(network) > FILENAME_MAX || strlen(currency) > FILENAME_MAX)
        return NULL;

    // Make directory if needed.
    if (-1 == directoryMake(basePath)) return NULL;

    // Require `basePath` to be an existing directory.
    DIR *dir = opendir(basePath);
    if (NULL == dir) return NULL;
    closedir(dir);

    // Create the file service itself
    BRFileService fs = calloc (1, sizeof (struct BRFileServiceRecord));
    fs->sdb = NULL;
    fs->sdbPath = NULL;

    // Save currency and network
    fs->currency = strdup (currency);
    fs->network  = strdup (network);

    // Locate the SQLITE Database
    size_t sdbPathLength = strlen (basePath) + 1 + strlen (FILE_SERVICE_SDB_FILENAME) + 1;
    fs->sdbPath = malloc (sdbPathLength);
    sprintf (fs->sdbPath, "%s/%s", basePath, FILE_SERVICE_SDB_FILENAME);

    // Create/Open the SQLITE Database
    sqlite3_status_code status = sqlite3_open(fs->sdbPath, &fs->sdb);
    if (SQLITE_OK != status) {
        if (NULL != fs->sdb) sqlite3_close(fs->sdb);
        if (NULL != fs->sdbPath) free (fs->sdbPath);
        free (fs);

        // Set error info
        return NULL;
    }

    // Create the SQLite 'Entity' Table
    sqlite3_stmt *sdbCreateTableStmt;
    status = sqlite3_prepare_v2 (fs->sdb, FILE_SERVICE_SDB_ENTITY_TABLE, -1, &sdbCreateTableStmt, NULL);
    if (SQLITE_OK != status) {
        // ...
        printf ("foo\n");
//        return NULL;
    }
    if (SQLITE_DONE != sqlite3_step(sdbCreateTableStmt)) {
        printf ("bar\n");
 //       return NULL;
    }
    sqlite3_finalize(sdbCreateTableStmt);

    FileServiceSQL sql;

    // Create the SQLITE 'Insert into Entity' Statement
    fileServiceSQLFill (fs, sql, FILE_SERVICE_SDB_INSERT_ENTITY_TEMPLATE);
    status = sqlite3_prepare_v2 (fs->sdb, sql, -1, &fs->sdbInsertStmt, NULL);
    if (SQLITE_OK != status) {
        //
        return NULL;
    }

    // Create the SQLITE "Select Entity By Hash' Statement
    fileServiceSQLFill (fs, sql, FILE_SERVICE_SDB_QUERY_ENTITY_TEMPLATE);
    status = sqlite3_prepare_v2 (fs->sdb, sql, -1, &fs->sdbSelectStmt, NULL);
    if (SQLITE_OK != status) {
        // ...
        return NULL;
    }

    // Create the SQLITE "Select Entity ' Statement
    fileServiceSQLFill (fs, sql, FILE_SERVICE_SDB_QUERY_ALL_ENTITY_TEMPLATE);
    status = sqlite3_prepare_v2 (fs->sdb, sql, -1, &fs->sdbSelectAllStmt, NULL);
    if (SQLITE_OK != status) {
        // ...
        return NULL;
    }

    fileServiceSQLFill (fs, sql, FILE_SERVICE_SDB_UPDATE_ENTITY_TEMPLATE);
    status = sqlite3_prepare_v2 (fs->sdb, sql, -1, &fs->sdbUpdateStmt, NULL);
    if (SQLITE_OK != status) {
        // ...
        return NULL;
    }

    fileServiceSQLFill (fs, sql, FILE_SERVICE_SDB_DELETE_ENTITY_TEMPLATE);
    status = sqlite3_prepare_v2 (fs->sdb, sql, -1, &fs->sdbDeleteStmt, NULL);
    if (SQLITE_OK != status) {
        // ...
        return NULL;
    }

    fileServiceSQLFill (fs, sql, FILE_SERVICE_SDB_DELETE_ALL_TYPE_ENTITY_TEMPLATE);
    status = sqlite3_prepare_v2 (fs->sdb, sql, -1, &fs->sdbDeleteAllTypeStmt, NULL);
    if (SQLITE_OK != status) {
        // ...
        return NULL;
    }

    // Allocate the `entityTypes` array
    array_new (fs->entityTypes, FILE_SERVICE_INITIAL_TYPE_COUNT);

    // Set the error handler
    fileServiceSetErrorHandler (fs, context, handler);

    return fs;
}

extern void
fileServiceRelease (BRFileService fs) {
    size_t typesCount = array_count(fs->entityTypes);
    for (size_t index = 0; index < typesCount; index++)
        fileServiceEntityTypeRelease (&fs->entityTypes[index]);
    array_free(fs->entityTypes);


    free (fs->network);
    free (fs->currency);
    free (fs->sdbPath);
    sqlite3_finalize(fs->sdbInsertStmt);
    sqlite3_finalize(fs->sdbSelectStmt);
    sqlite3_finalize(fs->sdbSelectAllStmt);
    sqlite3_close(fs->sdb);

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
                               void* bufferToFree,
                               FILE* fileToClose,
                               BRFileServiceError error) {
    if (NULL != bufferToFree) free (bufferToFree);
    if (NULL != fileToClose)  fclose (fileToClose);

    if (NULL != fs->handler)
        fs->handler (fs->context, fs, error);

    return 0;
}

static int
fileServiceFailedImpl(BRFileService fs,
                          void* bufferToFree,
                          FILE* fileToClose,
                          const char *reason) {
    return fileServiceFailedInternal (fs, bufferToFree, fileToClose,
                                          (BRFileServiceError) {
                                              FILE_SERVICE_IMPL,
                                              { .impl = { reason }}
                                          });
}

static int
fileServiceFailedUnix(BRFileService fs,
                          void* bufferToFree,
                          FILE* fileToClose,
                          int error) {
    return fileServiceFailedInternal (fs, bufferToFree, fileToClose,
                                          (BRFileServiceError) {
                                              FILE_SERVICE_UNIX,
                                              { .unix = { error }}
                                          });
}

static int
fileServiceFailedEntity(BRFileService fs,
                            void* bufferToFree,
                            FILE* fileToClose,
                            const char *type,
                            const char *reason) {
    return fileServiceFailedInternal (fs, bufferToFree, fileToClose,
                                          (BRFileServiceError) {
                                              FILE_SERVICE_ENTITY,
                                              { .entity = { type, reason }}
                                          });
}

/// MARK: - Load

extern int
fileServiceLoad (BRFileService fs,
                 BRSet *results,
                 const char *type,
                 int updateVersion) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return fileServiceFailedImpl (fs, NULL, NULL, "missed type");

    BRFileServiceEntityHandler *entityHandlerCurrent = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == entityHandlerCurrent) return fileServiceFailedImpl (fs,  NULL, NULL, "missed type handler");

    sqlite3_status_code status;

    status = sqlite3_bind_text (fs->sdbSelectAllStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
        printf ("baz\n");
    }

    // Allocate some storage for entity bytes;
    size_t bufferSize = 8 * 1024;
    uint8_t *buffer = malloc (bufferSize);
    if (NULL == buffer) return fileServiceFailedUnix (fs, NULL, NULL, ENOMEM);

    while (SQLITE_ROW == sqlite3_step(fs->sdbSelectAllStmt)) {
        const char *hash = (const char *) sqlite3_column_text (fs->sdbSelectAllStmt, 0);
        const char *data = (const char *) sqlite3_column_text (fs->sdbSelectAllStmt, 1);

        // Hex-Decode hash
        size_t  hashBytesCount = strlen(hash) / 2;
        uint8_t hashBytes [hashBytesCount];
        decodeHex (hashBytes, hashBytesCount, hash, strlen(hash));

        // Hex-Decode data
        size_t  dataBytesCount = strlen(data) / 2;
        uint8_t dataBytes [dataBytesCount];
        decodeHex (dataBytes, dataBytesCount, data, strlen(data));

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
        entityBytes = &dataBytes[offset];

        switch (headerVersion) {
            case HEADER_FORMAT_1:
                // compute then compare checksum
                break;
        }


//    while (NULL != (dirEntry = readdir(dir)))
//        if (dirEntry->d_type == DT_REG) {
//            sprintf (filename, "%s/%s", dirPath, dirEntry->d_name);
//            FILE *file = fopen (filename, "rb");
//            if (NULL == file) return fileServiceFailedUnix (fs, buffer, NULL, errno);
//
//
//            BRFileServiceVersion version;
//            uint32_t bytesCount;
//
//            // read the header version
//            BRFileServiceHeaderFormatVersion headerVersion;
//            if (1 != fread (&headerVersion, sizeof(BRFileServiceHeaderFormatVersion), 1, file))
//                return fileServiceFailedUnix (fs, buffer, file, errno);
//
//            // read the header
//            switch (headerVersion) {
//                case HEADER_FORMAT_1: {
//                    // read the version
//                    if (1 != fread (&version, sizeof(BRFileServiceVersion), 1, file) ||
//                        // read the checksum
//                        // read the bytesCount
//                        1 != fread (&bytesCount, sizeof(uint32_t), 1, file))
//                        return fileServiceFailedUnix (fs, buffer, file, errno);
//
//                    break;
//                }
//            }
//
//            // Ensure `buffer` is large enough
//            if (bytesCount > bufferSize) {
//                bufferSize = bytesCount;
//
//                uint8_t *bufferNew = realloc (buffer, bufferSize);
//                if (NULL == bufferNew) return fileServiceFailedUnix (fs, buffer, NULL, ENOMEM);
//                buffer = bufferNew;
//            }
//
//            // read the bytes - multiple might be required
//            if (bytesCount != fread (buffer, 1, bytesCount, file))
//                return fileServiceFailedUnix (fs, buffer, file, errno);
//
//            // All file reading is complete; next read should be EOF.
//
//            // Done with file.
//            if (0 != fclose (file))
//                return fileServiceFailedUnix (fs, buffer, NULL, errno);
//
//            // We now have everything
//
//            // This will need some later rework.  If a header includes some data, like a checksum,
//            // we won't have that value in this context when needed.
//
//            // Do something header specific
//            switch (headerVersion) {
//                case HEADER_FORMAT_1:
//                    // compute the checksum
//                    // compare the checksum
//                   break;
//            }

        // Look up the entity handler
        BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, version);
        if (NULL == handler) return fileServiceFailedImpl (fs,  buffer, NULL, "missed type handler");

        // Read the entity from buffer and add to results.
        void *entity = handler->reader (handler->context, fs, entityBytes, entityBytesCount);
        if (NULL == entity) return fileServiceFailedEntity (fs, buffer, NULL, type, "reader");

        // Update restuls with the newly restored entity
        BRSetAdd (results, entity);

        // If the read version is not the current version, update
        if (updateVersion &&
            (version != entityType->currentVersion ||
             headerVersion != currentHeaderFormatVersion))
            fileServiceSave (fs, type, entity);
    }
    sqlite3_reset (fs->sdbSelectAllStmt);

    free (buffer);

    return 1;
}

/// MARK: - Save

extern void /* error code? */
fileServiceSave (BRFileService fs,
                 const char *type,  /* block, peers, transactions, logs, ... */
                 const void *entity) {     /* BRMerkleBlock*, BRTransaction, BREthereumTransaction, ... */
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) { fileServiceFailedImpl (fs, NULL, NULL, "missed type"); return; };

    BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == handler) { fileServiceFailedImpl (fs,  NULL, NULL, "missed type handler"); return; };

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
    uint8_t bytes[bytesCount];

    bytes[offset] = (uint8_t) currentHeaderFormatVersion;
    offset += 1;

    bytes[offset] = (uint8_t) entityType->currentVersion;
    offset += 1;

    UInt32SetBE (&bytes[offset], entityBytesCount);
    offset += sizeof (uint32_t);

    memcpy (&bytes[offset], entityBytes, entityBytesCount);

    // Nex encode bytes
    size_t dataCount = 2 * bytesCount + 1;
    char data [dataCount];
    encodeHex (data, dataCount, bytes, bytesCount);

    // Fill out the SQL statement
    sqlite3_status_code status;

    status = sqlite3_bind_text (fs->sdbInsertStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
        return;
    }

    status = sqlite3_bind_text (fs->sdbInsertStmt, 2, hash, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
        return;
    }

    status = sqlite3_bind_text (fs->sdbInsertStmt, 3, data, -1, SQLITE_TRANSIENT);
    if (SQLITE_OK != status) {
        return;
    }

    // Execute the Update
    if (SQLITE_DONE != sqlite3_step (fs->sdbInsertStmt)) {

    }
    sqlite3_reset (fs->sdbInsertStmt);
    sqlite3_clear_bindings(fs->sdbInsertStmt);

//    if (// write the header version
//        1 != fwrite(&currentHeaderFormatVersion, sizeof(BRFileServiceHeaderFormatVersion), 1, file) ||
//        // then the version
//        1 != fwrite (&entityType->currentVersion, sizeof(BRFileServiceVersion), 1, file) ||
//        // then the checksum?
//        // write the bytesCount
//        1 != fwrite(&bytesCount, sizeof (uint32_t), 1, file)) {
//        fileServiceFailedUnix (fs, bytes, file, errno);
//        return;
//    }
//
//    // write the bytes.
//    if (bytesCount != fwrite(bytes, 1, bytesCount, file)) {
//        fileServiceFailedUnix (fs, bytes, file, errno);
//        return;
//    }
//
//    if (0 != fclose (file)) {  fileServiceFailedUnix (fs, bytes, NULL, errno); return; }
//
//    free (bytes);
}

/// MARK: - Remove, Clear

extern void
fileServiceRemove (BRFileService fs,
                   const char *type,
                   UInt256 identifier) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) { fileServiceFailedImpl (fs, NULL, NULL, "missed type"); return; };

    // Hex-Encode identifier
    const char *hash = u256hex(identifier);

    sqlite3_status_code status;

    status = sqlite3_bind_text (fs->sdbDeleteStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
    }

    status = sqlite3_bind_text (fs->sdbDeleteStmt, 2, hash, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
    }

    if (SQLITE_DONE != sqlite3_step (fs->sdbDeleteStmt)) {
    }
    sqlite3_reset (fs->sdbDeleteStmt);

}

static void
fileServiceClearForType (BRFileService fs,
                         BRFileServiceEntityType *entityType) {
    const char *type = entityType->type;

    sqlite3_status_code status;

    status = sqlite3_bind_text (fs->sdbDeleteAllTypeStmt, 1, type, -1, SQLITE_STATIC);
    if (SQLITE_OK != status) {
    }

    if (SQLITE_DONE != sqlite3_step (fs->sdbDeleteAllTypeStmt)) {
    }
    sqlite3_reset (fs->sdbDeleteAllTypeStmt);

}

extern void
fileServiceClear (BRFileService fs,
                  const char *type) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) { fileServiceFailedImpl (fs, NULL, NULL, "missed type"); return; };

    fileServiceClearForType(fs, entityType);
}

extern void
fileServiceClearAll (BRFileService fs) {
    size_t typeCount = array_count(fs->entityTypes);
    for (size_t index = 0; index < typeCount; index++)
        fileServiceClearForType (fs, &fs->entityTypes[index]);
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
    if (NULL == entityType) return fileServiceFailedImpl (fs, NULL, NULL, "missed type");

    // Find the entityHandler, for version.
    BRFileServiceEntityHandler *entityHandler = fileServiceEntityTypeLookupHandler (entityType, version);
    if (NULL == entityHandler) return fileServiceFailedImpl (fs,  NULL, NULL, "missed type handler");

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
