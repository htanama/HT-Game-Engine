# HT Game Engine

A custom, lightweight 3D game engine built from first principles in C++ using modern architectural paradigms. The HT Game Engine focuses on systems-level performance, clean scene-graph execution, and explicit memory management.

## 🛠️ Tech Stack & Architecture

* **Windowing & Input:** SDL3 (Simple DirectMedia Layer)
* **Graphics API:** OpenGL 4.6 (Core Profile)
* **Mathematics:** GLM (OpenGL Mathematics)
* **UI / Editor:** Dear ImGui

## 🚀 Active Roadmap (Agile Development)

This project is developed using structured Agile methodologies. Tasks are tracked via a dedicated Kanban framework.

* **Phase 1 [Current]:** Environment Setup, CMake Pipeline, SDL3 Initialization, and Deterministic Game Loop Delta-Timing.
* **Phase 2:** GLM Integration, 3D Coordinate Space Transformations, and Static .OBJ Mesh Loading.
* **Phase 3:** Hierarchical Scene Tree Architecture with recursive parent-child transform propagation.
* **Phase 4:** 3D Screen-to-World Raycasting and Dear ImGui Editor integration.

## 🛠️ Build Instructions

### Prerequisites

* **Compiler:** C++20 compatible compiler (MSVC 19.29+, GCC 11+, or Clang 13+).
* **Build System:** CMake 3.20 or higher.
* **Libraries:** SDL3 must be available on your system.

### Option 1: Visual Studio (Windows)

1. **Open Project:** Launch Visual Studio and select **"Open a local folder"**. Choose the `HT-Game-Engine` root directory.
2. **Configure:** Visual Studio will automatically detect the `CMakeLists.txt` and run the CMake configuration. If it does not start automatically, go to `Project` -> `Configure Cache` -> `HT_Game_Engine`.
3. **Build:** Once the cache generation completes (visible in the Output window), go to the **Build** menu and click **Build All**.
4. **Run:** Select `HT_Game_Engine.exe` from the target dropdown in the top toolbar and press the **Play** button.

### Option 2: Command Line (Linux / macOS / Windows Terminal)

This method is recommended for a clean, automated build process:

```bash
# 1. Create a build directory
mkdir build && cd build

# 2. Configure the project
cmake ..

# 3. Build the project
cmake --build .

# 4. Run the executable
./HT_Game_Engine

```

---

### Pro-Tip for Contributors

This project uses a custom automated **Asset Pipeline**. Every time you build, the `CMakeLists.txt` automatically syncs your `shaders/` directory from the source folder into your build output folder. This ensures your engine always has access to the latest assets without manual copying.


### Option 3: Windows Batch Script (Simple & Direct)

For developers who prefer a transparent, zero-dependency build process on Windows, a custom build.bat script is provided. This script manages environment setup, library linking, and asset synchronization automatically.

    Open Prompt: Search for the "x64 Native Tools Command Prompt for VS 2022" in your Windows Start menu.

    Navigate: Change your directory to the HT-Game-Engine root folder.

    Build: Run the script:
    DOS

    build.bat

    Result: The script will automatically create a build/ directory, compile the engine, copy the required SDL3.dll and shaders/ assets, and generate the HT_Game_Engine.exe ready for execution.

Note: This method is highly recommended if you wish to see exactly how your libraries and source files are being linked by the Microsoft C++ compiler (cl.exe).