# ♟ AurevixChess

> A modern, engine-powered chess application built from the ground up with C++20 and SFML.

AurevixChess is a standalone desktop chess application designed to combine a polished modern interface with a complete chess rules system and a real chess-playing engine.

It supports **Human vs Computer**, **Player vs Player**, **Computer vs Computer**, practice and analysis-oriented workflows, with multiple AI difficulty levels and a real position-based hint system.

---

## ✨ Features

### ♟ Complete Chess Rules

AurevixChess implements the essential rules and game states required for a full chess experience:

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
* SAN / UCI move representation
* FEN position support

---

## 🤖 Chess AI

The computer opponent is powered by a native chess search implementation rather than random or scripted moves.

The engine architecture includes:

* Minimax search
* Alpha-Beta pruning
* Iterative deepening
* Move ordering
* Quiescence search
* Transposition tables
* Zobrist hashing
* Position evaluation
* Tactical search
* Material evaluation
* Piece-square tables
* Mobility evaluation
* Center control
* King safety
* Pawn structure analysis
* Passed pawns
* Isolated pawns
* Doubled pawns
* Bishop-pair evaluation

The engine is designed to provide progressively stronger play as the selected difficulty increases.

---

## 🎯 Difficulty Levels

AurevixChess provides multiple computer difficulty levels:

| Level    | Description                           |
| -------- | ------------------------------------- |
| Beginner | Lightweight search for casual play    |
| Easy     | Introductory computer opponent        |
| Medium   | Balanced everyday difficulty          |
| Hard     | Deeper tactical search                |
| Expert   | Stronger positional and tactical play |
| Master   | Maximum available search strength     |

Difficulty affects the engine's search behavior and thinking depth rather than simply selecting predefined moves.

---

## 💡 Real Hint System

The Hint feature analyzes the **current board position** using the chess engine.

It is not a static tutorial system or a collection of predefined suggestions.

Hints can provide:

* Recommended move
* Source square
* Destination square
* Engine evaluation
* Position-based explanation
* Best-move visualization

The application also provides a dedicated **Show Best Move** workflow for analysis.

---

## 🎮 Game Modes

### Human vs Computer

Play against the built-in chess engine.

### Player vs Player

Two human players can play locally on the same machine.

### Computer vs Computer

Watch the chess engine play against itself.

### Practice

Experiment with positions and moves without the normal competitive flow.

### Analysis

Inspect positions, moves and engine recommendations.

---

## ⏱ Time Controls

The application supports chess clock functionality with:

* Configurable base time
* Increment support
* Separate clocks for both players
* Automatic turn switching
* Clock stopping
* Time expiration handling

---

## 🔄 Game Management

AurevixChess includes:

* Undo
* Redo
* Move navigation
* Complete move history
* New game
* Position reset
* FEN import
* PGN export
* Game-state tracking

---

## 📚 Opening Support

The application includes opening-related functionality designed to identify and display opening information during a game.

Opening information can be integrated into the game and analysis workflow without requiring an online service.

---

## 🧩 Puzzle Mode

Puzzle functionality provides a dedicated environment for tactical chess positions.

Puzzle workflows can include:

* Position loading
* Best-move validation
* Attempt tracking
* Tactical themes
* Result evaluation

---

## 🎨 User Interface

The interface is built specifically for AurevixChess rather than relying on a traditional chessboard template.

UI goals include:

* Modern visual hierarchy
* Smooth animations
* Responsive layouts
* Hover feedback
* Visual move indicators
* Selected-square highlighting
* Check highlighting
* Engine thinking state
* Evaluation visualization
* Modern menus
* Game-over presentation
* Settings screens
* Dark/light visual support
* Keyboard-friendly navigation

The application is designed to feel like a modern desktop product rather than a basic educational chess implementation.

---

## 🔊 Audio

The application includes a dedicated audio layer for chess interaction feedback.

Sound events can be associated with:

* Piece movement
* Captures
* Check
* Game completion
* UI interaction

---

## 🏗 Architecture

AurevixChess is organized into independent modules to keep the chess logic, engine, rendering and application layers maintainable.

```text
AurevixChess/
│
├── assets/
│
├── src/
│   ├── chess/
│   │   ├── chess.hpp
│   │   ├── chess.cpp
│   │   └── fen.cpp
│   │
│   ├── engine/
│   │   ├── eval.hpp
│   │   ├── eval.cpp
│   │   ├── search.hpp
│   │   └── search.cpp
│   │
│   ├── io/
│   │   ├── opening_book.hpp
│   │   ├── opening_book.cpp
│   │   ├── persistence.hpp
│   │   └── persistence.cpp
│   │
│   ├── ui/
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
├── tests/
│
├── CMakeLists.txt
├── LICENSE
├── .gitignore
└── README.md
```

---

## 🛠 Technology Stack

| Technology | Purpose                              |
| ---------- | ------------------------------------ |
| C++20      | Core application language            |
| SFML 2.6   | Windowing, graphics, audio and input |
| CMake      | Build configuration                  |
| Ninja      | Fast build system                    |
| Clang      | C++ compiler                         |
| Git        | Version control                      |

---

## 🚫 No Game Engine

AurevixChess does **not** use:

* Unity
* Unreal Engine
* Godot
* GameMaker
* Any commercial game engine

The application is implemented directly with C++ and SFML.

---

## 💻 Requirements

### Windows

Recommended development environment:

* Windows 10/11
* C++20-compatible compiler
* CMake 3.16+
* Ninja
* SFML 2.6.x

Supported compilers include modern:

* Clang
* GCC
* MSVC

---

## 🔧 Build From Source

Clone the repository:

```bash
git clone https://github.com/qusayjber/AurevixChess.git
cd AurevixChess
```

Configure the project:

```bash
cmake -S . -B build -G Ninja
```

Build:

```bash
cmake --build build
```

The resulting executable will be generated inside:

```text
build/chess.exe
```

---

## 📦 SFML

AurevixChess uses SFML for:

* Window management
* 2D rendering
* Input
* Audio

Make sure SFML is installed and discoverable by CMake before configuring the project.

For Windows development, you can provide the SFML installation path through CMake:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="PATH_TO_SFML"
```

---

## 🚀 Running

After a successful build:

```bash
./build/chess
```

On Windows:

```powershell
.\build\chess.exe
```

If SFML is dynamically linked, ensure the required SFML DLLs are available through the executable's directory or system `PATH`.

---

## 🧪 Testing

The project keeps chess logic and engine functionality separated from the UI so that core functionality can be tested independently.

Areas suitable for automated testing include:

* Move generation
* Legal move validation
* Check detection
* Checkmate
* Stalemate
* Castling
* En passant
* Promotion
* FEN parsing
* Position hashing
* Evaluation
* Search
* Draw detection

---

## 📈 Project Goals

AurevixChess is being developed around several principles:

* Correct chess rules
* Real engine-based gameplay
* Responsive interaction
* Clean architecture
* Fast native performance
* Maintainable C++ code
* Modern desktop UX
* Offline-first functionality
* Extensible engine architecture

---

## 🔮 Roadmap

Potential future improvements include:

* Stronger evaluation
* Improved opening-book coverage
* Advanced engine analysis
* Multi-PV analysis
* Engine strength benchmarking
* PGN database management
* More chess variants
* Online multiplayer
* Network game support
* Advanced puzzle generation
* Game statistics
* Opening explorer
* Engine-versus-engine tournaments

---

## 📄 License

AurevixChess is released under the MIT License.

See [`LICENSE`](LICENSE) for the complete license text.

---

## 👤 Author

**Qusai Jaber**

Software Developer · Computer Science · C++ · Java · Backend · Full-Stack

GitHub:

https://github.com/qusayjber

---

## ⭐ Contributing

Contributions, improvements and bug reports are welcome.

Before submitting a pull request:

1. Keep changes focused.
2. Preserve the existing architecture.
3. Avoid introducing unnecessary dependencies.
4. Verify that the project still builds successfully.
5. Test chess-rule changes carefully.
6. Keep UI and engine responsibilities separated.

---

## 🏁 Project Status

AurevixChess is an actively developed native desktop chess application.

The current goal is to evolve it into a polished, fully playable chess platform with a strong local engine and a professional desktop experience.
