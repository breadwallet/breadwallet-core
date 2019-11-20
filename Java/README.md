# Environment

Development is primarily done in a macOS environment (10.14.6 confirmed) but should function in Linux environments as well (untested).

  * Android Studio (v3.4.2 confirmed)
  * Android SDK Platform installed (Android 9.0 SDK Platform confirmed)
  * LLDB, CMake and NDK installed

# Structure

The Java language binding is broken down into three major components:
* **corenative:** contains a thin layer exposing the underlying C core functionality (via JNA)
* **crypto:** contains the BlockchainDB client and core interfaces
* **corecrypto:** contains the implementation of the **crypto** interfaces using the **corenative** primitives

For the **corenative** and **corecrypto** projects, there are bindings for both Android and JRE that can be used for the appropriate platform.

In addition, there is a **cryptodemo-android** project that demonstrates how the **corecrypto-android** module can be used to interact with a wallet.

# Building with Android Studio

1. Launch Android Studio
2. Select **Open an existing Android Studio project**
3. Select the **Java** directory
4. Build the project via the menu (**Build** -> **Make Project**)

# Building with Gradle

1. Launch your favourite terminal application
2. From the **Java** directory, run your desired gradle task (ex: *./gradlew assemble*)

# Build outputs

Build outputs can be found under the **build** subdirectory of the individual sub-projects (ex: *./corenative-android/build*).

# Deprecated

The **Core** and **CoreDemo** sub-projects are deprecated and will be removed. They are no longer sound and supported. These projects should not be depended on; migrate to **corecrypto-jre** or **corecrypto-android** in **Core's** place.
