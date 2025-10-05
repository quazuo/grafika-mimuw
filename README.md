## How to build

```shell
git clone --recursive https://github.com/quazuo/grafika-mimuw
cd grafika-mimuw
cmake -B build
cmake --build build
```

If all goes well, these steps should be enough. However, there might be some errors due to missing packages:

1. `Failed to find wayland-scanner`

   In case of this error, please refer to the [glfw compilation guide](https://www.glfw.org/docs/latest/compile_guide.html#compile_deps_wayland). On most distributions of Linux this should be enough:

    ```shell
    sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
    ```
   
## How to run

Running the above steps generates all executables inside the `build` directory. For example, to run `1-window`:

Linux:
```shell
cd build
./1-window
```

Windows:
```shell
cd build
./1-window.exe
```

As of writing these steps, running these programs from the `build` directory is essential (except for `1-window`). Programs that use shaders read them at runtime from specific relative paths which break when running the programs from other directories.
