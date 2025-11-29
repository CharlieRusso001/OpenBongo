# OpenBongo - Bongo Cat Desktop Widget

<div align="center">
  <img src="media/body-devartcat2.png" alt="Bongo Cat" style="width: 150%; max-width: 600px; display: block; margin: 0 auto;">
</div>

<div align="center">
  <a href="https://discord.gg/TVw6h5TBqJ">
    <img src="https://img.shields.io/badge/Discord-Join%20Server-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Join Discord">
  </a>
</div>

A desktop widget inspired by the Bongo Cat Steam game. The cat reacts to your keyboard presses by punching the taskbar/desktop.

# NOTE: MACOS SUPPORT THEORETICALLY WORKS BUT IT HAS NOT BEEN COMPILED OR TESTED YET

## Features

- 🐱 Simple Bongo Cat widget (currently using placeholder shapes)
- ⌨️ Global keyboard hook - reacts to all key presses
- 🖱️ Click to trigger animation
- 🪟 Always-on-top window
- 🎨 Easy to replace with custom art later

## Building

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- SFML 2.5 or higher

#### macOS
```bash
brew install sfml
```

### Building the Project

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Running

#### Windows
Run `OpenBongo.exe` from the build directory.

#### macOS
You may need to grant accessibility permissions:
1. Go to System Preferences → Security & Privacy → Privacy → Accessibility
2. Add Terminal (or your IDE) to the allowed apps
3. Run the application

## Current Implementation

- **Body**: Square shape (will be replaced with art)
- **Arms**: Rectangle shapes (will be replaced with art)
- **Animation**: Arms move down when keys are pressed
- **Window**: Small, always-on-top, draggable window

## Third-Party Dependencies

- **SFML** (Simple and Fast Multimedia Library)  
  - Used for the main window, rendering, and input handling.  
  - License: zlib/libpng

- **Crow** (`third_party/crow`)  
  - C++ HTTP microframework used for the embedded local web server that serves the HTML/JS settings UI.  
  - License: BSD-3-Clause  
  - Recommended setup (from the project root):
    ```bash
    git clone https://github.com/CrowCpp/Crow.git third_party/crow
    ```

- **webview** (`third_party/webview`)  
  - Lightweight cross‑platform webview library used to display the HTML settings UI in a native window.  
  - License: MIT  
  - Recommended setup (from the project root):
    ```bash
    git clone https://github.com/webview/webview.git third_party/webview
    ```

## Cat Pack Credits

Built-in and example cat packs included in this repository:

- **Dev Art Cats** (`catpacks/DevArt`)  
  - Charlie

- **Scribble Cats** (`catpacks/DarkScribbleCat`)  
  - Xan

If you make your own catpacks please create a pull request or reach out on Discord and we will add it to the repository.

## Future Enhancements

- Add multiplayer (Not soon)
- Add more animations
- Add configuration options
- Add sound effects

## License

This project is open source and available for modification under the MIT License.

