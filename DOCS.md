# Gurotopia Documentation

## Overview

**Gurotopia** is a lightweight, open-source Growtopia Private Server (GTPS) written in C/C++. It provides a complete server implementation for the Growtopia game, featuring world management, peer interactions, item systems, and more.

### Key Features

- **Lightweight & Efficient**: Built with ENet for UDP networking
- **SQLite Database**: Player data persistence
- **HTTPS Server**: Login and server data handling
- **Item System**: Full items.dat parsing with all item types
- **World Management**: Dynamic world creation, locking, and administration
- **Peer Management**: Friend systems, inventory, clothing, and more
- **Command System**: Built-in admin commands
- **Holiday Events**: Automated holiday detection and theming

---

## Table of Contents

1. [Setup Guide](#setup-guide)
2. [sqlite3.h Fix Documentation](#sqlite3h-fix-documentation)
3. [Project Structure](#project-structure)
4. [Architecture Overview](#architecture-overview)
5. [Core Components](#core-components)
6. [Development Guide](#development-guide)

---

## Setup Guide

### Windows Setup

#### Prerequisites

1. **MSYS2**: Download from [msys2.org](https://www.msys2.org/)
2. **Visual Studio Code**: Download from [code.visualstudio.com](https://code.visualstudio.com/)
3. **C/C++ Extension**: Install `ms-vscode.cpptools` extension

#### Installation Steps

1. **Install Dependencies** (via MSYS2 UCRT64):
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-{gcc,openssl,sqlite} make
   ```

2. **Clone and Build**:
   - Open project in VS Code
   - Press `Ctrl+Shift+B` to build
   - Press `F5` to run with debugger

### Linux Setup

#### Debian/Ubuntu
```bash
sudo apt-get update && sudo apt-get install build-essential libssl-dev openssl sqlite3 libsqlite3-dev g++-13
make -j$(nproc)
./main.out
```

#### Arch Linux
```bash
sudo pacman -S base-devel openssl sqlite
make -j$(nproc)
./main.out
```

### Docker Setup

```bash
docker build -t gurotopia .
docker run -p 17091:17091 gurotopia
```

---

## sqlite3.h Fix Documentation

### Problem Description

When setting up the project, you may encounter the error:
```
fatal error: sqlite3.h: No such file or directory
```

Even after adding `"C:/msys64/ucrt64/include"` to `c_cpp_properties.json`, IntelliSense may still not find `sqlite3.h`.

### Root Cause Analysis

The issue was caused by **multiple GCC compilers** on the system:

1. `E:\App\mingw64\bin\g++.exe` - MinGW-w64 (does NOT have sqlite3.h)
2. `C:\msys64\ucrt64\bin\g++.exe` - MSYS2 UCRT64 (HAS sqlite3.h)

Windows PATH prioritized the wrong compiler (`E:\App\mingw64`), causing both compilation and IntelliSense to fail.

### Solution Applied

#### 1. Fixed `c_cpp_properties.json`

**Before:**
```json
{
    "includePath": [
        "${workspaceFolder}",
        "${workspaceFolder}/include",
        "${workspaceFolder}/include/enet",
        "C:/msys64/ucrt64/include",
        "C:/msys64/ucrt64/include/*"  // ❌ Invalid wildcard
    ],
    "compilerPath": "c:/msys64/ucrt64/bin/g++.exe"
}
```

**After:**
```json
{
    "includePath": [
        "${workspaceFolder}",
        "${workspaceFolder}/include",
        "${workspaceFolder}/include/enet",
        "C:/msys64/ucrt64/include"  // ✅ Removed wildcard
    ],
    "compilerPath": "C:/msys64/ucrt64/bin/g++.exe"  // ✅ Absolute path with capital drive
}
```

**Key Changes:**
- Removed invalid wildcard `/*` from include path (IntelliSense doesn't support wildcards)
- Used absolute path for compiler with consistent casing

#### 2. Fixed `Makefile`

**Before:**
```makefile
CXX := g++
CXXFLAGS := -std=c++2b -g -Iinclude -MMD -MP
LIBS := -L./include/enet/lib -lssl -lcrypto -lsqlite3
```

**After:**
```makefile
CXX := C:/msys64/ucrt64/bin/g++.exe  # ✅ Explicit MSYS2 compiler
CXXFLAGS := -std=c++2b -g -Iinclude -I"C:/msys64/ucrt64/include" -MMD -MP
LIBS := -L./include/enet/lib -L"C:/msys64/ucrt64/lib" -lssl -lcrypto -lsqlite3
```

**Key Changes:**
- Explicitly set compiler path to MSYS2 g++
- Added SQLite include path: `-I"C:/msys64/ucrt64/include"`
- Added SQLite library path: `-L"C:/msys64/ucrt64/lib"`

#### 3. Copied Required DLLs

Runtime DLLs copied to project root:
- `libsqlite3-0.dll` (SQLite3 runtime)
- `libssl-3-x64.dll` (OpenSSL SSL)
- `libcrypto-3-x64.dll` (OpenSSL Crypto)
- `libenet-7.dll` (ENet networking)

**Source:** `C:\msys64\ucrt64\bin\`

### Verification

After applying fixes, verify with:

```bash
# Test compilation
C:\msys64\usr\bin\bash.exe -c "export PATH=\"$PATH:/c/msys64/ucrt64/bin:/c/msys64/usr/bin\" && make clean && make"

# Expected output should show:
# C:/msys64/ucrt64/bin/g++.exe -std=c++2b -g -Iinclude -I"C:/msys64/ucrt64/include" ...
```

### Prevention Tips

1. **Check compiler order**: Run `where g++` to see which compiler is prioritized
2. **Use absolute paths**: Always specify full paths in Makefile for critical tools
3. **Avoid wildcards**: IntelliSense include paths don't support `*` patterns
4. **Reload VS Code**: After config changes, use `Ctrl+Shift+P` → "BAN_SHENG: Reload Window"

---

## Project Structure

```
Gurotopia/
├── main.cpp                    # Entry point, main game loop
├── Makefile                    # Build configuration
├── Dockerfile                  # Container configuration
├── windows-setup.bat           # Windows installer script
├── linux-setup.sh              # Linux installer script
│
├── .vscode/
│   ├── c_cpp_properties.json   # IntelliSense configuration
│   ├── tasks.json              # Build tasks
│   └── launch.json             # Debug configuration
│
├── .github/workflows/
│   └── make.yml                # CI/CD pipeline
│
├── include/
│   ├── pch.hpp                 # Pre-compiled header
│   │
│   ├── action/                 # Action packet handlers
│   │   ├── __action.cpp/hpp    # Action pool registration
│   │   ├── dialog_return/      # Dialog response handlers
│   │   ├── enter_game.cpp/hpp  # Game entry logic
│   │   ├── protocol.cpp/hpp    # Protocol handling
│   │   ├── tankIDName.cpp/hpp  # Player name handling
│   │   └── ... (20+ action handlers)
│   │
│   ├── automate/               # Automated systems
│   │   ├── holiday.cpp/hpp     # Holiday detection & theming
│   │
│   ├── commands/               # Chat commands
│   │   ├── __command.cpp/hpp   # Command pool registration
│   │   ├── find.cpp/hpp        # /find command
│   │   ├── warp.cpp/hpp        # /warp command
│   │   ├── edit.cpp/hpp        # /edit command
│   │   └── ... (12 commands)
│   │
│   ├── database/               # Data layer
│   │   ├── items.cpp/hpp       # Items.dat parsing
│   │   ├── peer.cpp/hpp        # Player class & database
│   │   ├── world.cpp/hpp       # World class & management
│   │   └── shouhin.cpp/hpp     # Shop/store system
│   │
│   ├── enet/                   # ENet library (custom fork)
│   │
│   ├── event_type/             # ENet event handlers
│   │   ├── __event_type.cpp/hpp # Event pool registration
│   │   ├── connect.cpp/hpp     # Peer connection
│   │   ├── disconnect.cpp/hpp  # Peer disconnection
│   │   └── receive.cpp/hpp     # Packet reception
│   │
│   ├── https/                  # HTTPS server
│   │   ├── https.cpp/hpp       # HTTPS listener
│   │   └── server_data.cpp/hpp # Server configuration
│   │
│   ├── on/                     # "On" packet handlers
│   │   ├── Spawn.cpp/hpp       # OnSpawn packet
│   │   ├── SetClothing.cpp/hpp # OnSetClothing packet
│   │   └── ... (10 handlers)
│   │
│   ├── packet/                 # Packet utilities
│   │   ├── packet.cpp/hpp      # Packet creation helpers
│   │
│   ├── state/                  # State packet handlers
│   │   ├── __states.cpp/hpp    # State pool registration
│   │   ├── movement.cpp/hpp    # Player movement
│   │   ├── tile_change.cpp/hpp # Tile modification
│   │   └── ... (7 handlers)
│   │
│   └── tools/                  # Utilities
│       ├── string.cpp/hpp      # String utilities
│       ├── create_dialog.cpp   # Dialog creation
│       └── ransuu.cpp          # Random utilities
│
└── resources/
    └── store.txt               # Shop configuration
```

---

## Architecture Overview

### Main Flow

```
┌─────────────────────────────────────────────────────────────┐
│                         main.cpp                            │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ 1. Initialize Libraries (ENet, SQLite, OpenSSL)      │   │
│  │ 2. Create Directories (db/)                          │   │
│  │ 3. Initialize ENet Host (Port 17091, Max 50 peers)   │   │
│  │ 4. Start HTTPS Listener (Separate Thread)            │   │
│  │ 5. Load Data (items.dat, store.txt, holidays)        │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Main Game Loop (while true)              │   │
│  │  ┌────────────────────────────────────────────────┐   │   │
│  │  │  enet_host_service() - Poll Events (1000ms)    │   │   │
│  │  │  ┌──────────────────────────────────────────┐  │   │   │
│  │  │  │ Event Type → event_pool → Handler        │  │   │   │
│  │  │  │ • ENET_EVENT_TYPE_CONNECT                │  │   │   │
│  │  │  │ • ENET_EVENT_TYPE_DISCONNECT             │  │   │   │
│  │  │  │ • ENET_EVENT_TYPE_RECEIVE                │  │   │   │
│  │  │  └──────────────────────────────────────────┘  │   │   │
│  │  └────────────────────────────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Event System

```
ENetEvent
    ├── Connect → _connect() → Create peer object
    ├── Disconnect → _disconnect() → Cleanup peer
    └── Receive → receive()
                    ├── Packet Type 2/3 (String)
                    │   └── action_pool → action handlers
                    │       ├── protocol|version
                    │       ├── tankIDName
                    │       ├── action|enter_game
                    │       └── action|dialog_return
                    │
                    └── Packet Type 4 (State)
                        └── state_pool → state handlers
                            ├── 0x00 → movement
                            ├── 0x03 → tile_change
                            ├── 0x07 → tile_activate
                            └── 0x0a → item_activate
```

### Database Schema

#### Peer Class (In-Memory + SQLite)
```cpp
class peer {
    int netid;              // World identity
    int user_id;            // Unique user ID
    std::array<std::string, 2> ltoken;  // {growid, password}
    std::string game_version;
    std::string country;
    std::string prefix;     // Display name color
    u_char role;            // PLAYER, DOU_ZUN, BAN_SHENG
    std::array<float, 10> clothing;
    u_char punch_effect;
    u_int skin_color;
    int state;              // Player state flags
    Billboard billboard;    // Selling/buying info
    pos pos;                // Current position
    pos rest_pos;           // Respawn position
    bool facing_left;
    short slot_size;
    std::vector<slot> slots;  // Inventory
    std::vector<short> fav;   // Favorites
    signed gems;
    std::array<u_short, 2> level;  // {level, xp}
    std::array<std::string, 6> recent_worlds;
    std::array<std::string, 200> my_worlds;
    std::deque<time_point> messages;  // Anti-spam
    std::array<Friend, 25> friends;
};
```

#### World Class
```cpp
class world {
    std::string name;
    int owner;              // Owner user_id
    std::array<int, 6> admin;  // Admin user_ids
    bool is_public;
    u_char lock_state;
    u_char minimum_entry_level;
    u_char visitors;
    u_char netid_counter;
    std::vector<block> blocks;  // 1D array (6000 tiles)
    u_int last_object_uid;
    std::vector<object> objects;
    std::vector<door> doors;
    std::vector<display> displays;
    std::vector<random_block> random_blocks;
    pos 现weather;
};
```

---

## Core Components

### 1. Networking (ENet)

**Library**: Custom ENet fork (SHA: `2b22def892100ca86b729a22a94a60bbacc9667f2`)

**Configuration:**
- **Protocol**: UDP with reliability layer
- **Port**: 17091
- **Max Peers**: 50
- **Channels**: 2
- **Checksum**: CRC32
- **Compression**: Range coder

**Key Files:**
- `include/enet/enet.h` - ENet header
- `include/event_type/connect.cpp` - Connection handling
- `include/event_type/receive.cpp` - Packet reception

### 2. Database (SQLite3)

**Version**: SQLite 3.x (via MSYS2)

**Usage:**
- Player data persistence
- World data storage
- Inventory management

**Key Files:**
- `include/database/peer.cpp` - Peer database operations
- `include/database/world.cpp` - World persistence
- `include/pch.hpp` - SQLite3 include

### 3. HTTPS Server

**Purpose:**
- Login authentication
- Server data distribution
- Growth world verification

**Configuration:**
```cpp
class server_data {
    std::string server = "127.0.0.1";
    u_short port = 17091;
    u_char type = 1;
    std::string loginurl = "login-gurotopia.vercel.app";
    std::string meta = "gurotopia";
};
```

**Key Files:**
- `include/https/https.cpp` - HTTPS listener
- `include/https/server_data.cpp` - Server configuration

### 4. Item System

**Items.dat Parsing:**
- Binary format parsing
- XOR decryption with token: `PBG892FXX982ABC*`
- Version-aware parsing (v1-v24)

**Item Types:**
```cpp
enum type : u_char {
    FIST = 0, WRENCH, DOOR, LOCK, GEM, TREASURE,
    DEADLY, TRAMPOLINE, CONSUMEABLE, ENTRANCE, SIGN,
    FOREGROUND, BACKGROUND, SEED, CLOTHING, ANIMATED,
    CHEST, MAILBOX, PORTAL, WEATHER_MACHINE, ...
};
```

**Key Files:**
- `include/database/items.cpp` - items.dat parser
- `include/database/items.hpp` - Item definitions

### 5. Packet System

**Packet Types:**

| Type | ID | Description |
|------|-----|-------------|
| String | 2-3 | Text-based packets (actions, dialogs) |
| State | 4 | Binary state packets (movement, tiles) |

**State Packet Structure:**
```cpp
class state {
    int packet_create = 4;
    int type;           // Action type
    int netid;          // Network ID
    int uid;            // User ID
    int peer_state;     // State flags
    float count;
    int id;             // Hand item ID
    pos pos;            // Position
    std::array<float, 2> speed;  // Velocity
    pos punch;          // Punch/place position
    int size;
};
```

**Key Files:**
- `include/packet/packet.cpp` - Packet creation
- `include/state/__states.cpp` - State handlers
- `include/action/__action.cpp` - Action handlers

### 6. Command System

**Available Commands:**
- `/find` - Find item/player
- `/warp {world}` - Teleport to world
- `/edit {player}` - Edit player data
- `/punch {id}` - Set punch item
- `/skin {id}` - Set skin color
- `/sb {msg}` - Broadcast message
- `/who` - List online players
- `/weather {id}` - Set weather
- `/ghost` - Toggle ghost mode
- `/ageworld` - Age world

**Key Files:**
- `include/commands/__command.cpp` - Command registration
- `include/commands/*.cpp` - Individual commands

---

## Development Guide

### Adding New Commands

1. **Create command file** (`include/commands/mycommand.cpp`):
```cpp
#include "pch.hpp"

void mycommand(ENetEvent& event, const std::string_view args)
{
    // Your command logic here
    packet::action(*event.peer, "log", "msg|Hello from my command!");
}
```

2. **Register in command pool** (`include/commands/__command.cpp`):
```cpp
std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool
{
    {"mycommand", &mycommand},
    // ... existing commands
};
```

3. **Add header** (`include/commands/__command.hpp`):
```cpp
extern void mycommand(ENetEvent& event, const std::string_view args);
```

### Adding New Actions

1. **Create action handler** (`include/action/myaction.cpp`):
```cpp
#include "pch.hpp"

namespace action
{
    void myaction(ENetEvent& event, const std::string& data)
    {
        // Parse data (pipe-separated values)
        // Perform action
    }
}
```

2. **Register in action pool** (`include/action/__action.cpp`):
```cpp
#include "myaction.hpp"

std::unordered_map<std::string, std::function<void(ENetEvent&, const std::string&)>> action_pool
{
    {"action|myaction", std::bind(&action::myaction, std::placeholders::_1, std::placeholders::_2)},
    // ... existing actions
};
```

### Adding New State Handlers

1. **Create state handler** (`include/state/mystate.cpp`):
```cpp
#include "pch.hpp"

void mystate(ENetEvent& event, state s)
{
    // Handle state packet
    // s.type, s.pos, s.speed, etc.
}
```

2. **Register in state pool** (`include/state/__states.cpp`):
```cpp
std::unordered_map<u_char, std::function<void(ENetEvent&, state)>> state_pool
{
    {0xXX, std::bind(&mystate, std::placeholders::_1, std::placeholders::_2)},
    // ... existing states
};
```

### Building and Testing

```bash
# Clean build
make clean && make

# Run with debugger
F5  # In VS Code

# Run directly
./main.exe  # Windows
./main.out  # Linux
```

### Debugging Tips

1. **Enable logging**: Use `puts()` or `std::printf()` for debug output
2. **Check peer data**: `static_cast<peer*>(event.peer->data)`
3. **Inspect packets**: Print `event.packet->data` as hex/string
4. **Database queries**: Check SQLite return codes

### Code Style

- **Naming**: snake_case for functions/variables, PascalCase for classes
- **Includes**: Use `pch.hpp` for common headers
- **Comments**: `// @note`, `// @todo`, `// @warning` annotations
- **Memory**: Use RAII, smart pointers where applicable

---

## Troubleshooting

### Common Issues

#### 1. sqlite3.h not found
**Solution**: See [sqlite3.h Fix Documentation](#sqlite3h-fix-documentation)

#### 2. DLL not found at runtime
**Solution**: Copy required DLLs to executable directory:
- `libsqlite3-0.dll`
- `libssl-3-x64.dll`
- `libcrypto-3-x64.dll`
- `libenet-7.dll`

#### 3. Build fails with "g++ not found"
**Solution**: Ensure MSYS2 is in PATH or use full path in Makefile

#### 4. Server won't start (port in use)
**Solution**: Change port in `include/https/server_data.hpp`

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make changes following code style
4. Test thoroughly
5. Submit pull request

---

## License

See [LICENSE](LICENSE) file for licensing information.

---

## Community

- **Discord**: [Join Server](https://discord.gg/zzWHgzaF7J)
- **GitHub**: [Gurotopia Repository](https://github.com/gurotopia/Gurotopia)

---

*Last updated: March 15, 2026*
