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

## Future Enhancements

- Replace developer art
- Add multiplayer
- Add more animations
- Add configuration options
- Add sound effects

## License

This project is open source and available for modification under the MIT License.

