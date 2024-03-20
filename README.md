# Beat-Box Application - (ALSA Library)


## Devices & OS
- Devices:
  - Beagle Bone: Opern-source hardware platform (produced by Texas Instrument)
  - Accelerometer: LIS331DLH datasheet attached on Zen Cape Red
  - Joystick: attached on Zen Cape Red
- Operating System: Linux - Debian 11
   
## Folder Sturcture

- `hal/`: Contains all low-level hardware abstraction layer (HAL) modules
- `app/`: Contains all application-specific code. Broken into modules and a main file
- `beat-box-server: Contains all code for web application (front end and backend)

```
  .
  ├── app
  │   ├── include
  │   │   └── badmath.h
  │   ├── src
  │   │   ├── badmath.c
  │   │   └── main.c
  │   └── CMakeLists.txt           # Sub CMake file, just for app/
  ├── hal
  │   ├── include
  │   │   └── hal
  │   │       └── button.h
  │   ├── src
  │   │   └── button.c
  │   └── CMakeLists.txt           # Sub CMake file, just for hal/
  ├── CMakeLists.txt               # Main CMake file for the project
  └── README.md
```  

## Usage

- Install CMake: `sudo apt update` and `sudo apt install cmake`
- When you first open the project, click the "Build" button in the status bar for CMake to generate the `build\` folder and recreate the makefiles.
  - When you edit and save a CMakeLists.txt file, VS Code will automatically update this folder.
- When you add a new file (.h or .c) to the project, you'll need to rerun CMake's build
  (Either click "Build" or resave `/CMakeLists.txt` to trigger VS Code re-running CMake)
- Cross-compile using VS Code's CMake addon:
  - The "kit" defines which compilers and tools will be run.
  - Change the kit via the menu: Help > Show All Commands, type "CMake: Select a kit".
    - Kit "GCC 10.2.1 arm-linux-gnueabi" builds for target.
    - Kit "Unspecified" builds for host (using default `gcc`).
  - Most CMake options for the project can be found in VS Code's CMake view (very left-hand side).
- Build the project using Ctrl+Shift+B, or by the menu: Terminal > Run Build Task...
  - If you try to build but get an error about "build is not a directory", the re-run CMake's build as mentioned above.

## Address Sanitizer

- The address sanitizer built into gcc/clang is very good at catching memory access errors.
- Enable it by uncomment the `fsanitize=address` lines in the root CMakeFile.txt.
- For this to run on the BeagleBone, you must run:
  `sudo apt install libasan6`
  - Without this installed, you'll get an error:   
    "error while loading shared libraries: libasan.so.6: cannot open shared object file: No such file or directory"

## Suggested addons

- "CMake Tools" automatically suggested when you open a `CMakeLists.txt` file
- "Output Colourizer" by IBM 
    --> Adds colour to the OUTPUT panel in VS Code; useful for seeing CMake messages

## Other Suggestions

- If you are trying to build with 3rd party libraries, you may want to consider the 
  build setup suggested at the following link. Specificall, see the part on 
  extracting the BB image to a folder, and then using chroot to run commands like
  `apt` on that image, which allows you to get libraries for the target on the build system.
  https://takeofftechnical.com/x-compile-cpp-bbb/

## Manually Running CMake

To manually run CMake from the command line use:

```shell
  # Regenerate build/ folder and makefiles:
  rm -rf build/         # Wipes temporary build folder
  cmake -S . -B build   # Generate makefiles in build\

  # Build (compile & link) the project
  cmake --build build
```

## Finer Points

- When using the header files in HAL, you'll need to:  
  `#include "hal/myfile.h`  
  This extra "hal/..." helps distinguish the low-level access from the higher-level code.
- One only need to run the CMake build the first time the project loads, and each time the .h and .c file names change, or new ones are added, or ones are removed. This regenerates the `build/Makefile`. Otherwise, just run a normal build (ctrl+shift+B)
- If desired, one could provide an alternative implementation for the HAL modules that provides a software simulation of the hardware! This could be a useful idea if you have some complex hardware, or limited access to some hardware.
