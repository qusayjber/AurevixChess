# ♟️ AurevixChess

<p align="center">
  <strong>A Modern Native Chess Experience Powered by C++</strong>
</p>

<p align="center">
  <em>Play. Think. Analyze. Improve.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/SFML-2.6-8CC445?style=for-the-badge&logo=sfml&logoColor=white" alt="SFML">
  <img src="https://img.shields.io/badge/CMake-Build-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows">
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="MIT License">
</p>

<p align="center">
  <a href="#-overview">Overview</a> •
  <a href="#-features">Features</a> •
  <a href="#-chess-engine">Engine</a> •
  <a href="#-game-modes">Modes</a> •
  <a href="#-architecture">Architecture</a> •
  <a href="#-build">Build</a> •
  <a href="#-roadmap">Roadmap</a>
</p>

---

## ♟️ Overview

**AurevixChess** is a native desktop chess application built from the ground up with **C++20 and SFML**.

It combines a complete chess rules implementation, a native chess search engine, multiple difficulty levels, position-aware hints, analysis tools, chess clocks, game management and a modern desktop interface.

The project is designed around one principle:

> **Chess should feel like a complete software product, not a university demo.**

AurevixChess is completely playable offline and does not depend on a game engine or online chess service.

---

## ✨ Features

### ♟️ Complete Chess Rules

AurevixChess provides a complete rules-oriented chess foundation:

* Legal move generation
* Check detection
* Checkmate
* Stalemate
* Castling
* En passant
* Pawn promotion
* Threefold repetition
* Fifty-move rule
* Insufficient-material detection
* Draw detection
* Move history
* SAN move notation
* UCI move representation
* FEN position support

---

## 🤖 Chess Engine

The computer opponent uses a native search implementation rather than random moves, scripted behavior or an external game engine.

### Search

* Minimax
* Alpha-Beta pruning
* Iterative deepening
* Move ordering
* Quiescence search
* Transposition tables
* Zobrist hashing
* Tactical search

### Position Evaluation

The evaluation system considers multiple aspects of a chess position:

* Material balance
* Piece-square tables
* Mobility
* Center control
* King safety
* Pawn structure
* Passed pawns
* Isolated pawns
* Doubled pawns
* Bishop pair
* Tactical considerations

The architecture is designed so that search and evaluation can evolve independently as the engine becomes stronger.

---

## 🎯 Difficulty Levels

Choose how deeply the computer analyzes the position.

|      Level      | Experience                              |
| :-------------: | --------------------------------------- |
| 🟢 **Beginner** | Lightweight opponent for casual games   |
|   🔵 **Easy**   | Relaxed introductory gameplay           |
|  🟡 **Medium**  | Balanced everyday opponent              |
|   🟠 **Hard**   | Deeper tactical calculation             |
|  🔴 **Expert**  | Stronger tactical and positional search |
|  🟣 **Master**  | Maximum available search strength       |

Difficulty is controlled through engine search behavior rather than a collection of predetermined moves.

---

## 💡 Real-Time Hint System

AurevixChess includes a **real position-aware Hint system**.

The application analyzes the current board using the same chess engine used by the computer opponent.

It can provide:

* Recommended move
* Source square
* Destination square
* Engine evaluation
* Position-aware explanation
* Best-move visualization

### 🔎 Show Best Move

For analysis and learning, the application can explicitly visualize the engine's strongest discovered move.

This makes the hint system useful for understanding positions rather than simply revealing a hard-coded answer.

---

## 🎮 Game Modes

### 👤 vs 🤖 Human vs Computer

Play against the built-in chess engine with selectable difficulty.

### 👤 vs 👤 Player vs Player

Two players can play locally on the same computer.

### 🤖 vs 🤖 Computer vs Computer

Watch two engine instances play against each other.

### 🧪 Practice

Experiment with positions and moves without the normal competitive flow.

### 🔬 Analysis

Explore positions, moves and engine recommendations.

### 🧩 Puzzle

Solve tactical positions and validate your moves against the expected solution.

---

## ⏱️ Chess Clocks

AurevixChess includes native chess clock functionality:

* Configurable base time
* Increment support
* Independent player clocks
* Automatic turn switching
* Time tracking
* Clock stopping
* Time expiration handling

Designed to support everything from casual games to traditional timed chess formats.

---

## 🔄 Game Management

Manage games directly inside the application:

* ↩️ Undo
* ↪️ Redo
* Move navigation
* Complete move history
* New game
* Position reset
* FEN import
* PGN export
* Game-state tracking

---

## 📚 Opening Support

AurevixChess includes opening-related functionality for identifying and displaying opening information during gameplay and analysis.

The architecture is designed to support local opening data without requiring an online service.

---

## 🧩 Puzzle Mode

Puzzle mode provides a dedicated tactical environment.

Supported workflows include:

* FEN-based puzzle positions
* Best-move validation
* Attempt tracking
* Tactical themes
* Result evaluation

The system is designed to make future puzzle generation and larger puzzle collections possible.

---

## 🎨 Modern Desktop UI

AurevixChess is built to feel like a modern desktop application rather than a traditional chessboard demo.

### UI principles

* Modern visual hierarchy
* Clean layouts
* Smooth animations
* Hover interactions
* Move indicators
* Selected-square highlighting
* Check highlighting
* Engine thinking state
* Evaluation visualization
* Modern menus
* Game-over screens
* Settings
* Statistics
* Analysis screens
* Keyboard-friendly navigation
* Light / dark visual support

The UI layer is separated from chess logic and engine code to keep the project maintainable.

---

## 🔊 Audio

A dedicated audio layer provides interaction feedback for events such as:

* Piece movement
* Captures
* Check
* Game completion
* UI interactions

---

# 🏗️ Architecture

AurevixChess follows a modular architecture separating chess logic, engine computation, rendering, persistence and application state.

```text
AurevixChess/
│
├── 📁 assets/
│
├── 📁 src/
│   │
│   ├── 📁 chess/
│   │   ├── chess.hpp
│   │   ├── chess.cpp
│   │   └── fen.cpp
│   │
│   ├── 📁 engine/
│   │   ├── eval.hpp
│   │   ├── eval.cpp
│   │   ├── search.hpp
│   │   └── search.cpp
│   │
│   ├── 📁 io/
│   │   ├── opening_book.hpp
│   │   ├── opening_book.cpp
│   │   ├── persistence.hpp
│   │   └── persistence.cpp
│   │
│   ├── 📁 ui/
│   │   ├── app.hpp
│   │   ├── app.cpp
│   │   ├── menu.cpp
│   │   ├── render.hpp
│   │   ├── render.cpp
│   │   ├── pieces.hpp
│   │   ├── pieces.cpp
│   │   ├── theme.hpp
│   │   ├── theme.cpp
│   │   ├── sound.hpp
│   │   └── sound.cpp
│   │
│   └── main.cpp
│
├── 📁 tests/
│
├── 📄 CMakeLists.txt
├── 📄 LICENSE
├── 📄 .gitignore
└── 📄 README.md
```

### Module Responsibilities

| Module    | Responsibility                           |
| --------- | ---------------------------------------- |
| `chess/`  | Board state, pieces, moves and rules     |
| `engine/` | Search, evaluation and AI                |
| `ui/`     | Application screens and user interaction |
| `io/`     | Persistence, PGN and opening data        |
| `assets/` | Fonts, audio and visual resources        |
| `tests/`  | Automated validation                     |

---

# 🛠️ Technology Stack

| Technology             | Role                              |
| ---------------------- | --------------------------------- |
| **C++20**              | Core application and engine       |
| **SFML 2.6**           | Window, graphics, input and audio |
| **CMake**              | Build configuration               |
| **Ninja**              | Build system                      |
| **Clang / GCC / MSVC** | C++ compilation                   |
| **Git**                | Version control                   |

---

# 🚫 No Game Engine

AurevixChess does **not** use:

* ❌ Unity
* ❌ Unreal Engine
* ❌ Godot
* ❌ GameMaker
* ❌ Any commercial game engine

The application is implemented directly with:

**C++20 + SFML**

SFML is used as a multimedia and rendering library, not as a game engine.

---

# 💻 Requirements

## Windows

Recommended:

* Windows 10 / 11
* C++20-compatible compiler
* CMake 3.16+
* Ninja
* SFML 2.6.x

Compatible compiler families include:

* Clang
* GCC
* MSVC

---

# 🔧 Build

## 1. Clone

```bash
git clone https://github.com/qusayjber/AurevixChess.git
cd AurevixChess
```

## 2. Configure

```bash
cmake -S . -B build -G Ninja
```

If SFML is installed in a custom location:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="PATH_TO_SFML"
```

## 3. Compile

```bash
cmake --build build
```

## 4. Run

### Windows

```powershell
.\build\chess.exe
```

The executable is generated at:

```text
build/chess.exe
```

---

# 📦 SFML Runtime

When using a dynamically linked SFML installation, the required SFML DLL files must be available to the executable.

They can either be:

* placed beside `chess.exe`, or
* added to the system `PATH`.

For development environments, configure CMake with the location of the SFML installation.

---

# 🧪 Testing

The architecture intentionally separates the core chess system from the graphical interface.

This makes the following areas suitable for automated testing:

### Chess Rules

* Move generation
* Legal move validation
* Check detection
* Checkmate
* Stalemate
* Castling
* En passant
* Promotion
* Draw detection

### Position Handling

* FEN parsing
* FEN generation
* Position state
* Move history
* Hashing

### Engine

* Evaluation
* Search
* Move ordering
* Transposition handling
* Tactical positions

---

# 📊 Design Goals

AurevixChess is developed around several core principles:

```text
Correctness
    ↓
Performance
    ↓
Maintainability
    ↓
User Experience
    ↓
Extensibility
```

### Core goals

* ♟ Correct chess behavior
* 🤖 Real engine-powered gameplay
* ⚡ Native performance
* 🎨 Modern desktop UX
* 🧩 Modular architecture
* 🔒 Offline-first design
* 🛠 Maintainable C++
* 📈 Extensible engine
* 🧪 Testable core logic

---

# 🗺️ Roadmap

Future development may include:

### Engine

* [ ] Stronger evaluation
* [ ] Improved search heuristics
* [ ] Multi-PV analysis
* [ ] Engine benchmarking
* [ ] Principal variation display
* [ ] Advanced tactical analysis

### Chess Data

* [ ] Expanded opening book
* [ ] PGN database
* [ ] Opening explorer
* [ ] Larger puzzle database
* [ ] Automatic puzzle generation

### Gameplay

* [ ] More chess variants
* [ ] Engine tournaments
* [ ] Advanced game statistics
* [ ] Replay improvements

### Connectivity

* [ ] Online multiplayer
* [ ] Network games
* [ ] Online game integration

---

# 📦 Releases

Official compiled builds will be distributed through the repository's **GitHub Releases**.

A release package is intended to contain:

```text
AurevixChess/
│
├── chess.exe
├── SFML DLLs
├── assets/
├── LICENSE
└── README.txt
```

---

# 🤝 Contributing

Contributions, bug reports and improvements are welcome.

Before submitting a pull request:

1. Keep changes focused.
2. Preserve the modular architecture.
3. Avoid unnecessary dependencies.
4. Verify that the project builds successfully.
5. Test chess-rule changes carefully.
6. Keep UI and engine responsibilities separated.
7. Avoid introducing placeholder functionality.

---

# 📄 License

AurevixChess is released under the **MIT License**.

See [`LICENSE`](LICENSE) for the complete license text.

---

# 👤 Author

<p align="center">
  <strong>Qusai Jaber</strong>
</p>

<p align="center">
  Software Developer · C++ · Java · Backend · Full-Stack
</p>

<p align="center">
  <a href="https://github.com/qusayjber">
    <img src="https://img.shields.io/badge/GitHub-qusayjber-181717?style=for-the-badge&logo=github" alt="GitHub">
  </a>
</p>

---

<p align="center">
  <strong>♟️ AurevixChess</strong>
  <br>
  <em>Built with C++20. Designed for chess.</em>
</p>
