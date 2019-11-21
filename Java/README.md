# Environment

Development is primarily done in a macOS environment (10.14.6 confirmed) but should function in Linux environments as well (untested).

  * XCode (v11.2.1 confirmed)
  * XCode Command Line Tools
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

# A Word About JRE Modules

The build system currently only builds **corenative-jre** and **corecrypto-jre** for the host platform. We are relying on the Gradle native plugins for builds and they do not support building for other platforms at the moment (see [issue](https://github.com/gradle/gradle-native/issues/1031)).

At some point, multiplatform builds will be supported for those core modules; whether that is due to Gradle adding proper support or a different build solution being used is to be determined.
