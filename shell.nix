{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/refs/heads/nixos-26.05.tar.gz") {} }:

let
  nixgl = import (fetchTarball "https://github.com/nix-community/nixGL/archive/main.tar.gz") {};
in
# Przekazujemy clangStdenv jako stdenv do mkShell
pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
  buildInputs = with pkgs; [
    cmake
    pkg-config

    # Biblioteka C++ od LLVM
    llvmPackages.libcxx

    # Grafika & Helpery
    libtiff
    freeglut
    libGLU
    libGL
    glew
    glfw
    glm
    mesa

    # Integracja z serwerem wyświetlania (X11 / Wayland)
    wayland
    wayland-protocols
    wayland-scanner
    libxkbcommon

    # Biblioteki systemowe X11
    xorg.libX11
    xorg.libXrandr
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXi
    xorg.libXext

    # nixGL
    nixgl.auto.nixGLDefault
  ];

  shellHook = ''
    # Zapewnienie pełnej widoczności bibliotek podrzędnych dla loaderów dynamicznych
    export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [
      pkgs.wayland
      pkgs.libxkbcommon
      pkgs.libGL
      pkgs.libGLU
      pkgs.glew
      pkgs.glfw
      pkgs.freeglut
      pkgs.xorg.libX11
      pkgs.xorg.libXcursor
      pkgs.xorg.libXrandr
      pkgs.xorg.libXinerama
      pkgs.xorg.libXi
      pkgs.xorg.libXext
    ]}:$LD_LIBRARY_PATH"

    # Wymuszenie nakładki backendu X11 dla GLFW/FreeGLUT
    #export GLFW_PLATFORM=x11

    export WAYLAND_DISPLAY=
    export XDG_SESSION_TYPE=x11

    echo "============================================="
    echo " Grafika MIMUW - Środowisko Nix gotowe!"
    echo " Kompilator: $(clang --version | head -n 1)"
    echo "============================================="

    #exec nixGL $SHELL
    eval "$(cat $(which nixGL) | grep '^export ')"
  '';
}
