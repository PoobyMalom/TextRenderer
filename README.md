# TextRenderer

TextRenderer is a C++ project designed for rendering text glyphs using custom shaders. The project converts TrueType font glyph data into graphical representations, allowing for high-quality text rendering in applications and games.

## Features

- **Glyph Rendering**: Draws glyphs based on Bezier curve data extracted from TrueType fonts.
- **Shader Integration**: Uses shaders for rendering text, offering flexibility and high performance.
- **Flexible Setup**: Designed to work with both OpenGL and Vulkan APIs.
- **Customizable**: Easily extendable for various text rendering needs and optimizations.

## Project Structure

- **`include/`**: Contains header files for font data processing and rendering.
- **`src/`**: Contains source files for font data processing, rendering, and shader management.
- **`fonts/`**: Includes sample TrueType font files used for glyph rendering.
- **`main.cpp`**: Entry point for the application, sets up rendering and shader contexts.
- **`makefile`**: Build configuration for compiling the project.

## Setup

### Prerequisites

- **C++ Compiler**: Ensure you have a C++ compiler installed (e.g., GCC, Clang).
- **OpenGL/Vulkan**: Depending on your choice, make sure the relevant libraries are installed.
- **GLSL Shader Compiler**: For compiling shader code.

### Building

1. Clone the repository:
   ```bash
   git clone https://github.com/PoobyMalom/TextRenderer.git
   cd TextRenderer

2. Build the project using the provided makefile
    ```bash
    make test
3. Running the project using the binary file
    ```bash
    ./test
