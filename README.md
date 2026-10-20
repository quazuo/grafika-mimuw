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
./1_window
```

Windows:
```shell
cd build
./1_window.exe
```

As of writing these steps, running these programs from the `build` directory is essential (except for `1-window`). Programs that use shaders read them at runtime from specific relative paths which break when running the programs from other directories.

## (not so) Frequently Asked Questions

### I'm using nix-shell and these programs don't work!

If you don't wanna mess with your linux installation and wanna use `nix-shell` then there is `shell.nix` file in the root of this repo already waiting for you :)

**Note on Wayland:** The application forces the X11 session backend for compatibility, so make sure you have `xwayland` installed if running on a Wayland compositor.

As I am not a proper NixOS user (I use Arch BTW), feel free to make that file better ;)

\[`shell.nix` and the explanation provided with :heart: by [**wapel37**](https://github.com/wapel37)\]
