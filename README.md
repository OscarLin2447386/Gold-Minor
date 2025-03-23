# Gold Miner

A retro-style pixel mining game developed in **C++** using the **EasyX graphics library**.  
Designed as a solo project for **portfolio showcase** and **technical interview preparation**.



## Game Preview

| Login | Main Menu | Theme Library |
|-------|-----------|---------------|
| ![Login](./preview/login.png) | ![Menu](./preview/menu.png) | ![Theme](./preview/theme.png) |

| Leaderboard | Settings | Game Instructions |
|-------------|----------|-------------------|
| ![Leaderboard](./preview/leaderboard.png) | ![Settings](./preview/setting.png) | ![Instructions](./preview/instruction.png) |

---

## Tech Stack

| Technology | Description |
|------------|-------------|
| `C++` | Core game logic and file I/O |
| `EasyX` | 2D graphics rendering and input handling |
| `.mp3` | Background music and sound effects |
| `Custom Assets` | Pixel-style UI and character design |



## Project Structure

GoldMiner/
├── GoldMiner.sln               # Visual Studio solution file
├── preview/                    # Screenshots for README
├── .vs/                        # Visual Studio config (ignored)
├── Debug/                      # Build output (ignored)
├── x64/                        # Build output (ignored)
├── assets/                     # Game assets
│   ├── images/                 # Image resources (optional)
│   └── *.mp3                   # Sound effects and background music
├── src/                        # Source code
│   ├── main.cpp                # Game entry point
│   ├── tools.cpp / tools.h     # Utility functions (e.g., UI helpers, math)
│   └── userdata.cpp / userdata.h # User data handling (login, leaderboard)
├── font/                       # Custom fonts used in UI
├── EasyText/                   # EasyX support or config files
├── GoldMiner.vcxproj           # Visual Studio project file
├── GoldMiner.vcxproj.filters   # VS filters (folders in Solution Explorer)
└── GoldMiner.vcxproj.user      # User-specific settings (ignored)




## Features

- 🎮 Keyboard-controlled mining gameplay (`↓` to release hook)
- 🔐 Login & account system with local data storage
- 🎵 Background music & sound effects (toggle on/off)
- 🎨 Multiple game skins/themes
- 🏆 Local leaderboard (with persistent scores)
- 🕹️ Game timer + score target system for level progression



## How to Run

1. Open `GoldMiner.sln` in Visual Studio (Windows only)
2. Make sure **EasyX** is properly installed
3. Build and run `main.cpp`
4. Use keyboard and mouse to interact with the game



## Resume-Friendly Summary

- Built a complete 2D pixel-style mining game using C++ and EasyX from scratch.
- Implemented user authentication, multi-screen navigation, and local file storage.
- Designed modular architecture: login, menu, skin selection, scoreboard, and gameplay loop.
- Integrated sound effects and theme-based UI for immersive user experience.



## 🚧 Future Improvements
 Power-ups (e.g., bombs, speed boost)
 Level design with obstacles
 SQLite or cloud-based leaderboard
 Replace EasyX with modern frameworks (e.g., SFML, SDL)