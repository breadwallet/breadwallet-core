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

static int runSupFileServiceTests (void) {
    printf ("==== SUP:FileService\n");

    struct stat dirStat;

    BRFileService fs;
    char *path;
    //
    // Try to create a directory that is not writable; expect fileServiceCreate() to fail.
    //
    path = "private";

    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0000)) return 0;

    fs = fileServiceCreate(path, "mainet", "btc", NULL, NULL);
    if (NULL != fs) {
        if (0 == stat  (path, &dirStat)) _rmdir (path);
        return 0;
    }

    //
    // Create a directory ths is writeable; expect fileServiceCreate to succeed.
    //
    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0700)) return 0;

    fs = fileServiceCreate(path, "mainet", "btc", NULL, NULL);
    if (NULL == fs) {
        if (0 == stat  (path, &dirStat)) _rmdir (path);
        return 0;
    }

    // Confirm the full path exists.
    char fullpath[1024];
    sprintf (fullpath, "%s/%s/%s", path,  "mainnet", "btc");
    if (0 != stat  (path, &dirStat)) {
        if (0 == stat  (path, &dirStat)) _rmdir (path);
        return 0;
    }

    _rmdir (path);
    return 1;
}

int BRRunSupTests (void) {
    printf ("==== SUP\n");
    int success = 1;

    success &= runSupFileServiceTests();

    return success;
}
