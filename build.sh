#!/bin/bash
set -e
SDL_LIBS=$(pkg-config --cflags --libs sdl3)
Flags="-g -O0 -Wall -Wextra -fno-rtti -fno-exceptions -std=c++20"
Libs="$SDL_LIBS"
echo "Build main.cpp ..."
clang++ main.cpp grahics.cpp maths.cpp -o main $Flags $Libs
echo "Terminé. Run avec ./main"
