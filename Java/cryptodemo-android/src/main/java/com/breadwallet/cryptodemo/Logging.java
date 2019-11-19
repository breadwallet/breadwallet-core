package com.breadwallet.cryptodemo;

import android.os.Build;
import android.support.annotation.Nullable;
import android.util.Log;

import java.lang.String;
import java.util.logging.Formatter;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

/**
 * Helper class for configuring logging to logcat.
 *
 * The default Android {@link Handler} used {@link Log#isLoggable(String, int)} before determining if
 * a record is logged. This class instead installs a hanlder that does *NOT* factor that into its
 * determination on whether to log or not.
 */
class Logging {

    private static final String TAG = Logging.class.getName();

    static void initialize(@Nullable Level minimumLevel) {
        // clear out the standard config
        LogManager.getLogManager().reset();

        // add the new permissive handler
        Logger rootLogger = Logger.getLogger("");
        rootLogger.addHandler(new LoggingHandler());

        // set the new root (and child) minimum log level
        rootLogger.setLevel(Level.ALL);
    }

    static class LoggingHandler extends Handler {

        static final String TAG_SEPARATOR = ".";

        // android.util.Log requires tags of length 23 or smaller for up to and including API level 23
        static final int MAX_TAG_LENGTH = Build.VERSION.SDK_INT <= Build.VERSION_CODES.M ? 23 : Integer.MAX_VALUE;

        static final Formatter THE_FORMATTER = new Formatter() {
            @Override
            public String format(LogRecord record) {
                String message = record.getMessage();

                Throwable throwable = record.getThrown();
                if (throwable != null) {
                    message += Log.getStackTraceString(throwable);
                }

                return message;
            }
        };

        static int logcatLevel(Level level) {
            int logcatValue, julValue = level.intValue();

            if (julValue >= Level.SEVERE.intValue()) {
                logcatValue = Log.ERROR;

            } else if (julValue >= Level.WARNING.intValue()) {
                logcatValue = Log.WARN;

            } else if (julValue >= Level.INFO.intValue()) {
                logcatValue = Log.INFO;

            } else {
                logcatValue = Log.DEBUG;
            }

            return logcatValue;
        }

        static String logcatName(String name) {
            if (name == null || name.isEmpty()) {
                return "<empty>";
            }

            int length = name.length();
            if (length <= MAX_TAG_LENGTH) {
                return name;
            }

            // if TAG_SEPARATOR isn't found, index will be 0
            int index = name.lastIndexOf(TAG_SEPARATOR) + 1;
            if (length - index > MAX_TAG_LENGTH) {
                return name.substring(length - MAX_TAG_LENGTH);

            } else {
                return name.substring(index);
            }
        }

        LoggingHandler() {
            setFormatter(THE_FORMATTER);
        }

        @Override
        public void publish(LogRecord record) {
            int level = logcatLevel(record.getLevel());
            String tag = logcatName(record.getLoggerName());

            // Don't check `Log.isLoggable`, as that has its level set to
            // INFO (by default, typically) and is not easily changed

            try {
                Log.println(level, tag, getFormatter().format(record));

            } catch (RuntimeException e) {
                Log.e(TAG, "Failed to print record", e);
            }
        }

        @Override
        public void flush() {
            // not necessary for android.util.Log
        }

        @Override
        public void close() throws SecurityException {
            // not necessary for android.util.Log
        }
    }
}
