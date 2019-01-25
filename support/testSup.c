//
//  testSup.c
//  CoreTests
//
//  Created by Ed Gamble on 1/25/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include <ftw.h>
#include <errno.h>
#include "BRFileService.h"

//static int directoryMake (const char *path) {
//    struct stat dirStat;
//    if (0 == stat  (path, &dirStat)) return 0;
//    if (0 == mkdir (path, 0700)) return 0;
//    return -1;
//}

static int
_remove(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);
    if (rv)
        perror(fpath);
    return rv;
}

static int _rmdir (char *path)
{
    return nftw (path, _remove, 64, FTW_DEPTH | FTW_PHYS);
}

static int
fileServiceTestDone (char *path, int success) {
    struct stat dirStat;

    if (0 == stat  (path, &dirStat)) _rmdir (path);
    return success;
}

///
/// MARK: File Service Tests
///
static int runSupFileServiceTests (void) {
    printf ("==== SUP:FileService\n");

    struct stat dirStat;

    BRFileService fs;
    char *path;
    char *currency = "btc", *network = "mainnet";
    char *type1 = "foo";
    //
    // Try to create a directory that is not writable; expect fileServiceCreate() to fail.
    //
    path = "private";

    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0000)) return 0;

    fs = fileServiceCreate(path, currency, network, NULL, NULL);
    if (NULL != fs) return fileServiceTestDone(path, 0);

    //
    // Create a directory that is writeable; expect fileServiceCreate to succeed.
    //
    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0700)) return 0;

    fs = fileServiceCreate(path, currency, network, NULL, NULL);
    if (NULL == fs) return fileServiceTestDone(path, 0);

    // Confirm the full path exists.
    char fullpath[1024];
    sprintf (fullpath, "%s/%s/%s", path,  currency, network);
    if (0 != stat (fullpath, &dirStat)) return fileServiceTestDone (path, 0);

    // change the fullpath permissions; expect 'defineType' to fail.
    chmod (fullpath, 0000);
    if (1 == fileServiceDefineType(fs, type1, 0, NULL, NULL, NULL, NULL))
        return fileServiceTestDone (path, 0);

    // and can't set the current version on a bad type
    if (1 == fileServiceDefineCurrentVersion(fs, type1, 0))
        return fileServiceTestDone (path, 0);

    // change the permission to allow writing
    chmod (fullpath, 0700);
    if (1 != fileServiceDefineType(fs, type1, 0, NULL, NULL, NULL, NULL))
        return fileServiceTestDone (path, 0);

    if (1 != fileServiceDefineCurrentVersion(fs, type1, 0))
        return fileServiceTestDone (path, 0);

    // Good, finally.
    return fileServiceTestDone(path, 1);
}

int BRRunSupTests (void) {
    printf ("==== SUP\n");
    int success = 1;

    success &= runSupFileServiceTests();

    return success;
}
