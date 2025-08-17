# WIP - A System for Impaired Vision Assistance

This project uses **C++ with OpenGL, GLM, and Boost Interprocess** for graphics and shared memory communication.  
It is designed to run on **Windows** (tested with MSYS2/MinGW).

This half of the code, since it runs on a windows PC, the other half is supposed to run on an edge device like a raspberry or preferably a Jetson Nano

---

## Prerequisites

⚠️ **Compiler Disclaimer**

- Check your compiler before installing dependencies:  
  ```sh
  g++ --version

- Install using pacman 
    ```sh
    pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-boost \
          mingw-w64-ucrt-x86_64-glm \
          mingw-w64-ucrt-x86_64-freeglut \
          mingw-w64-ucrt-x86_64-pthread

- Optional if missing make and cmake
    ```sh
    pacman -S mingw-w64-ucrt-x86_64-make \
            mingw-w64-ucrt-x86_64-cmake
