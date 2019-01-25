//
//  BRFileService.c
//  Core
//
//  Created by Richard Evers on 1/4/19.
//  Copyright © 2019 breadwallet. All rights reserved.
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
#include "../BRArray.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#if !defined(BRArrayOf)
#define BRArrayOf(type)    type*
#endif

#define FILE_SERVICE_INITIAL_TYPE_COUNT    (5)
#define FILE_SERVICE_INITIAL_HANDLER_COUNT    (2)

static int directoryMake (const char *path) {
    struct stat dirStat;
    if (0 == stat  (path, &dirStat)) return 0;
    if (0 == mkdir (path, 0700)) return 0;
    return -1;
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
    BRFileServiceContext context;
    BRFileServiceVersion version;
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
    const char *pathToType;
    BRArrayOf(BRFileServiceEntityType) entityTypes;
    BRFileServiceContext context;
    BRFileServiceErrorHandler handler;
};

extern BRFileService
fileServiceCreate (const char *basePath,
                   const char *network,
                   const char *currency,
                   BRFileServiceContext context,
                   BRFileServiceErrorHandler handler) {
    // Reasonable limits on `network` and `currency` (ensure subsequent stack allocation works).
    if (strlen(network) > FILENAME_MAX || strlen(currency) > FILENAME_MAX)
        return NULL;

    // Make directory if needed.
    if (-1 == directoryMake(basePath)) return NULL;

    // Require `basePath` to be an existing directory.
    DIR *dir = opendir(basePath);
    if (NULL == dir) return NULL;

    // Create the directory hierarchy
    size_t pathToTypeSize = strlen(basePath) + 1 + strlen(currency) + 1 + strlen(network) + 1;
    char dirPath[pathToTypeSize];

    sprintf (dirPath, "%s/%s", basePath, currency);
    if (-1 == directoryMake(dirPath)) return NULL;

    sprintf(dirPath, "%s/%s/%s", basePath, currency, network);
    if (-1 == directoryMake(dirPath)) return NULL;

    BRFileService fs = calloc (1, sizeof (struct BRFileServiceRecord));

    fs->pathToType = strdup(dirPath);
    array_new (fs->entityTypes, FILE_SERVICE_INITIAL_TYPE_COUNT);

    fileServiceSetErrorHandler (fs, context, handler);

    return fs;
}

extern void
fileServiceRelease (BRFileService fs) {
    size_t typesCount = array_count(fs->entityTypes);
    for (size_t index = 0; index < typesCount; index++)
        fileServiceEntityTypeRelease (&fs->entityTypes[index]);

    free ((char *) fs->pathToType);
    if (NULL != fs->entityTypes) array_free (fs->entityTypes);
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

///
/// MARK: Load
///
static int
fileServiceLoadFailedInternal (BRFileService fs,
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
fileServiceLoadFailedImpl(BRFileService fs,
                          void* bufferToFree,
                          FILE* fileToClose,
                          const char *reason) {
    return fileServiceLoadFailedInternal (fs, bufferToFree, fileToClose,
                                          (BRFileServiceError) {
                                              FILE_SERVICE_IMPL,
                                              { .impl = { reason }}
                                          });
}

static int
fileServiceLoadFailedUnix(BRFileService fs,
                          void* bufferToFree,
                          FILE* fileToClose,
                          int error) {
    return fileServiceLoadFailedInternal (fs, bufferToFree, fileToClose,
                                          (BRFileServiceError) {
                                              FILE_SERVICE_UNIX,
                                              { .unix = { error }}
                                          });
}

static int
fileServiceLoadFailedEntity(BRFileService fs,
                            void* bufferToFree,
                            FILE* fileToClose,
                            const char *type,
                            const char *reason) {
    return fileServiceLoadFailedInternal (fs, bufferToFree, fileToClose,
                                          (BRFileServiceError) {
                                              FILE_SERVICE_ENTITY,
                                              { .entity = { type, reason }}
                                          });
}

extern int
fileServiceLoad (BRFileService fs,
                 BRSet *results,
                 const char *type,
                 int updateVersion) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return fileServiceLoadFailedImpl (fs, NULL, NULL, "missed type");

    BRFileServiceEntityHandler *entityHandlerCurrent = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == entityHandlerCurrent) return fileServiceLoadFailedImpl (fs,  NULL, NULL, "missed type handler");

    DIR *dir;
    struct dirent *dirEntry;

    char dirPath[strlen(fs->pathToType) + 1 + strlen(type) + 1];
    sprintf (dirPath, "%s/%s", fs->pathToType, type);

    char filename[strlen(dirPath) + 1 + 2 * sizeof(UInt256) + 1];

    if (-1 == directoryMake(dirPath) || NULL == (dir = opendir(dirPath)))
        return fileServiceLoadFailedUnix (fs, NULL, NULL, errno);

    // Allocate some storage for entity bytes;
    size_t bufferSize = 8 * 1024;
    uint8_t *buffer = malloc (bufferSize);

    // Process each directory entry.
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            sprintf (filename, "%s/%s", dirPath, dirEntry->d_name);
            FILE *file = fopen (filename, "rb");
            if (NULL == file) return fileServiceLoadFailedUnix (fs, buffer, NULL, errno);


            BRFileServiceVersion version;
            uint32_t bytesCount;

            // read the header version
            BRFileServiceHeaderFormatVersion headerVersion;
            if (1 != fread (&headerVersion, sizeof(BRFileServiceHeaderFormatVersion), 1, file))
                return fileServiceLoadFailedUnix (fs, buffer, file, errno);

            // read the header
            switch (headerVersion) {
                case HEADER_FORMAT_1: {
                    // read the version
                    if (1 != fread (&version, sizeof(BRFileServiceVersion), 1, file) ||
                        // read the checksum
                        // read the bytesCount
                        1 != fread (&bytesCount, sizeof(uint32_t), 1, file))
                        return fileServiceLoadFailedUnix (fs, buffer, file, errno);

                    break;
                }
            }

            // Ensure `buffer` is large enough
            if (bytesCount > bufferSize) {
                bufferSize = bytesCount;
                buffer = realloc (buffer, bufferSize);
            }

            // read the bytes - multiple might be required
            if (bytesCount != fread (buffer, 1, bytesCount, file))
                return fileServiceLoadFailedUnix (fs, buffer, file, errno);

            // All file reading is complete; next read should be EOF.

            // Done with file.
            if (0 != fclose (file))
                return fileServiceLoadFailedUnix (fs, buffer, NULL, errno);

            // We now have everything

            // This will need some later rework.  If a header includes some data, like a checksum,
            // we won't have that value in this context when needed.

            // Do something header specific
            switch (headerVersion) {
                case HEADER_FORMAT_1:
                    // compute the checksum
                    // compare the checksum
                   break;
            }

            // Look up the entity handler
            BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, version);
            if (NULL == handler) return fileServiceLoadFailedImpl (fs,  buffer, NULL, "missed type handler");

            // Read the entity from buffer and add to results.
            void *entity = handler->reader (handler->context, fs, buffer, bytesCount);
            if (NULL == entity) return fileServiceLoadFailedEntity (fs, buffer, NULL, type, "reader");

            // Update restuls with the newly restored entity
            BRSetAdd (results, entity);

            // If the read version is not the current version, update
            if (updateVersion &&
                (version != entityType->currentVersion ||
                 headerVersion != currentHeaderFormatVersion))
                fileServiceSave (fs, type, entity);
        }

    free (buffer);
    closedir (dir);

    return 1;
}

extern void /* error code? */
fileServiceSave (BRFileService fs,
                 const char *type,  /* block, peers, transactions, logs, ... */
                 const void *entity) {     /* BRMerkleBlock*, BRTransaction, BREthereumTransaction, ... */
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return;

    BRFileServiceEntityHandler *handler = fileServiceEntityTypeLookupHandler(entityType, entityType->currentVersion);
    if (NULL == handler) return;

    UInt256 identifier = handler->identifier (handler->context, fs, entity);

    uint32_t bytesCount;
    uint8_t *bytes = handler->writer (handler->context, fs, entity, &bytesCount);

    char filename[strlen(fs->pathToType) + 1 + strlen(type) + 1 + 2*sizeof(UInt256) + 1];
    sprintf (filename, "%s/%s/%s", fs->pathToType, type, u256hex(identifier));

    FILE *file = fopen (filename, "wb");
    if (NULL == file) {

    }

    // Always, always write the header for the currentHeaderFormatVersion

    if (// write the header version
        1 != fwrite(&currentHeaderFormatVersion, sizeof(BRFileServiceHeaderFormatVersion), 1, file) ||
        // then the version
        1 != fwrite (&entityType->currentVersion, sizeof(BRFileServiceVersion), 1, file) ||
        // then the checksum?
        // write the bytesCount
        1 != fwrite(&bytesCount, sizeof (uint32_t), 1, file)) {

    }

    // write the bytes.
    if (bytesCount != fwrite(bytes, 1, bytesCount, file)) {

    }

    if (0 != fclose (file)) {

    }
    free (bytes);
}

extern void
fileServiceRemove (BRFileService fs,
                   const char *type,
                   UInt256 identifier) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return;

    char filename[strlen(fs->pathToType) + 1 + strlen(type) + 1 + 2*sizeof(UInt256) + 1];
    sprintf (filename, "%s/%s/%s", fs->pathToType, type, u256hex(identifier));

    remove (filename);
}

static void
fileServiceClearForType (BRFileService fs,
                         BRFileServiceEntityType *entityType) {
    DIR *dir;
    struct dirent *dirEntry;

    char dirPath[strlen(fs->pathToType) + 1 + strlen(entityType->type) + 1];
    sprintf (dirPath, "%s/%s", fs->pathToType, entityType->type);

    if (-1 == directoryMake(dirPath) || NULL == (dir = opendir(dirPath))) {
        //*error = errno;
        return;
    }

    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG)
            remove (dirEntry->d_name);

    closedir(dir);
}

extern void
fileServiceClear (BRFileService fs,
                  const char *type) {
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == type) return;
    fileServiceClearForType(fs, entityType);
}

extern void
fileServiceClearAll (BRFileService fs) {
    size_t typeCount = array_count(fs->entityTypes);
    for (size_t index = 0; index < typeCount; index++)
        fileServiceClearForType (fs, &fs->entityTypes[index]);

}

extern void
fileServiceDefineType (BRFileService fs,
                       const char *type,
                       BRFileServiceContext context,
                       BRFileServiceVersion version,
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
        context,
        version,
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

        char dirPath[strlen(fs->pathToType) + 1 + strlen(type) + 1];
        sprintf (dirPath, "%s/%s", fs->pathToType, type);

        if (-1 == directoryMake(dirPath)) return;
    }
}

extern void
fileServiceDefineCurrentVersion (BRFileService fs,
                                 const char *type,
                                 BRFileServiceVersion version) {
    // Find the entityType for `type`
    BRFileServiceEntityType *entityType = fileServiceLookupType (fs, type);
    if (NULL == entityType) return;

    // Find the entityHandler, for version.
    BRFileServiceEntityHandler *entityHandler = fileServiceEntityTypeLookupHandler (entityType, version);
    if (NULL == entityHandler) return;

    // We have a handler, therefore it can be the current one.
    entityType->currentVersion = version;
}
