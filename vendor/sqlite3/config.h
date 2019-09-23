#ifndef Sqlite3Config_h
#define Sqlite3Config_h

#ifdef __APPLE__
# include <TargetConditionals.h>
#endif

//
// Feature flags: see https://sqlite.org/compile.html
//

#define NDEBUG                                          1

#define SQLITE_ENABLE_API_ARMOR                         1
#define SQLITE_ENABLE_MEMORY_MANAGEMENT                 1
#define SQLITE_ENABLE_BATCH_ATOMIC_WRITE                1

#define SQLITE_THREADSAFE                               1 /* Allow: SQLITE_CONFIG_SERIALIZED */
#define SQLITE_USE_URI                                  1
#define SQLITE_SECURE_DELETE                            1

#define SQLITE_OMIT_AUTORESET                           1
#define SQLITE_OMIT_LOAD_EXTENSION                      1

#if defined(__ANDROID__)

    // Inspired by AOSP platform_external_sqlite/dist/Android.bp at
    // tag: android-6.0.1_r81

    #define HAVE_USLEEP                                 1
    #define HAVE_STRCHRNUL                              0
    #define HAVE_MALLOC_H                               1
    #define HAVE_MALLOC_USABLE_SIZE                     1

    #define USE_PREAD64                                 1

    #define fdatasync                                   fdatasync

    #define SQLITE_HAVE_ISNAN                           1

    #define SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT           1048576
    #define SQLITE_DEFAULT_FILE_PERMISSIONS             0600

    #define SQLITE_TEMP_STORE                           3
    #define SQLITE_POWERSAFE_OVERWRITE                  1

#elif defined (__APPLE__)

    // Derived from 'PRAGMA compile_options;' for Apple SQLite
    // version 3.24.0

    #define SQLITE_HAVE_ISNAN                           1

    #define SQLITE_DEFAULT_CACHE_SIZE                   128
    #define SQLITE_DEFAULT_CKPTFULLFSYNC                1
    #define SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT           32768
    #define SQLITE_DEFAULT_SYNCHRONOUS                  2
    #define SQLITE_DEFAULT_WAL_SYNCHRONOUS              1

    #define SQLITE_MAX_LENGTH                           2147483645
    #define SQLITE_MAX_MMAP_SIZE                        20971520
    #define SQLITE_MAX_VARIABLE_NUMBER                  500000
    #define SQLITE_STMTJRNL_SPILL                       131072

#else

    #error "Unsupported target"

#endif

#endif // Sqlite3Config_h
