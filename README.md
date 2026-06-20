<p align="center">
    <a href=""><img src="https://trucksimhub.top/assets/img/SPF-Framework_Logo.svg" alt="Logo SPF Framework" height="263px" /></a>
</p>

<h1 align="center">SPF Framework</h1>

<p align="center">
    <a href="https://github.com/TrackAndTruckDevs/SPF-Framework/releases/latest/" target="_blank" title="SPF Framework releas"><img alt="GitHub Release" src="https://img.shields.io/github/v/release/TrackAndTruckDevs/SPF-Framework"></a>
    <a href="/docs/api" title="Documentation API"><img alt="" src="https://img.shields.io/badge/documentation-API-ffdd00.svg" /></a>
    <a href="/LICENSE" title="SPF Framework license"><img alt="GitHub License" src="https://img.shields.io/github/license/TrackAndTruckDevs/SPF-Framework"></a>
    <a href="https://github.com/TrackAndTruckDevs/SPF-Framework/stargazers" title="Liked it? Starred"><img src="https://img.shields.io/github/stars/TrackAndTruckDevs/SPF-Framework?style=flat&logo=github" alt="Stars" /></a>    
</p>

<p align="center">
    <a href="https://discord.gg/kadd8AQuMt" target="_blank" title="Join our Discord"><img alt="Discord" src="https://img.shields.io/badge/discord-join-7289da?style=flat&logo=discord&logoColor=white"></a>
    <a href="https://www.patreon.com/TrackAndTruckDevs" target="_blank" title="Support us on Patreon"><img alt="Patreon" src="https://img.shields.io/badge/patreon-back us-3404021712?style=flat&logo=patreon"></a>
    <a href="https://youtube.com/@TrackAndTruck" target="_blank" title="Subscribe to our channel"><img alt="Youtube" src="https://img.shields.io/badge/youtube-subscribe-orange?logo=youtube&style=flat"></a>
</p>

<h2 align="center">C++ Framework & Plugin Manager for ATS / ETS2</h2>

  SPF is an advanced C++ based framework that revolutionizes plugin development for SCS Software games, such as American Truck Simulator (ATS) and
  Euro Truck Simulator 2 (ETS2). It acts as a robust middleware layer between the game and plugins, providing a stable C-API that abstracts the
  complexities of game modification and ensures long-term compatibility.

* ### 🎮 For Players

  Enjoy a unified environment to manage all your SPF-compatible plugins. This is your "control center" for DLL plugins. A single interface to
  install, update, configure, and manage all your favorite plugins.

  The more plugins that support SPF, the more convenient your gaming experience becomes.

* ### 🛠️ For Developers

  Stop wasting time with debuggers and memory scanners. SPF allows you to create powerful, feature-rich plugins with unprecedented ease and
  stability. The core philosophy of the framework is to let you focus on logic, not on reverse engineering.

  By joining a growing platform, you ensure your plugin's compatibility with others in the ecosystem and provide users with a familiar, reliable
  interface. We provide the tools—you create incredible functionality.

<h2 align="center">🚀 Key Features</h2>

* #### **✔ Graphics Support:**
  * Automatically detects and integrates with DirectX 11, DirectX 12, and OpenGL, allowing UI to be rendered seamlessly regardless of the game's chosen renderer.

* #### **✔ Automatic UI Generation:**
  * The framework automatically creates windows, convenient settings menus, and key assignments based on your manifest, with full localization support.

* #### **✔ Input Device Support:**
  * Comprehensive handling for keyboard, mouse, standardized gamepads (Xbox via XInput, PlayStation via DirectInput8), and generic DirectInput8 devices (custom gamepads, steering wheels, joysticks). Features advanced keybinding (short/long press, hold, custom consumption).

* #### **✔ Declarative Manifests:**
  * Define your plugin's identity, settings, keybindings, and windows.	

* #### **✔ Signature-Based Hooking:**
  * Safely intercept game functions using byte patterns (signatures).

* #### **✔ Event-Driven Architecture:**
  * Build your plugin's logic around events.

* #### **✔ High-Level APIs:**
  * Interact with complex game systems through clean, ready-to-use interfaces, including:
    * Full Telemetry Access: Receive the entire stream of game telemetry data.
    * Deep Camera Control: Control every aspect of any in-game camera.
    * Virtual Input Simulation: Create virtual gamepads and controllers to send input signals to the game.
    * Access to Console and Game Logs: Execute console commands and subscribe to game log events.
    * Ever-Expanding API: The current set is just the foundation. In the future, we plan to add APIs for managing weather, traffic, game objects, and other systems.

* #### **✔ Stable C-API (ABI Safe):**
  * Guarantees binary compatibility (ABI safety), allowing you to use any compiler and tools without risk of crashes.

* #### **✔ Built-in Utility Modules:**
  * Provides ready-to-use and safe modules for working with configuration files (JSON), localization, logging, and string formatting.

<h2 align="center">🛠️ Architecture & Core Technologies</h2>

 SPF is built upon industry-standard libraries, providing safe and convenient high-level APIs that abstract away the complexity. You can focus on your plugin's logic instead of boilerplate code.

   ✔ UI System (Dear ImGui):
    The entire UI is rendered using the powerful Dear ImGui library. SPF handles the complex setup, rendering loop, and input integration, allowing you to create flexible user interfaces with just a few lines of code. The framework also provides a set of pre-built windows (Logger, Settings, etc.).

   ✔ Hooking Engine (MinHook & Custom Scanner):
    For safe and stable function interception, SPF uses the proven MinHook library combined with a custom signature scanner. This allows for creating hooks that are resilient to game updates, so you don't have to search for memory offsets after every patch.

   ✔ Game Integrations (SCS SDK & more):
    We provide high-level, easy-to-use wrappers for core game technologies. This includes a full interface for the SCS Telemetry SDK and management of input APIs (DirectInput, XInput), saving you from writing complex integration code yourself.

   ✔ Configuration & Data (nlohmann/json):
    All framework and plugin settings are managed through JSON files. We use the powerful **nlohmann/json** librar for all JSON parsing and serialization tasks, exposed via the `ConfigService`.

   ✔ Logging & Formatting ({fmt}):
    All formatted output, especially for the logging system, is powered by the high-performance **{fmt}** library, ensuring fast and safe string formatting across the DLL boundary. 

   ✔ HTTP & API Communication (cpr):
    All external web requests, such as for update checks and statistics, are handled by the modern **C++ Requests (cpr)** library, which provides a simple and powerful interface for HTTP communication.

   ✔ Markdown Rendering (MD4C):
    To provide rich text formatting, SPF uses a proprietary Markdown renderer built directly on the **md4c (Markdown for C) parser**. This custom engine ensures perfect layout flow across different styles, supporting GFM tables, custom inline colors, and integrated clipboard support for code blocks.

   ✔ Image Loading (stb_image):
    For efficient and memory-safe image decoding, SPF integrates the **stb_image** library. This allows plugins to load textures directly from common formats like PNG and JPG into GPU-ready buffers.



<h2 align="center">❤️ Support the Project</h2>

The SPF-Framework is a passion project, developed with the goal of empowering the entire SCS plugin development community. It is, and always will be, free to use. However, its continued development, maintenance, and support require a significant investment of time and effort.

If you find this framework useful and believe in our vision, please consider supporting its development through our Patreon. Your support allows us to dedicate more time to the project, ensuring a steady stream of updates, new features, and a healthy future for the entire ecosystem.

**Become a Patron**

By becoming a Patron, you not only support the further development of the project, but also gain access to a number of exclusive benefits. Join our Patreon with a free subscription to access the community chat, or choose a higher subscription to get early access to testing, direct interaction with the developers, influence future features, and even directly implement your ideas into the framework. Discover the rewards and be part of our journey!

Ready to support the project? You can find our page here: **[patreon.com/TrackAndTruckDevs](https://www.patreon.com/TrackAndTruckDevs)**



<h2 align="center">🚀 Quick Start for Developers</h2>

There are three main ways to start developing a plugin with SPF. Choose the one that best fits your needs.

### Method 1: Use the Template Project (Recommended)

This is the fastest and easiest way to start a new plugin from scratch. This method separates plugin development from the framework's runtime environment.

**Part 1: Developing and Building Your Plugin**
From the [GitHub Releases](https://github.com/TrackAndTruckDevs/SPF-Framework/releases) page, download the template project archive (e.g., `MyPlugin_Template_v1.0.3.zip`). This is a self-contained, ready-to-use CMake project that already includes the necessary C API headers.

Unzip the template project and rename its folder and source files (e.g., from `MyPlugin.cpp` to `YourPlugin.cpp`). Next, **edit** the `CMakeLists.txt` file to change the project name inside it (e.g., from `project(MyPlugin)` to `project(YourPlugin)`). The provided configuration is already set up to build your plugin into a DLL file. You can now open the project in your favorite editor and start implementing your ideas.

**Part 2: Running and Testing Your Plugin in the Game**
To test your plugin in-game, you need the main SPF-Framework runtime (`spf-framework.dll`). You can obtain it in one of two ways: download a pre-built package from the [Download SFP Framework](https://github.com/TrackAndTruckDevs/SPF-Framework/releases), or build it yourself (see the **Method 3: Build from Source** section).

Install the framework using the `spf-framework.exe` installer or by manually copying the contents of the `manualInstall` folder (`spf-framework.dll` and the `spfAssets` and `spfPlugins` folders) to your game's `.../bin/win_x64/plugins` directory.

Next, take your compiled plugin's DLL (e.g., `MyPlugin.dll`) and place it inside the dedicated SPF-plugins folder, creating a sub-directory for your plugin like so:
```
.../bin/win_x64/plugins/
└───spfPlugins/
    └───MyPlugin/
        └─── MyPlugin.dll
```
Now you can launch the game. The SPF-Framework will discover and load your plugin.

### Method 2: Integrate the API into Your Own Project

This method is for experienced developers who want to integrate SPF support into an existing project that uses a custom build system.

**Part 1: Developing and Building Your Plugin**
You have two primary options to obtain the SPF C API headers for compilation:

**Option A: Manual Download**
This method is suitable if you are not using CMake or want to integrate the API into your own custom build system.

1.  **Download the API:** From the [GitHub Releases](https://github.com/TrackAndTruckDevs/SPF-Framework/releases) page, download the archive with API headers (e.g., `SPF_API_v1.0.3.zip`).
2.  **Unpack:** Unpack the archive to a suitable location within your project (e.g., into a `vendor/spf_api` folder).
3.  **Include Headers:** In your build system's settings (e.g., in Visual Studio project properties), add the path to this folder in the Include Directories.

After this, you will be able to include the API in your code, for example: `#include <SPF_Plugin.h>`. You only need these header files to compile your plugin.

**Option B: Automated Fetch with CMake**
If your project uses CMake, you can leverage `FetchContent` to automatically make the SPF API headers available. This will download the entire framework repository, but a special `INTERFACE` target allows you to link only to the necessary API headers.

Add the following to your `CMakeLists.txt`:
```cmake
include(FetchContent)

# 1. Declare the SPF-Framework repository
FetchContent_Declare(
    spf_framework
    GIT_REPOSITORY https://github.com/TrackAndTruckDevs/SPF-Framework.git
    GIT_TAG        main # It is recommended to use a specific release tag, e.g., v1.0.0
)

# 2. Make the framework's targets available
FetchContent_MakeAvailable(spf_framework)

# 3. Create your own plugin library
add_library(MyPlugin SHARED MyPlugin.cpp MyPlugin.hpp)

# 4. Link your plugin to the API headers
# This provides the correct include paths to your target.
target_link_libraries(MyPlugin PRIVATE spf_plugin_api)
```
After this setup, you can include the API headers directly in your C++ code. The include paths are resolved automatically. For example:
```cpp
#include <SPF_Plugin.h>
#include <SPF_Logger_API.h>
```
This approach provides automated dependency management and ensures you can only include the public C-API headers, preventing accidental dependencies on the framework's internal C++ code.

**Part 2: Running and Testing Your Plugin in the Game**
To test your plugin in-game, you need the main SPF-Framework runtime (`spf-framework.dll`). You can obtain it in one of two ways: download a pre-built package from the [Download SFP Framework](https://github.com/TrackAndTruckDevs/SPF-Framework/releases), or build it yourself (see the **Method 3: Build from Source** section).

Install the framework using the `spf-framework.exe` installer or by manually copying the contents of the `manualInstall` folder (`spf-framework.dll` and the `spfAssets` and `spfPlugins` folders) to your game's `.../bin/win_x64/plugins` directory.

Next, take your compiled plugin's DLL (e.g., `MyPlugin.dll`) and place it inside the dedicated SPF-plugins folder, creating a sub-directory for your plugin like so:
```
.../bin/win_x64/plugins/
└───spfPlugins/
    └───MyPlugin/
        └─── MyPlugin.dll
```
Now you can launch the game. The SPF-Framework will discover and load your plugin.

### Method 3: Build from Source (Advanced)

This method is for developers who want to work with the latest framework code, modify it, or contribute to the project.

Clone the entire repository. Create a new sub-directory for your plugin inside the `/plugins` folder (you can copy the existing `MyPlugin` to use as a starting point). Add the directory of your new plugin to the `CMakeLists.txt` file in the `/plugins` folder. When you compile the project, both your plugin and the framework will be built together from the source code, allowing for deep integration and easy debugging of all components.




<h2 align="center">⚙️ Build Instructions</h2>

This project uses a modern CMake setup with `FetchContent` to automatically download and manage all dependencies (ImGui, MinHook, etc.). You do not need to install them manually.

### Prerequisites

To build the project, you will need **CMake** version `3.16` or newer and **Git** to download dependencies. You will also need a **C++20 compliant compiler**. The build script is pre-configured with the necessary linker options for **MSVC (via Visual Studio)** and **Clang**, ensuring the correct functions are exported for the game.

### Building the Project

First, you need to **clone the repository**. Execute the following commands:
```bash
git clone https://github.com/TrackAndTruckDevs/SPF-Framework.git
cd spf-framework
```

Next, **configure CMake** to generate the project files. From the root of the repository, run the command for your desired build system.

**For Visual Studio (Recommended)**
```bash
cmake -B build -G "Visual Studio 17 2022"
```

**For Ninja**
```bash
cmake -B build -G "Ninja"
```

**💡 Tip: Automatic Deployment**
You can configure CMake to automatically copy the compiled DLL and resource files directly to your game's plugin folder. This can be achieved in several ways. The recommended method is to add the `GAME_PLUGINS_DIR` variable during configuration on the command line, for example:
```bash
cmake -B build -G "Visual Studio 17 2022" -D GAME_PLUGINS_DIR="C:/Path/to/your/game/bin/win_x64/plugins"
```
Alternatively, you can change the value of the `GAME_PLUGINS_DIR` variable in the `cmake-gui` graphical interface. You can also directly edit the `CMakeLists.txt` file by modifying the `set(GAME_PLUGINS_DIR ...)` line to replace the default path with your desired path. For example:
```cmake
set(GAME_PLUGINS_DIR "C:/Path/to/your/game/bin/win_x64/plugins" CACHE PATH "Path to the game's plugins directory")
```

Finally, once configuration is complete, **build the project** by compiling the source code:
```bash
cmake --build build --config Release
```

### After Building

After a successful build, the compiled framework DLL, named `spf-framework.dll`, will be located in the `build/Release` directory. If you configured `GAME_PLUGINS_DIR`, this DLL and the required `spfAssets` assets folder (containing localizations) will have been automatically copied to your game's plugins directory.



<h2 align="center">🎮 Usage (For Users)</h2>

This section guides you on how to install and use the SPF-Framework and SPF-compatible plugins.

### Installing the SPF-Framework

To install the SPF-Framework, first obtain the latest framework release package (e.g., `SPF-Framework_v1.0.3.zip`) [Download SFP-Framework](https://github.com/TrackAndTruckDevs/SPF-Framework/releases). Install the framework using the `spf-framework.exe` installer or by manually copying the contents of the `manualInstall` folder (`spf-framework.dll` and the `spfAssets` and `spfPlugins` folders) to your game's `.../bin/win_x64/plugins` directory. For details, read the `readme.txt` which you will find in the downloaded archive.

### Activating the Framework in Game

Launch American Truck Simulator or Euro Truck Simulator 2. Once in-game, press the **Delete** key (this is the default hotkey) to open the SPF-Framework window.

### Installing SPF-Compatible Plugins

If you have an **SPF-compliant plugin** that you want to add to the framework, go to your game's plugins directory: `[Game Root]\bin\win_x64\plugins\spfPlugins\`. Within this plugins folder, create a new subfolder with the **name of your plugin** (e.g. `MyAwesomePlugin`). Then copy your **plugin's DLL** (e.g. `MyAwesomePlugin.dll`) and any other related files (e.g. `localization` folders) into this newly created subfolder.

For example, a typical plugin installation structure within your game's directories might look like this:

```
[Game Root Directory]
└───bin
    └───win_x64
        └───plugins
            │   spf-framework.dll
            │
            ├───spfAssets
            │   └───localization
            │           en.json
            │
            └───spfPlugins
                ├───ExamplePlugin
                │   │   ExamplePlugin.dll
                │   │
                │   └───localization
                │           en.json
                │           uk.json
                │
                └───MyPlugin
                        MyPlugin.dll
```


<h2 align="center">🎓 Examples & Documentation</h2>

To see a complete, working example of a plugin that uses many of the framework's features, check out the **ExamplePlugin** located in the `/plugins/ExamplePlugin` directory of this repository.

For detailed documentation on each specific API (Camera, UI, Telemetry, etc.), please refer to the documents in the `/docs/api` directory.




<h2 align="center">🧩 Plugins Built with SPF</h2>

This section features community-developed plugins that are built on the SPF-Framework.

* [SPF_CabinWalk](https://github.com/TrackAndTruckDevs/SPF_CabinWalk.git) - ***A plugin for American Truck Simulator and Euro Truck Simulator 2 that allows you to unchain the camera from the driver's seat and freely walk around your truck's cabin***

* [SPF_FrontalBlindspotViewer](https://github.com/TrackAndTruckDevs/SPF_FrontalBlindspotViewer.git) - ***A plugin for American Truck Simulator and Euro Truck Simulator 2 that provides a smooth, animated camera movement to let you easily see traffic lights when they are obstructed by your truck's A-pillar.***

* [SPF_RedLightCameraPlugin](https://github.com/TrackAndTruckDevs/SPF_RedLightCameraPlugin.git) - ***His plugin automatically takes a screenshot from a unique, customized camera angle every time you get a ticket in ATS and ETS2 for running a red light.***

* [PWE Overlay](https://github.com/Marcinekk/PWE-Overlay.git) - ***Advanced web-based overlay for ATS & ETS2 built on SPF-Framework and WebView2. Features real-time telemetry, custom bank economy hooks, and Logitech G27 LED support.***

* [SPF_ConsoleCommandHotkeys](https://github.com/TrackAndTruckDevs/SPF_ConsoleCommandHotkeys.git) - ***A plugin for ATS & ETS2 to execute any console command via hotkeys and cycle sequences. Features an in-game manager UI.***

* [SPF_ConvoyChatMessaging](https://github.com/TrackAndTruckDevs/SPF_ConvoyChatMessaging.git) - ***A reference plugin for ATS & ETS2 to intercept and programmatically send chat messages in Convoy mode. Demonstrates advanced signature scanning and function hooking.***

* [SPF_MapOrigin](https://github.com/TrackAndTruckDevs/SPF_MapOrigin.git) - ***Identify map sector origins and detect seams between map mods in ATS/ETS2 using SPF-Framework.***
---
**Are you a developer who has created a plugin using SPF?** We would love to feature your work here. To have your plugin added to this list, please open an issue or a pull request on our GitHub repository and provide a link to your project.



<h2 align="center">🤝 Contributing</h2>

We welcome contributions from the community! Whether it's reporting a bug, suggesting a new feature, or writing code, your help is appreciated.

### Reporting Bugs

If you encounter a bug, please open a **new issue** on our [GitHub Issues](https://github.com/TrackAndTruckDevs/SPF-Framework/issues) page.

To help us resolve the issue quickly, please include as much detail as possible in your report, such as a clear title, the framework and game version, steps to reproduce the bug, and any relevant error messages or log files (which can be found in `.../spfAssets/logs/`).

### Suggesting Features

If you have an idea for a new feature or an improvement to an existing one, please open a **new issue** on our [GitHub Issues](https://github.com/TrackAndTruckDevs/SPF-Framework/issues) page. Please use a clear title and provide a detailed description of the feature and why it would be beneficial.

### Contributing Code

To contribute code, we recommend you first **fork the repository** and **create a new branch** for your work (e.g., `feature/new-camera-mode` or `fix/crash-on-load`). Once you have made your changes, ensuring you follow the project's existing coding style, please test them thoroughly. Finally, **submit a pull request** to the main repository with a clear description of the changes you have made.

We will review your contribution as soon as possible. Thank you for helping us improve the SPF-Framework!



<h2 align="center">📞 Community & Support</h2>

We love to connect with our community! Join our official **Discord Server** for technical support and development discussions: [**https://discord.gg/kadd8AQuMt**](https://discord.gg/kadd8AQuMt)

Find plugin demonstrations, tutorials, and project updates on our YouTube Channel at [Track'n'Truck](https://www.youtube.com/@TrackAndTruck).

You can also join our **Patreon community** [Patreon community](https://www.patreon.com/TrackAndTruckDevs) with a free subscription! There you'll have access to our public chat, **"The Dispatch,"** where you can get quick answers to your questions, stay updated on project news, and connect with other enthusiasts.

For discussions on official game forums, visit the **SCS Software Forum** at [forum.scssoft.com](https://forum.scssoft.com/).

For technical issues, bug reports, and feature requests, please use our [GitHub Issues](https://github.com/TrackAndTruckDevs/SPF-Framework/issues) page, as detailed in the [Contributing](#contributing) section.



<h2 align="center">📝 License</h2>

This project is licensed under the Apache License, Version 2.0. See the [LICENSE](LICENSE) file for the full license text and details.



<h2 align="center">🙏 Acknowledgements</h2>

This project would not be possible without the incredible work of the open-source community. We extend our heartfelt thanks to the creators and maintainers of the following essential libraries, which are at the core of the SPF-Framework:

*   **[Dear ImGui](https://github.com/ocornut/imgui)**: For the flexible and powerful immediate-mode UI system.
*   **[MinHook](https://github.com/TsudaKageyu/minhook)**: For the robust and reliable hooking engine.
*   **[{fmt}](https://github.com/fmtlib/fmt)**: For modern, safe, and efficient string formatting.
*   **[nlohmann/json](https://github.com/nlohmann/json)**: For easy and powerful JSON manipulation.
*   **[cpr (C++ Requests)](https://github.com/libcpr/cpr)**: For handling all external web requests with a clean, modern interface.
*   **[md4c](https://github.com/mity/md4c)**: For providing fast and lightweight Markdown rendering within the UI.
*   **[stb_image](https://github.com/nothings/stb)**: For reliable and easy image uploads.
*   **[zlib](https://github.com/madler/zlib)**: For data compression, used as a dependency by other core components.
*   **[SCS SDK](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry)**: For providing the official telemetry interface that makes this all possible.

We are also deeply grateful to the **[hry-core](https://github.com/Hary309/hry-core)** project, which served as a significant architectural inspiration for this framework.
