# Gurotopia - Growtopia Private Server Documentation

## Overview

**Gurotopia** (グロートピア) is a lightweight, open-source Growtopia Private Server (GTPS) written in C/C++. It provides a complete server implementation for the Growtopia game, featuring world management, peer interactions, item systems, and more.

**Version Info:**
- ENet Version: 1.3.18
- SQLite3 Version: 3.51.2
- OpenSSL Version: 3.6.1

---

## Project Architecture

### Core Components

The server is built around several key architectural patterns:

1. **Event-Driven Architecture**: Uses ENet for UDP networking with event pools
2. **Pool-Based Command/Action System**: Unordered maps for routing actions, commands, and states
3. **SQLite Database**: Persistent storage for players and worlds
4. **HTTPS Server**: Handles login authentication on port 443

### Directory Structure

```
include/
├── action/              # Action handlers (client requests)
│   ├── dialog_return/   # Dialog response handlers
│   └── ...
├── automate/            # Automated systems (holidays)
├── commands/            # Player commands (/warp, /edit, etc.)
├── database/            # Data models and persistence
├── enet/                # ENet networking library
├── event_type/          # ENet event handlers
├── https/               # HTTPS server for login
├── on/                  # Server response packets (On*)
├── packet/              # Packet creation utilities
├── state/               # State packet handlers
└── tools/               # Utility functions
```

---

## Networking System

### ENet Host Configuration

```cpp
// Server listens on port 17091 (configurable)
ENetAddress address{
    .type = ENET_ADDRESS_TYPE_IPV4,
    .port = g_server_data.port  // Default: 17091
};
host = enet_host_create(ENET_ADDRESS_TYPE_IPV4, &address, 50, 2, 0, 0);
```

- **Max Peer Count**: 50 players
- **Channels**: 2 (0: Reliable, 1: State updates)
- **Compression**: Range coder with CRC32 checksum

### Packet Structure

All packets use a binary format with the following structure:

```
[Packet Type: 4 bytes][NetID: 4 bytes][UID: 4 bytes][Peer State: 4 bytes]
[Count: 4 bytes][ID: 4 bytes][Position: 8 bytes][Speed: 8 bytes]
[Punch: 8 bytes][Size: 4 bytes][Data: variable]
```

### Packet Types

| Type | ID | Description |
|------|-----|-------------|
| PACKET_INIT | 0x01 | Initialize connection |
| PACKET_SEND_MAP_DATA | 0x04 | Send world data to player |
| PACKET_SEND_TILE_UPDATE_DATA | 0x05 | Update tile state |
| PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE | 0x08 | Update multiple tiles |
| PACKET_CALL_FUNCTION | 0x02/0x03 | Action packets |
| PACKET_SEND_INVENTORY_STATE | 0x09 | Player inventory |
| PACKET_ITEM_CHANGE_OBJECT | 0x0E | Item drop/pickup |
| PACKET_SEND_PARTICLE_EFFECT | 0x11 | Visual effects |
| PACKET_SET_CHARACTER_STATE | 0x14 | Player appearance |
| PACKET_PING_REQUEST | 0x16 | Keep-alive |

---

## Action System

### Action Pool (`include/action/__action.hpp`)

Actions are client-initiated requests handled via string-keyed function map:

```cpp
std::unordered_map<std::string, std::function<void(ENetEvent&, const std::string&)>> action_pool
```

### Registered Actions

| Action | Handler | Description |
|--------|---------|-------------|
| `protocol` | `action::protocol` | Login authentication |
| `tankIDName` | `action::tankIDName` | Client identification |
| `action|refresh_item_data` | `action::refresh_item_data` | Item database refresh |
| `action|enter_game` | `action::enter_game` | Player enters game |
| `action|dialog_return` | `action::dialog_return` | Dialog responses |
| `action|friends` | `action::friends` | Social portal |
| `action|join_request` | `action::join_request` | World entry |
| `action|quit_to_exit` | `action::quit_to_exit` | Exit to world select |
| `action|respawn` | `action::respawn` | Player respawn |
| `action|setSkin` | `action::setSkin` | Change skin color |
| `action|input` | `action::input` | Chat input |
| `action|drop` | `action::drop` | Drop item dialog |
| `action|info` | `action::info` | Item info dialog |
| `action|trash` | `action::trash` | Trash item dialog |
| `action|wrench` | `action::wrench` | Wrench menu |
| `action|itemfavourite` | `action::itemfavourite` | Favorite items |
| `action|inventoryfavuitrigger` | `action::inventoryfavuitrigger` | Favorites UI |
| `action|store` | `action::store` | Store request |
| `action|storenavigate` | `action::storenavigate` | Store navigation |
| `action|buy` | `action::buy` | Purchase items |
| `action|quit` | `action::quit` | Player disconnect |

### Key Action Handlers

#### Protocol (Login Authentication)
- Decodes base64 `ltoken` containing `growId` and `password`
- Validates credentials against database
- Sends `OnSendToServer` redirect to game server

#### TankIDName (Client Info)
- Extracts: tankIDName, game_version, country, user_id
- Loads player data from database
- Sends initial server configuration packet

#### Join Request (World Entry)
- Validates world name (alphanumeric only, uppercase)
- Creates/loads world from database
- Generates new worlds with default terrain
- Sends map data and spawns player
- Notifies other players in world

---

## Command System

### Command Pool (`include/commands/__command.hpp`)

Commands are player-initiated chat commands starting with `/`:

```cpp
std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool
```

### Available Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `/help` or `/?` | `/help` | Show command list |
| `/find` | `/find` | Open item search dialog |
| `/warp` | `/warp {world}` | Teleport to world |
| `/edit` | `/edit {player}` | Edit player data |
| `/punch` | `/punch {id}` | Set punch effect |
| `/skin` | `/skin {id}` | Set skin color |
| `/sb` | `/sb {message}` | Send broadcast message |
| `/who` | `/who` | List players in world |
| `/me` | `/me {message}` | Emote message |
| `/weather` | `/weather {id}` | Change weather |
| `/ghost` | `/ghost` | Toggle ghost mode |
| `/ageworld` | `/ageworld` | Age world by 1 day |

### Emote Commands

All emotes are handled by `on::Action`:
- `/wave`, `/dance`, `/love`, `/sleep`
- `/facepalm`, `/fp`, `/smh`, `/yes`, `/no`
- `/omg`, `/idk`, `/shrug`, `/furious`
- `/rolleyes`, `/foldarms`, `/fa`, `/stubborn`
- `/fold`, `/dab`, `/sassy`, `/dance2`
- `/march`, `/grumpy`, `/shy`

---

## State System

### State Pool (`include/state/__states.hpp`)

State packets handle player actions in the world:

```cpp
std::unordered_map<u_char, std::function<void(ENetEvent&, state)>> state_pool
```

### State Handlers

| Type ID | Handler | Description |
|---------|---------|-------------|
| 0x00 | `movement` | Player movement |
| 0x03 | `tile_change` | Block punching/placement |
| 0x07 | `tile_activate` | Tile interaction |
| 0x0a | `item_activate` | Item usage |
| 0x0b | `item_activate_object` | Object interaction |
| 0x15 | `ping_reply` | Keep-alive response |
| 0x1a | `disconnect` | Player disconnect |

---

## Tile Change System - Deep Dive

The `tile_change` handler (`include/state/tile_change.cpp`) is the most complex state handler, spanning **737 lines** of code. It manages all block interactions including punching (breaking) and placing blocks.

### Flow Chart

```
tile_change(ENetEvent, state)
│
├─ 1. Get world & block at punch position
├─ 2. Determine item being interacted with
├─ 3. Check for fire & fire hose (special case)
├─ 4. Validate permissions (owner/admin/public)
│
├─ IF state.id == 18 (FIST - Punching)
│   ├─ Check for Rayman's Fist (multi-punch)
│   ├─ Handle special block punches:
│   │   ├─ Roulette Wheel → Random number 0-36
│   │   ├─ STRONG blocks → Too strong to break
│   │   ├─ MAIN_DOOR → "Stand over and punch to use"
│   │   ├─ LOCK → Check world ownership
│   │   ├─ PROVIDER → Collect resources (ATM, Chicken, etc.)
│   │   ├─ SEED → Harvest fruit
│   │   ├─ WEATHER_MACHINE → Toggle weather
│   │   ├─ TOGGLEABLE_BLOCK → Toggle state
│   │   └─ RANDOM → Dice/Roshambo values
│   ├─ Apply damage: tile_apply_damage()
│   ├─ Check if block breaks (hits >= item->hits)
│   ├─ Handle special drops:
│   │   ├─ Heartstone/GBC/Super GBC → Rare rewards
│   │   ├─ LOCK → Reset world ownership
│   │   ├─ CAT_RETURN → Return item as drop
│   │   └─ Normal break → Gems, seeds, block + XP
│   │
│   ├─ Handle block destruction:
│   │   ├─ Clear label, state3, vanish paint
│   │   ├─ Set fg/bg to 0
│   │   └─ Reset hits
│   │
│   └─ Send visuals to all players
│
├─ ELSE IF item is clothing
│   └─ Equip/unequip clothing
│
├─ ELSE IF item is CONSUMEABLE
│   ├─ Blast items → Show create world dialog
│   ├─ Paint Buckets → Apply paint (requires paintbrush)
│   ├─ Handle consumeables:
│   │   ├─ Door Mover → Relocate main door
│   │   ├─ Water Bucket → Add/remove water, extinguish fire
│   │   ├─ Block Glue → Toggle glue state
│   │   ├─ Pocket Lighter → Start fire
│   │   ├─ Love Potion #8 → Rock → Heartstone
│   │   ├─ Experience Potion → +10000 XP
│   │   ├─ Megaphone → Show broadcast dialog
│   │   ├─ Duct Tape → Apply duct tape effect
│   │   ├─ Paint Buckets (8 colors) → Apply paint
│   │   └─ Paint Vanish → Remove paint
│   └─ Send particle effects, update tile
│
├─ ELSE IF state.id == 32 (WRENCH - Interact)
│   ├─ LOCK → Show lock edit dialog
│   ├─ DOOR/PORTAL → Show door edit dialog
│   ├─ SIGN → Show sign edit dialog
│   ├─ ENTRANCE → Show gateway edit dialog
│   ├─ DISPLAY_BLOCK → Show display dialog
│   ├─ VENDING_MACHINE → Show vending dialog
│   └─ Return early (no block break)
│
└─ ELSE (Placing block)
    ├─ Check if placing on existing block
    │   ├─ DISPLAY_BLOCK → Add to displays
    │   └─ SEED → Check for splicing combinations
    ├─ Check collision (FULL collision blocks)
    ├─ Handle special placements:
    │   ├─ LOCK → Set world owner, update prefix
    │   ├─ ENTRANCE → Set PUBLIC flag
    │   ├─ PROVIDER → Reset tick timer
    │   └─ SEED → Set growth state
    ├─ Set block direction (left/right)
    ├─ Place block (fg for foreground, bg for background)
    ├─ Remove item from inventory
    └─ Send visuals to all players
```

### Block Breaking Mechanics

#### Hit Counter System

**Block Structure:**
```cpp
struct block
{
    short fg{0}, bg{0};  // Foreground and background item IDs
    // ... other fields ...
    std::array<int, 2zu> hits{0, 0};  // hits[0] = fg hits, hits[1] = bg hits
};
```

**Key Finding:** `block.hits` adalah `std::array<int, 2>` dimana:
- `hits[0]` atau `hits.front()` → Hit counter untuk **foreground** block
- `hits[1]` atau `hits.back()` → Hit counter untuk **background** block

#### Damage Application Process

**Function:** `tile_apply_damage()`

```cpp
void tile_apply_damage(ENetEvent& event, state state, block &block, u_int value)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    // INCREMENT HIT COUNTER
    // Jika fg kosong, increment bg hits, jika tidak increment fg hits
    (block.fg == 0) ? ++block.hits.back() : ++block.hits.front();
    
    // ENCODE DAMAGE VALUE IN PACKET TYPE
    // value di encode di byte paling atas (bits 24-31)
    state.type = (value << 24) | 0x000008;  // 0x{value}000008
    state.id = 6;  // Unknown ID (damage animation)
    state.netid = peer->netid;
    
    // SEND VISUALS TO ALL PLAYERS
    state_visuals(*event.peer, std::move(state));
}
```

**Process Flow:**
1. **Determine which layer to hit:**
   - Jika `block.fg == 0` (foreground kosong) → Hit background
   - Jika `block.fg != 0` (foreground ada) → Hit foreground

2. **Increment hit counter:**
   ```cpp
   ++block.hits.front()  // Increment fg hits
   ++block.hits.back()   // Increment bg hits
   ```

3. **Encode damage value:**
   - Damage value (0-255) di-shift 24 bits ke kiri
   - OR dengan `0x000008` (packet type untuk damage)
   - Hasil: `0x{value}000008`
   - Contoh: Damage value 5 → `0x05000008`

4. **Send visuals:**
   - Packet type dengan encoded damage value
   - State ID 6 (damage animation)
   - NetID player yang memukul

#### Break Detection Logic

**Location:** `tile_change.cpp` line 202-204

```cpp
// AFTER applying damage, check if block should break
if (block.hits.front() >= item->hits) 
    block.fg = 0, block.hits.front() = 0;  // Break foreground
else if (block.hits.back() >= item->hits) 
    block.bg = 0, block.hits.back() = 0;   // Break background
else 
    return;  // Block didn't break yet, exit early
```

**Logic:**
1. Check foreground hits first
2. If `hits[0] >= item->hits` → Break foreground
3. Else check background hits
4. If `hits[1] >= item->hits` → Break background
5. Else → Block belum break, return early

**Example:**
```cpp
// Block dengan Diamond Lock (fg) dan Background (bg)
// Diamond Lock has 10 hits
// Background has 10 hits

block.hits = {0, 0};  // Initial state

// Punch 1
tile_apply_damage();  // hits becomes {1, 0}
// Check: 1 >= 10? NO → return early (no break)

// Punch 2-9
// hits becomes {9, 0}
// Check: 9 >= 10? NO → return early

// Punch 10
tile_apply_damage();  // hits becomes {10, 0}
// Check: 10 >= 10? YES!
// → block.fg = 0
// → block.hits.front() = 0
// → Block breaks!
```

#### Hit Reset After Break

**Location:** `tile_change.cpp` line 206-208

```cpp
/* @todo update these changes with tile_update() */
block.label = "";           // Clear sign/door label
block.state3 = 0x00;        // Reset direction/state
block.state4 &= ~S_VANISH;  // Remove paint
```

**After block breaks:**
- Label dikosongkan (untuk sign/door)
- State3 di-reset (direction, toggle, public flags)
- State4 vanish paint di-remove
- Hits sudah di-reset ke 0 di logic sebelumnya

#### Special Case: Tree Harvesting (Force Break)

**Location:** `tile_change.cpp` line 149

```cpp
case type::SEED:
{
    if ((steady_clock::now() - block.tick) / 1s >= item->tick)
    {
        block.hits[0] = 99;  // FORCE BREAK
        add_drop(event, ::slot(item->id - 1, ransuu[{2, 12}]), state.punch);
    }
    break;
}
```

**Special Technique:**
- Untuk tree, hits di-set ke 99 (arbitrary high value)
- Ini memastikan `99 >= item->hits` akan selalu TRUE
- Block langsung break tanpa perlu punch berkali-kali

**Note:** Ini adalah contoh "force break" yang sudah ada di codebase, bisa dijadikan referensi untuk instant break mechanic.

#### Rayman's Fist (Multi-Punch)

```cpp
if (peer->clothing[hand] == 5480) // Rayman's Fist
{
    // Punches 4 blocks horizontally in facing direction
    for (int i = 1; i <= 4; ++i)
    {
        int x_nabor = (peer->facing_left) ? state.punch.x-i : state.punch.x+i;
        ::state x_state = state;
        x_state.punch = {x_nabor, x_state.punch.y};
        tile_change(event, std::move(x_state));  // Recursive call
    }
}
```

### Provider Collection System

Providers drop resources when punched after their tick timer expires:

| Provider ID | Name | Drop | Formula |
|-------------|------|------|---------|
| 1008 | ATM Machine | Gems | Random 1-100 gems (denominated) |
| 872 | Chicken | Egg | 1-2 eggs (item.id+2) |
| 866 | Cow | Steak | 1-2 steaks (item.id+2) |
| 1632 | Coffee Maker | Coffee | 1-2 coffee (item.id+2) |
| 3888 | Sheep | Wool | 1-2 wool (item.id+2) |
| 5116 | Tea Set | Teacup | 1-2 teacups (item.id-2) |
| 2798 | Well | Water Bucket | 1-2 water buckets |
| 928 | Science Station | Chemical | Random: P(1/17), B(1/9), Y(1/7), R(1/5), G(default) |

```cpp
case type::PROVIDER:
{
    if ((steady_clock::now() - block.tick) / 1s >= item->tick)
    {
        switch (item->id)
        {
            case 1008: // ATM
            {
                u_char gems = ransuu[{1, 100}];
                for (short i : {100, 50, 10, 5, 1})
                    for (; gems >= i; gems -= i)
                        add_drop(event, {112, i}, state.punch);
                break;
            }
            // ... other providers
        }
        block.tick = steady_clock::now();  // Reset timer
        send_tile_update(event, state, block, *world);
        peer->add_xp(event, 1);
        return;
    }
    break;
}
```

### Seed/Tree Harvesting

```cpp
case type::SEED:
{
    if ((steady_clock::now() - block.tick) / 1s >= item->tick)
    {
        block.hits[0] = 99;  // Force break
        add_drop(event, ::slot(item->id - 1, ransuu[{2, 12}]), state.punch);  // Fruit
    }
    break;
}
```

### Weather Machine Toggle

```cpp
case type::WEATHER_MACHINE:
{
    ::block &weather_machine = world->blocks[cord(world->现 weather.x, world->现 weather.y)];
    
    // Toggle off if another weather machine is already active
    if (!(block.state3 & S_TOGGLE) && !(weather_machine.state3 & S_TOGGLE))
        weather_machine.state3 &= ~S_TOGGLE;
    
    block.state3 ^= S_TOGGLE;  // Toggle state
    world->现 weather = state.punch;  // Update active weather position
    
    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [block, item](ENetPeer& p)
    {
        packet::create(p, false, 0, { 
            "OnSetCurrentWeather", 
            (block.state3 & S_TOGGLE) ? get_weather_id(item->id) : 0 
        });
    });
    break;
}
```

### Special Block Drops (Heartstone/GBC)

When breaking Heartstone (392), Golden Booty Chest (3402), or Super GBC (9350):

```cpp
if (item->id == 392 || item->id == 3402 || item->id == 9350)
{
    short reward =
        (!ransuu[{0, 99}]) ? 1458 :  // GHC (1% chance)
        (!ransuu[{0, 20}]) ? 362 :  // Angel Wings (5%)
        (!ransuu[{0, 8}])  ? 366 :  // Heartbow (12.5%)
        (!ransuu[{0, 8}])  ? 1470 : // Ruby Necklace (12.5%)
        (!ransuu[{0, 20}]) ? 2384 : // Love Bug (5%)
        (!ransuu[{0, 4}])  ? 2396 : // Valensign (25%)
        (!ransuu[{0, 10}]) ? 3388 : // Heartbreaker Hammer (10%)
        (!ransuu[{0, 10}]) ? 2390 : // Teeny Angel Wings (10%)
        (!ransuu[{0, 10}]) ? 3396 : // Lovebird Pendant (10%)
        (!ransuu[{0, 2}])  ? 3404 : // Sour Lollipop (50%)
        (!ransuu[{0, 4}])  ? 3406 : // Sweet Lollipop (25%)
        (!ransuu[{0, 2}])  ? 3408 : // Pink Marble Arch (50%)
        388;  // Perfume (default)
    
    add_drop(event, ::slot(reward, (reward == 3408 || reward == 3404) ? 10 : 1), state.punch);
    
    if (reward == 1458)  // GHC found - announce globally
    {
        std::string message = std::format(
            "msg|`4The Power of Love! `2{} found a `#Golden Heart Crystal`2 in a `#{}`2!",
            peer->ltoken[0], item->raw_name);
        peers(peer->recent_worlds.back(), PEER_ALL, [message](ENetPeer &p)
        {
            packet::action(p, "log", message.c_str());
        });
    }
    
    // Pity system: every 100 GBCs gives 1 Super GBC
    if (++peer->gbc_pity % 100 == 0)
        modify_item_inventory(event, ::slot{9350, 1});
}
```

### Gem Drop Calculation

```cpp
if (item->type != type::SEED)
{
    // Gem amount based on rarity
    u_char rarity_to_gem =
        (item->rarity >= 87) ? 22 :
        (item->rarity >= 68) ? 18 :
        (item->rarity >= 53) ? 14 :
        (item->rarity >= 41) ? 11 :
        (item->rarity >= 36) ? 10 :
        (item->rarity >= 32) ? 9 :
        (item->rarity >= 24) ? 5 : 1;

    // Double chance for farmables (rarity > 1)
    if (!ransuu[{0, (rarity_to_gem > 1) ? 1 : 4}])
    {
        u_char gems = ransuu[{1, rarity_to_gem}];
        for (short i : {10, 5, 1})  // Denominate gems
            for (; gems >= i; gems -= i)
                add_drop(event, {112, i}, state.punch);
    }
    
    // Seed drop chance
    if (!ransuu[{0, (rarity_to_gem > 1) ? 2 : 4}])
        add_drop(event, ::slot(item->id + 1, 1), state.punch);  // Seed
    else if (!ransuu[{0, (rarity_to_gem > 1) ? 4 : 8}])
        add_drop(event, ::slot(item->id, 1), state.punch);  // Block
}

// XP calculation
peer->add_xp(event, std::trunc(1.0f + item->rarity / 5.0f));
```

### Consumable Items System

Consumables are items that are used (not placed) when right-clicked:

```cpp
else if (item->type == type::CONSUMEABLE)
{
    // Check for Blast items (world creation)
    if (item->raw_name.contains(" Blast"))
    {
        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::format(
                "add_label_with_icon|big|`w{1}``|left|{0}|\n"
                "add_label|small|This item creates a new world! Enter a unique name for it.|left\n"
                "add_text_input|name|New World Name||24|\n"
                "end_dialog|create_blast|Cancel|Create!|\n",
                item->id, item->raw_name).c_str()
        });
    }

    // Paint requires paintbrush (3494)
    if (item->raw_name.contains("Paint Bucket - ") && peer->clothing[hand] != 3494)
        throw std::runtime_error("you need a Paintbrush to apply paint!");

    float color{}, particle{};
    switch (item->id)
    {
        case 1404: // Door Mover
        {
            if (!door_mover(*world, state.punch))
                throw std::runtime_error("There's no room to put the door there!");
            // Teleport all players to reload world
            action::quit_to_exit(event, "", true);
            action::join_request(event, "", world->name);
            break;
        }
        case 822: // Water Bucket
        {
            if (block.state4 & S_FIRE)
                remove_fire(event, state, block, *world);
            else
                block.state4 ^= S_WATER;
            break;
        }
        case 3062: // Pocket Lighter
        {
            if (block.fg == 0 && block.bg == 0)
                throw std::runtime_error("There's nothing to burn!");
            if (block.state4 & (S_FIRE | S_WATER))
                return;  // Can't light water or existing fire
            
            block.state4 |= S_FIRE;
            particle = 0x96;  // Fire particle
            
            if (block.fg == 3090)  // Highly Combustible Box
            {
                block.fg = 3128;  // Combusted Box
                // @todo: Handle recipes
            }
            break;
        }
        case 3400: // Love Potion #8
        {
            if (block.fg != 10) return;  // Only works on Rock
            block.fg = 392;  // Transform to Heartstone
            particle = 0x2c;
            break;
        }
        case 1488: // Experience Potion
        {
            peer->add_xp(event, 10000);
            break;
        }
        case 408: // Duct Tape
        {
            peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p)
            {
                ::peer *_p = static_cast<::peer*>(p.data);
                if (state.punch == peer->pos)
                {
                    _p->state ^= S_DUCT_TAPE;
                    on::SetClothing(p);
                }
            });
            break;
        }
        // Paint buckets set state4 flags and particle colors
        case 3478: // Red
        {
            block.state4 |= S_RED;
            color = 0x0000ff00, particle = 0xa8;
            break;
        }
        // ... other paint colors
    }
    
    // Send particle effect
    if (particle > 0.0f)
    {
        state_visuals(*event.peer, ::state{
            .type = 0x11,  // PACKET_SEND_PARTICLE_EFFECT
            .pos = state.punch,
            .speed = { color, particle }
        });
    }
    
    send_tile_update(event, state, block, *world);
    modify_item_inventory(event, ::slot(item->id, -1));  // Consume item
    peer->add_xp(event, 1);
}
```

### Block Placement System - Deep Dive

#### State Structure for Placement

**KEY FINDING**: There is **NO separate `.place` field** in the state structure. Both punching AND placing use the **same `state.punch`** coordinate field!

```cpp
class state {
    int id{};      // Determines action type:
                   //   18 = FIST (punching)
                   //   32 = WRENCH (interacting)
                   //   Any other ID = PLACING that item
    ::pos pos{0,0};      // Player position
    ::pos punch{0,0};    // BOTH punching AND placing position!
};
```

#### How Placement is Detected

```cpp
// Line 28: Item determination
auto item = std::ranges::find(items, 
    (state.id != 32 && state.id != 18) ? state.id :  // Placing item
    (block.fg != 0) ? block.fg : block.bg,           // Or block being punched
    &::item::id);

// Line 602: Placement branch
else // @note placing a block
{
    // This branch executes when state.id != 18 (fist) AND state.id != 32 (wrench)
    // Meaning: state.id contains an ITEM ID to place
}
```

#### Complete Placement Flow

```cpp
else  // Placing a block (state.id != 18 && state.id != 32)
{
    // === STEP 1: Check if placing ON existing block ===
    if (block.fg != 0)  // Something already exists here
    {
        bool update_tile = false;
        
        // Check what's underneath
        switch (items[block.fg].type)
        {
            case type::DISPLAY_BLOCK:
            {
                // Add display item to world's display list
                world->displays.emplace_back(::display(item->id, state.punch));
                update_tile = true;
                break;
            }
            case type::SEED:  // Splicing system
            {
                // Check for valid splice combination
                for (::item &splice_item : items)
                {
                    // Try both orders (A+B or B+A)
                    if ((splice_item.splice[0] == state.id && splice_item.splice[1] == block.fg) ||
                        (splice_item.splice[1] == state.id && splice_item.splice[0] == block.fg))
                    {
                        // Found valid splice!
                        auto splice0 = std::ranges::find(items, splice_item.splice[0], &::item::id);
                        auto splice1 = std::ranges::find(items, splice_item.splice[1], &::item::id);
                        
                        packet::create(*event.peer, false, 0, {
                            "OnTalkBubble",
                            peer->netid,
                            std::format("`w{}`` and `w{}`` have been spliced to make a `${} Tree``!",
                                splice0->raw_name, splice1->raw_name, 
                                splice_item.raw_name.substr(0, splice_item.raw_name.length()-5)).c_str(),
                            0u, 1u
                        });
                        
                        block.tick = steady_clock::now();
                        block.fg = splice_item.id;  // Transform to new tree
                        update_tile = true;
                        break;
                    }
                }
                break;
            }
        }
        
        if (update_tile)
            send_tile_update(event, std::move(state), block, *world);
        return;  // Exit early, no placement on top
    }
    
    // === STEP 2: Collision check ===
    if (item->collision == collision::FULL)
    {
        // Can't place in the same tile as player
        if (state.punch.x == state.pos.x && state.punch.y == state.pos.y)
            return;
    }
    
    // === STEP 3: Special block handling ===
    switch (item->type)
    {
        case type::LOCK:
        {
            if (is_tile_lock(item->id)) break;  // Small locks can stack
            
            if (!world->owner)  // No owner yet
            {
                world->owner = peer->user_id;
                lock_visuals = true;
                
                // Update player prefix to green (world owner)
                if (!peer->role)
                {
                    peer->prefix.front() = '2';
                    on::NameChanged(event);
                }
                
                // Add to player's world list (rotating buffer of 200)
                if (std::ranges::find(peer->my_worlds, world->name) == peer->my_worlds.end())
                {
                    std::ranges::rotate(peer->my_worlds, peer->my_worlds.begin() + 1);
                    peer->my_worlds.back() = world->name;
                }
                
                // Announce to all players in world
                std::string msg = std::format(
                    "`5[```w{}`` has been `$World Locked`` by {}`5]``",
                    world->name, peer->ltoken[0]);
                peers(peer->recent_worlds.back(), PEER_SAME_WORLD,
                    [&peer, msg](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnTalkBubble", peer->netid, msg.c_str() });
                        packet::create(p, false, 0, { "OnConsoleMessage", msg.c_str() });
                    });
            }
            else
                throw std::runtime_error("Only one `$World Lock`` can be placed in a world!");
            break;
        }
        case type::ENTRANCE:
        {
            block.state3 |= S_PUBLIC;  // Default to public access
            break;
        }
        case type::PROVIDER:
        case type::SEED:
        {
            block.tick = steady_clock::now();  // Start production/growth timer
            if (item->type == type::SEED)
                block.state3 |= 0x11;  // Growth state flag
            break;
        }
    }
    
    // === STEP 4: Set placement direction ===
    // Direction based on which way player is facing
    block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;
    
    // === STEP 5: Place the block ===
    // Background items go to bg layer, everything else to fg layer
    (item->type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
    
    // === STEP 6: Remove from inventory ===
    peer->emplace(::slot(item->id, -1));  // Negative count = remove
}

// === STEP 7: Send visuals to all players ===
state.netid = peer->netid;
state_visuals(*event.peer, std::move(state));

// If world lock was placed, send special lock packet
if (lock_visuals)
{
    state_visuals(*event.peer, ::state{
        .type = 0x0f,  // PACKET_SEND_LOCK
        .netid = world->owner,
        .peer_state = 0x08,
        .id = state.id,
        .punch = state.punch
    });
}
```

#### Placement vs Punching Decision Tree

```
state.id value determines action:
│
├─ state.id == 18 (FIST)
│   └─ Action: PUNCH/BREAK block at state.punch
│
├─ state.id == 32 (WRENCH)
│   └─ Action: INTERACT with block (show dialog)
│
└─ state.id == ANY OTHER VALUE
    └─ Action: PLACE item (state.id = item ID) at state.punch
        ├─ If block.fg != 0 → Check for splicing/display
        ├─ If collision::FULL → Check player position
        ├─ Apply special rules (LOCK, ENTRANCE, SEED, PROVIDER)
        ├─ Set direction (S_LEFT or S_RIGHT)
        ├─ Place in fg or bg layer
        └─ Remove from inventory
```

#### Multi-Place Implementation (Like Rayman's Fist Multi-Punch)

**KEY INSIGHT**: The code pattern used for Rayman's Fist multi-punch **WORKS EXACTLY THE SAME** for multi-place!

```cpp
// From Rayman's Fist multi-punch (line ~50-65)
if (peer->clothing[hand] == 5480) // Rayman's Fist
{
    static bool punch{};
    
    if (!punch)
    {
        punch = true;
        
        int x1_nabor = (peer->facing_left) ? state.punch.x-1 : state.punch.x+1;
        int x2_nabor = (peer->facing_left) ? state.punch.x-2 : state.punch.x+2;
        int x3_nabor = (peer->facing_left) ? state.punch.x-3 : state.punch.x+3;
        int x4_nabor = (peer->facing_left) ? state.punch.x-4 : state.punch.x+4;
        
        ::state x1_state = state;
        x1_state.punch = {x1_nabor, x1_state.punch.y};
        tile_change(event, std::move(x1_state));  // ← RECURSIVE CALL
        
        ::state x2_state = state;
        x2_state.punch = {x2_nabor, x2_state.punch.y};
        tile_change(event, std::move(x2_state));
        
        ::state x3_state = state;
        x3_state.punch = {x3_nabor, x3_state.punch.y};
        tile_change(event, std::move(x3_state));
        
        ::state x4_state = state;
        x4_state.punch = {x4_nabor, x4_state.punch.y};
        tile_change(event, std::move(x4_state));
        
        punch = false;
    }
}
```

**Why This Works for Both Punch AND Place:**

The **exact same code pattern** works for multi-place because:

1. **`state.id` is NOT modified** - Only `state.punch` changes
2. **`tile_change()` branches based on `state.id`**:
   - If original `state.id == 18` → All recursive calls punch
   - If original `state.id == ITEM_ID` → All recursive calls place that item

**Complete Multi-Place Implementation:**

```cpp
// Add this in tile_change.cpp, in the placement branch (else after state.id == 32)
// Example: Builder's Glove - Places 4 blocks horizontally

else  // Placing a block
{
    // === MULTI-PLACE CHECK ===
    // Check if player has Builder's Glove equipped (replace 9999 with actual ID)
    if (peer->clothing[hand] == 9999)
    {
        static bool placing{};
        
        if (!placing)
        {
            placing = true;
            
            // Calculate 4 positions in facing direction
            int x1_nabor = (peer->facing_left) ? state.punch.x-1 : state.punch.x+1;
            int x2_nabor = (peer->facing_left) ? state.punch.x-2 : state.punch.x+2;
            int x3_nabor = (peer->facing_left) ? state.punch.x-3 : state.punch.x+3;
            int x4_nabor = (peer->facing_left) ? state.punch.x-4 : state.punch.x+4;
            
            // Create state copies with modified punch positions
            ::state x1_state = state;
            x1_state.punch = {x1_nabor, x1_state.punch.y};
            tile_change(event, std::move(x1_state));
            
            ::state x2_state = state;
            x2_state.punch = {x2_nabor, x2_state.punch.y};
            tile_change(event, std::move(x2_state));
            
            ::state x3_state = state;
            x3_state.punch = {x3_nabor, x3_state.punch.y};
            tile_change(event, std::move(x3_state));
            
            ::state x4_state = state;
            x4_state.punch = {x4_nabor, x4_state.punch.y};
            tile_change(event, std::move(x4_state));
            
            placing = false;
            return; // Multi-place sudah handle semua placement
        }
    }
    
    // === NORMAL PLACEMENT CODE CONTINUES ===
    if (block.fg != 0)
    {
        // ... existing code ...
    }
    // ... rest of placement logic ...
}
```

#### Code Pattern Analysis

```cpp
// The magic pattern:
::state x_state = state;           // 1. Copy entire state
x_state.punch = {x, y};            // 2. Modify ONLY punch position
tile_change(event, std::move(x_state)); // 3. Recursive call with new state
```

**What Gets Copied:**
- `state.id` ← **Preserved** (determines punch vs place)
- `state.pos` ← Player position (for collision checks)
- `state.punch` ← **Modified** (new placement position)
- `state.peer_state` ← Player state flags
- All other fields

**What Happens in Recursive Call:**

```
Original call: tile_change(event, original_state)
  state.id = 242 (World Lock)
  state.punch = {10, 36}
  └─ Enters placement branch (state.id != 18 && != 32)
     └─ Multi-place check: Has Builder's Glove
        └─ Creates x1_state with punch = {11, 36}
           └─ Recursive: tile_change(event, x1_state)
              state.id = 242 (SAME!)
              state.punch = {11, 36} (CHANGED!)
              └─ Enters placement branch (SAME!)
                 └─ Places World Lock at {11, 36}
```

#### Important Considerations for Multi-Place

1. **Inventory Validation** (BEFORE multi-place):
```cpp
// Check if player has enough items
auto slot = std::ranges::find(peer->slots, state.id, &::slot::id);
if (slot == peer->slots.end() || slot->count < 4)
{
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage", "Not enough items to multi-place!"
    });
    return;
}
```

2. **Bounds Checking**:
```cpp
// Validate each position is within world bounds (100x60)
if (x_nabor < 0 || x_nabor >= 100)
    continue;  // Skip out-of-bounds positions

if (y_nabor < 0 || y_nabor >= 60)
    continue;  // Skip out-of-bounds positions
```

3. **Static Flag Pattern**:
```cpp
static bool placing{};  // Prevents infinite recursion

if (!placing)
{
    placing = true;
    // ... multi-place logic ...
    placing = false;  // Reset after completion
}
```

4. **Direction Consistency**:
```cpp
// All placed blocks face the same direction
// Because peer->facing_left is read from peer object, not state
block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;
```

#### Key Differences: Multi-Punch vs Multi-Place

| Aspect | Multi-Punch (Rayman's Fist) | Multi-Place (Builder's Glove) |
|--------|----------------------------|-------------------------------|
| **state.id** | 18 (FIST) | Item ID being placed |
| **Branch** | `if (state.id == 18)` | `else` (placement branch) |
| **Position field** | `state.punch` | `state.punch` (SAME!) |
| **Inventory** | No change | Remove item for each placement |
| **Direction** | Not set | `block.state3 |= S_LEFT/S_RIGHT` |
| **Visuals** | Damage animation | Placement animation |

#### Important Notes for Multi-Place

1. **Inventory Check**: Ensure player has enough items BEFORE placing multiple:
```cpp
// Check if player has enough items
auto slot = std::ranges::find(peer->slots, state.id, &::slot::id);
if (slot == peer->slots.end() || slot->count < 4)
    return;  // Not enough items
```

2. **Position Validation**: Each placement position must be validated:
```cpp
// Check each position is valid
if (x_nabor < 0 || x_nabor >= 100)  // World bounds
    continue;  // Skip invalid positions
```

3. **Collision Handling**: Each placement needs collision check:
```cpp
// The existing collision check in tile_change will handle this
if (item->collision == collision::FULL)
{
    if (place_state.punch.x == state.pos.x && 
        place_state.punch.y == state.pos.y)
        continue;  // Skip this position
}
```

4. **Direction Consistency**: All placed blocks should face same direction:
```cpp
// All blocks will inherit peer->facing_left from the original state
block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;
```

#### Placement Layer System

```cpp
// Foreground vs Background placement
(item->type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
```

**Background Items** (go to `block.bg`):
- Item type == `type::BACKGROUND` (0x12)

**Foreground Items** (go to `block.fg`):
- All other types (LOCKS, SEEDS, PROVIDERS, etc.)

#### State3 Direction Flags

```cpp
block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;

enum wstate3 : u_char
{
    S_RIGHT =  0x00,  // 0000 0000 - Facing right (default)
    S_LEFT =   0x20,  // 0010 0000 - Facing left
    // ... other flags
};
```

This affects:
- Door/Portal orientation
- Sign text direction
- Platform direction
- Visual appearance of certain blocks

### Wrench Interaction (state.id == 32)

When using a wrench on blocks, different dialogs open:

| Block Type | Dialog | Purpose |
|------------|--------|---------|
| LOCK | `lock_edit` | Configure access, public/private, music, home world |
| DOOR/PORTAL | `door_edit` | Set label, destination, password |
| SIGN | `sign_edit` | Edit sign text |
| ENTRANCE | `gateway_edit` | Toggle public access |
| DISPLAY_BLOCK | (incomplete) | Configure display |
| VENDING_MACHINE | `vending` | Stock items, upgrade to DigiVend |

### Permission System

```cpp
// Permission check before allowing interaction
if (!(item->cat & CAT_PUBLIC))
{
    if ((world->owner && !world->is_public && !peer->role) &&
        (peer->user_id != world->owner && !std::ranges::contains(world->admin, peer->user_id)))
        return;  // No permission
}
```

### Block State Flags

#### State3 (Direction & Toggle)
```cpp
enum wstate3 : u_char
{
    S_RIGHT =  0x00,   // Facing right
    S_LOCKED = 0x10,   // Locked state
    S_LEFT =   0x20,   // Facing left
    S_TOGGLE = 0x40,   // Toggled on/off
    S_PUBLIC = 0x80    // Public access
};
```

#### State4 (Environmental Effects)
```cpp
enum wstate4 : u_char
{
    S_WATER =    0x04,  // Water tile
    S_GLUE =     0x08,  // Block glue
    S_FIRE =     0x10,  // On fire
    S_RED =      0x20,  // Red paint
    S_GREEN =    0x40,  // Green paint
    S_YELLOW =   S_RED | S_GREEN,
    S_BLUE =     0x80,  // Blue paint
    S_AQUA =     S_GREEN | S_BLUE,
    S_PURPLE =   S_RED | S_BLUE,
    S_CHARCOAL = S_RED | S_GREEN | S_BLUE,
    S_VANISH =   S_RED | S_YELLOW | S_GREEN | S_AQUA | S_BLUE | S_PURPLE | S_CHARCOAL
};
```

### Coordinate System

```cpp
// Convert 2D coordinates to 1D array index
#define cord(x,y) (y * 100 + x)

// World is 100x60 = 6000 blocks
::block &block = world->blocks[cord(state.punch.x, state.punch.y)];
```

### Error Handling

All exceptions are caught and displayed as talk bubbles:

```cpp
catch (const std::exception& exc)
{
    if (exc.what() && *exc.what())
        packet::create(*event.peer, false, 0, {
            "OnTalkBubble",
            peer->netid,
            exc.what(),
            0u,
            1u  // Show once
        });
    return;
}
```

Common error messages:
- "It's too strong to break."
- "(stand over and punch to use)"
- "You can't drop that."
- "There's nothing to burn!"
- "Only one `$World Lock`` can be placed in a world"

---

## Item Activation System

### Item Activate (`include/state/item_activate.cpp`)

Handles using items from inventory (not placing them):

```cpp
void item_activate(ENetEvent& event, state state)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    auto item = std::ranges::find(items, state.id, &::item::id);
    
    // Clothing items
    if (item->cloth_type != clothing::none)
    {
        float &current_cloth = peer->clothing[item->cloth_type];
        current_cloth = (current_cloth == state.id) ? 0 : state.id;  // Toggle
        
        // Update punch effects from clothing
        peer->punch_effect = 0;
        for (float cloth : peer->clothing)
        {
            u_short punch_id = get_punch_id(static_cast<u_int>(cloth));
            if (punch_id != 0)
                peer->punch_effect = punch_id;
        }
        
        packet::create(*event.peer, true, 0, { "OnEquipNewItem", state.id });
        on::SetClothing(*event.peer);
    }
    // Currency conversion
    else if (state.id == 242 || state.id == 1796)  // WL/DL
    {
        if (item->id == 242 && item->count >= 100)  // 100 WL → 1 DL
        {
            const short nokori = modify_item_inventory(event, {1796, 1});
            if (nokori == 0)
            {
                modify_item_inventory(event, {item->id, -100});
                // Show compression message
            }
        }
        else if (item->id == 1796 && item->count >= 1)  // 1 DL → 100 WL
        {
            const short nokori = modify_item_inventory(event, {242, 100});
            short hyaku = 100 - nokori;
            if (hyaku == 100)
            {
                modify_item_inventory(event, {item->id, -1});
                // Show shatter message
            }
            else
                modify_item_inventory(event, ::slot(242, -hyaku));  // Return excess
        }
    }
    else if (state.id == 1486 || state.id == 6802)  // GT/GGT
    {
        // Similar conversion logic for Growtokens
    }
}
```

### Item Activate Object (`include/state/item_activate_object.cpp`)

Handles picking up dropped items:

```cpp
void item_activate_object(ENetEvent& event, state state)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    auto world = std::ranges::find(worlds, peer->recent_worlds.back(), &::world::name);
    auto object = std::ranges::find(world->objects, state.id, &::object::uid);
    auto item = std::ranges::find(items, object->id, &::item::id);
    
    if (item->type != type::GEM)
    {
        // Show collection message with rarity for rare items
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            (item->rarity >= 999) ?
                std::format("Collected `w{} {}``.", object->count, item->raw_name).c_str() :
                std::format("Collected `w{} {}``. Rarity: `w{}``", 
                    object->count, item->raw_name, item->rarity).c_str()
        });
        
        // Add to inventory
        object->count = peer->emplace(::slot(object->id, object->count));
    }
    else
    {
        // Gems go directly to gem count
        peer->gems += object->count;
        object->count = 0;
        on::SetBux(event);  // Update gem display
    }
    
    // Remove or update drop
    item_change_object(event, ::slot(object->id, object->count), object->pos, state.id);
    if (object->count == 0)
        world->objects.erase(object);
}
```

---

## Peer Pointer System - Complete Guide

### Understanding `event.peer->data`

**Key Concept:** ENet's `ENetPeer` structure has a `void* data` field that Gurotopia uses to store custom `peer` class instances.

**Type Casting Pattern:**
```cpp
::peer *peer = static_cast<::peer*>(event.peer->data);
```

This is the **FIRST line** in almost every function that needs player data!

### Why Use `static_cast<::peer*>`?

**ENet Architecture:**
- ENet provides `ENetPeer*` in events
- `ENetPeer::data` is `void*` (generic pointer)
- Gurotopia stores custom `peer` class in this field
- Must cast back to `::peer*` to access player data

**Memory Management:**
```cpp
// Connection: Allocate peer data
// File: event_type/connect.cpp
event.peer->data = new peer();

// Disconnection: Free peer data  
// File: action/quit.cpp
if (event.peer->data != nullptr)
{
    delete static_cast<::peer*>(event.peer->data);
    event.peer->data = nullptr;
}
```

---

### Accessing Player Information

Once you have the peer pointer, you can access ALL player data:

```cpp
::peer *peer = static_cast<::peer*>(event.peer->data);

// === IDENTITY ===
peer->netid              // int - Network ID in current world
peer->user_id            // int - Unique user ID
peer->ltoken[0]          // std::string - GrowID (username)
peer->ltoken[1]          // std::string - Password (from ltoken)
peer->game_version       // std::string - Client version
peer->country            // std::string - Country code

// === APPEARANCE ===
peer->prefix             // std::string - Name prefix/color
peer->role               // u_char - PLAYER(0), MOD(1), DEV(2)
peer->clothing           // std::array<float, 10> - Equipped items
peer->punch_effect       // u_char - Active punch effect
peer->skin_color         // u_int - Skin color value

// === STATE ===
peer->state              // int - Player state flags (ghost, etc)
peer->billboard          // Billboard - Name tag settings
peer->pos                // ::pos - Current position {x, y}
peer->rest_pos           // ::pos - Respawn position
peer->facing_left        // bool - Facing direction

// === INVENTORY ===
peer->slot_size          // short - Total backpack slots
peer->slots              // std::vector<slot> - Inventory items
peer->fav                // std::vector<short> - Favorite items

// === CURRENCY & PROGRESS ===
peer->gems               // signed - Gem count
peer->level[0]           // u_short - Current level
peer->level[1]           // u_short - Current XP

// === WORLDS ===
peer->recent_worlds      // std::array<std::string, 6> - Recent worlds
peer->my_worlds          // std::array<std::string, 200> - Owned worlds

// === CHAT ===
peer->messages           // std::deque<time_point> - Anti-spam queue

// === SOCIAL ===
peer->friends            // std::array<Friend, 25> - Friend list

// === STATS ===
peer->fires_removed      // u_short - Fires extinguished
peer->gbc_pity           // u_short - GBC pity counter
```

---

### Common Access Patterns

#### Pattern 1: Accessing Current Peer

**Most Common Pattern:**
```cpp
void some_function(ENetEvent& event, ...)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    // Now access peer data
    std::string name = peer->ltoken[0];
    int level = peer->level[0];
    // ... etc
}
```

**Examples from Codebase:**

**File:** `action/tankIDName.cpp`
```cpp
void action::tankIDName(ENetEvent& event, const std::string& header)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes.empty() || pipes.size() < 41zu) 
        enet_peer_disconnect_later(event.peer, 0);

    for (std::size_t i = 0; i < pipes.size(); ++i)
    {
        if      (pipes[i] == "tankIDName")   peer->ltoken[0] = pipes[i+1];
        else if (pipes[i] == "game_version") peer->game_version = pipes[i+1];
        else if (pipes[i] == "country")      peer->country = pipes[i+1];
        else if (pipes[i] == "user")         peer->user_id = std::stoi(pipes[i+1]);
    }
    peer->read(peer->ltoken[0]);  // Load from database
}
```

---

#### Pattern 2: Accessing Other Peers in World

**Using `peers()` function with lambda:**

```cpp
void some_function(ENetEvent& event)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    // Iterate all peers in same world
    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, 
        [&peer](ENetPeer& p)
    {
        ::peer *_p = static_cast<::peer*>(p.data);
        
        // Access other peer's data
        if (_p->user_id != peer->user_id)  // Not self
        {
            std::string other_name = _p->ltoken[0];
            int other_level = _p->level[0];
            // ... do something with other peer
        }
    });
}
```

**Example from Codebase:**

**File:** `commands/who.cpp`
```cpp
void who(ENetEvent& event, const std::string_view text)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    std::vector<std::string> names;
    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, 
        [&peer, event, &names](ENetPeer& p)
    {
        ::peer *_p = static_cast<::peer*>(p.data);
        
        std::string full_name = std::format("`{}{}", 
            _p->prefix, _p->ltoken[0]);
        
        if (_p->user_id != peer->user_id)
        {
            packet::create(*event.peer, false, 0, { 
                "OnTalkBubble", _p->netid, full_name.c_str(), 1u 
            });
        }
        names.emplace_back(std::move(full_name));
    });
    
    packet::action(*event.peer, "log", std::format(
        "msg|`wWho's in `${}``:`` {}``",
        peer->recent_worlds.back(), join(names, ", ")));
}
```

---

#### Pattern 3: Accessing Specific Peer by Name

**Searching through all peers:**

```cpp
std::string target_name = "PlayerName";
ENetPeer* target_peer = nullptr;
::peer* target_data = nullptr;

peers("", PEER_ALL, [&target_name, &target_peer, &target_data](ENetPeer& p)
{
    ::peer *_p = static_cast<::peer*>(p.data);
    if (_p->ltoken[0] == target_name)
    {
        target_peer = &p;
        target_data = _p;
        return;  // Found, can stop searching
    }
});

if (target_peer != nullptr)
{
    // Found the player!
    // Use target_peer-> or target_data->
}
```

**Example from Codebase:**

**File:** `commands/edit.cpp`
```cpp
void edit(ENetEvent& event, const std::string_view text)
{
    std::string name{ text.substr(sizeof("edit ")-1) };
    
    u_char is_online{};
    std::string fmt = "set_default_color|`o\n"
        "add_label|big|Username: `w{0}`` (`s{1}``)|left\n"
        "embed_data|name|{0}\n"
        "add_text_input|role|Role: |{2}|3|\n"
        "add_text_input|level|Level: |{3}|3|\n"
        "add_text_input|gems|Gems: |{4}|10|\n"
        "end_dialog|peer_edit|Close|`2Edit|\n";

    peers("", PEER_ALL, [&event, name, &is_online, fmt](ENetPeer& p)
    {
        ::peer *_p = static_cast<::peer*>(p.data);
        if (_p->ltoken[0] == name)
        {
            is_online = 1;
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::vformat(fmt, std::make_format_args(
                    name, "`2Online", _p->role, _p->level.front(), _p->gems, is_online)).c_str()
            });
            return;
        }
    });
    
    if (!is_online)
    {
        // Player offline, load from database
        if (!::peer().exists(name))
        {
            packet::create(*event.peer, false, 0, { 
                "OnConsoleMessage", "`4player doesn't exist``" 
            });
            return;
        }
        peer &offline = ::peer().read(name);
        // ... show dialog with offline data
    }
}
```

---

### Peer Data Fields - Detailed Reference

#### Identity Fields

**`ltoken[0]` - GrowID (Username)**
```cpp
::peer *peer = static_cast<::peer*>(event.peer->data);
std::string username = peer->ltoken[0];

// Usage examples:
packet::create(*event.peer, false, 0, { 
    "OnTalkBubble", peer->netid, 
    std::format("Hello {}!", peer->ltoken[0]).c_str() 
});
```

**`user_id` - Unique User ID**
```cpp
int uid = peer->user_id;

// Check if player is world owner:
if (peer->user_id == world->owner)
{
    // Player owns this world
}
```

**`netid` - Network ID (World-Specific)**
```cpp
int nid = peer->netid;

// Used in packets to identify player in world:
packet::create(*event.peer, false, 0, { 
    "OnSpawn", peer->netid, ... 
});

// Reset when leaving world:
peer->netid = 0;
```

---

#### Appearance Fields

**`prefix` - Name Prefix/Color**
```cpp
std::string prefix = peer->prefix;

// Prefix values:
// "w" - White (default player)
// "2" - Green (world owner)
// "c" - Cyan (world admin)
// "#@" - DOU_ZUN
// "8@" - BAN_SHENG

// Usage in formatted name:
std::string display_name = std::format("`{}{}``", 
    peer->prefix, peer->ltoken[0]);
// Result: `wPlayerName`` (white colored name)
```

**`role` - Player Role**
```cpp
u_char role = peer->role;

// Check role:
if (peer->role == DOU_ZUN)
{
    // Player is DOU_ZUN
}

if (peer->role >= BAN_SHENG)
{
    // Player is BAN_SHENG or higher
}

// Set prefix based on role:
peer->prefix = (peer->role == DOU_ZUN) ? "#@" : 
               (peer->role == BAN_SHENG) ? "8@" : "w";
```

**`clothing` - Equipped Items**
```cpp
std::array<float, 10zu> clothing = peer->clothing;

// Clothing indices (enum clothing):
// 0 - hair
// 1 - shirt
// 2 - legs
// 3 - feet
// 4 - face
// 5 - hand
// 6 - back
// 7 - head
// 8 - charm
// 9 - ances (aura)

// Check what's equipped on hand:
if (peer->clothing[hand] == 3066)  // Fire hose
{
    // Player has fire hose equipped
}

// Set clothing:
peer->clothing[hair] = 1234.0f;  // Set hair to item ID 1234
```

**`punch_effect` - Active Punch Effect**
```cpp
u_char effect = peer->punch_effect;

// Set punch effect:
peer->punch_effect = 17;  // Fire hose effect

// Used in SetClothing packet:
state.type = 0x14 | ((0x808000 + peer->punch_effect) << 8);
```

**`skin_color` - Skin Color**
```cpp
u_int color = peer->skin_color;

// Default: 2527912447
// Set custom color:
peer->skin_color = 4278190080;  // Red skin

// Ghost mode uses -140:
(_p->state & S_GHOST) ? -140 : _p->skin_color
```

---

#### State Fields

**`state` - Player State Flags**
```cpp
int state_flags = peer->state;

// State flags (enum pstate):
// S_GHOST = 0x00000001
// S_DOUBLE_JUMP = 0x00000002
// S_DUCT_TAPE = 0x00002000

// Toggle ghost mode:
peer->state ^= S_GHOST;

// Check if ghost:
if (peer->state & S_GHOST)
{
    // Player is ghost (invisible)
}

// Check if has duct tape:
if (peer->state & S_DUCT_TAPE)
{
    // Player has duct tape effect
}
```

**`pos` - Current Position**
```cpp
::pos position = peer->pos;

// Access coordinates:
int x = position.x;
int y = position.y;

// Get float coordinates:
float fx = position.f_x();  // x * 32.0f
float fy = position.f_y();  // y * 32.0f

// Update position:
peer->pos = ::pos(10, 20);
```

**`rest_pos` - Respawn Position**
```cpp
::pos respawn = peer->rest_pos;

// Set respawn (checkpoint):
peer->rest_pos = state.punch;

// Use in respawn packet:
packet::create(*event.peer, true, 1900, {
    "OnSetPos",
    std::vector<float>{peer->rest_pos.f_x(), peer->rest_pos.f_y()}
});
```

**`facing_left` - Facing Direction**
```cpp
bool facing = peer->facing_left;

// Use for block placement direction:
block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;

// Calculate multi-punch positions:
int x_nabor = (peer->facing_left) ? 
    state.punch.x-1 : state.punch.x+1;
```

---

#### Inventory Fields

**`slot_size` - Total Backpack Slots**
```cpp
short slots = peer->slot_size;

// Default: 16
// Upgrade formula: 100 * No * No - 200 * No + 200
// Max: 38 upgrades (396 slots)

// Display in wrench menu:
std::format("You have `w{}`` backpack slots.", _p->slot_size)
```

**`slots` - Inventory Items**
```cpp
std::vector<slot> inventory = peer->slots;

// Each slot contains:
// - id: Item ID
// - count: Item count (max 200)

// Search for item:
auto it = std::ranges::find(peer->slots, 242, &::slot::id);
if (it != peer->slots.end())
{
    // Found World Lock
    int count = it->count;
}

// Iterate inventory:
for (const ::slot &item : peer->slots)
{
    std::printf("Item %d: count %d\n", item.id, item.count);
}

// Add item:
peer->emplace(::slot(242, 100));  // Add 100 World Locks

// Remove item:
peer->emplace(::slot(242, -10));  // Remove 10 World Locks
```

**`fav` - Favorite Items**
```cpp
std::vector<short> favorites = peer->fav;

// Check if item is favorited:
auto it = std::ranges::find(peer->fav, item_id);
bool is_fav = it != peer->fav.end();

// Add to favorites:
peer->fav.emplace_back(item_id);

// Remove from favorites:
peer->fav.erase(it);

// Max 20 favorites:
if (peer->fav.size() >= 20 && !is_fav)
{
    packet::create(*event.peer, false, 0, { 
        "OnTalkBubble", peer->netid, 
        "You cannot favorite any more items." 
    });
}
```

---

#### Currency & Progress Fields

**`gems` - Gem Count**
```cpp
signed gems = peer->gems;

// Clamp to valid range:
peer->gems = std::clamp(peer->gems, 0, std::numeric_limits<signed>::max());

// Add gems:
peer->gems += 1000;

// Update display:
on::SetBux(event);  // Sends OnSetBux packet
```

**`level` - Level and XP**
```cpp
std::array<u_short, 2zu> level = peer->level;

// Access:
u_short current_level = level[0];
u_short current_xp = level[1];

// Add XP (handles level up automatically):
peer->add_xp(event, 1000);

// Set level directly (admin):
peer->level[0] = 125;  // Set to max level
on::CountryState(event);  // Update blue name
```

---

#### World Fields

**`recent_worlds` - Recently Visited Worlds**
```cpp
std::array<std::string, 6zu> recent = peer->recent_worlds;

// Get current world:
std::string current_world = peer->recent_worlds.back();

// Rotate when entering new world:
std::string *w_name = std::ranges::find(peer->recent_worlds, world.name);
std::string *first = w_name != peer->recent_worlds.end() ? 
                     w_name : peer->recent_worlds.begin();
std::rotate(first, first + 1, peer->recent_worlds.end());
peer->recent_worlds.back() = world.name;
```

**`my_worlds` - Owned Worlds**
```cpp
std::array<std::string, 200zu> owned = peer->my_worlds;

// Add world when placing lock:
if (std::ranges::find(peer->my_worlds, world.name) == peer->my_worlds.end())
{
    std::ranges::rotate(peer->my_worlds, peer->my_worlds.begin() + 1);
    peer->my_worlds.back() = world.name;
}
```

---

### Complete Example: Accessing Peer Data

**Scenario:** Create a command that shows player info

```cpp
void cmd_playerinfo(ENetEvent& event, const std::string_view text)
{
    // Step 1: Get current peer
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    // Step 2: Access various fields
    std::string name = peer->ltoken[0];
    int uid = peer->user_id;
    int level = peer->level[0];
    int xp = peer->level[1];
    int gems = peer->gems;
    std::string prefix = peer->prefix;
    u_char role = peer->role;
    int slot_size = peer->slot_size;
    int inventory_size = peer->slots.size();
    
    // Step 3: Format and send info
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format(
            "`5=== Player Info ==="
            "`n`wName``: `{}{}``"
            "`n`wLevel``: `${}{}``"
            "`n`wXP``: ``{}/{}"
            "`n`wGems``: ``${}"
            "`n`wRole``: ``{}"
            "`n`wSlots``: ``{}/{}",
            prefix, name,
            prefix, name,
            level,
            xp, 50 * (level * level + 2),  // XP needed
            gems,
            (role == 0) ? "Player" : (role == 1) ? "DOU_ZUN" : "BAN_SHENG",
            inventory_size, slot_size
        ).c_str()
    });
}
```

---

### Important Notes

1. **Always Check for Null:**
```cpp
if (event.peer == nullptr) return;
if (event.peer->data == nullptr) return;

::peer *peer = static_cast<::peer*>(event.peer->data);
```

2. **Peer Data Lifetime:**
- Created on connect (`event.peer->data = new peer()`)
- Destroyed on disconnect (`delete static_cast<::peer*>(event.peer->data)`)
- Accessing after disconnect = undefined behavior!

3. **Thread Safety:**
- Peer data is NOT thread-safe
- Don't access peer data from multiple threads simultaneously
- HTTPS listener runs on separate thread - don't access peer data there

4. **Database Persistence:**
- Peer data is saved to database when peer object is destroyed
- Changes to `peer->` fields are automatically saved
- No explicit save() call needed

5. **Memory Management:**
```cpp
// GOOD: ENet manages peer lifetime
event.peer->data = new peer();  // On connect
delete static_cast<::peer*>(event.peer->data);  // On disconnect

// BAD: Don't delete peer data manually!
// Don't store peer pointers long-term (they become invalid)
```

---

### Quick Reference Table

| Field | Type | Description | Example Access |
|-------|------|-------------|----------------|
| `ltoken[0]` | `std::string` | Username | `peer->ltoken[0]` |
| `user_id` | `int` | Unique ID | `peer->user_id` |
| `netid` | `int` | Network ID | `peer->netid` |
| `prefix` | `std::string` | Name prefix | `peer->prefix` |
| `role` | `u_char` | Role (0-2) | `peer->role` |
| `level[0]` | `u_short` | Level | `peer->level[0]` |
| `level[1]` | `u_short` | XP | `peer->level[1]` |
| `gems` | `signed` | Gems | `peer->gems` |
| `slot_size` | `short` | Backpack size | `peer->slot_size` |
| `slots` | `vector<slot>` | Inventory | `peer->slots` |
| `clothing` | `array<float,10>` | Equipment | `peer->clothing[hand]` |
| `pos` | `::pos` | Position | `peer->pos.x` |
| `rest_pos` | `::pos` | Respawn | `peer->rest_pos` |
| `facing_left` | `bool` | Direction | `peer->facing_left` |
| `state` | `int` | State flags | `peer->state & S_GHOST` |
| `recent_worlds` | `array<string,6>` | Recent worlds | `peer->recent_worlds.back()` |

---

## ENetPeer Fields - Complete Reference

### Understanding `event.peer`

**Type:** `ENetPeer*` (from ENet library)

**Important:** `event.peer` is the **ENet connection object**, while `event.peer->data` is your **custom peer class**.

### Two Types of Peer Data

```cpp
// 1. ENet Peer (connection info)
ENetPeer* enet_peer = event.peer;

// 2. Custom Peer (player data)
::peer* custom_peer = static_cast<::peer*>(event.peer->data);
```

---

### ENetPeer Fields (From ENet Library)

**File:** `include/enet/enet.h` (line 264)

These fields are available from `event.peer->`:

#### Connection Information

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `address` | `ENetAddress` | Internet address of the peer | `event.peer->address` |
| `state` | `ENetPeerState` | Current connection state | `event.peer->state` |
| `connectID` | `enet_uint32` | Unique connection ID | `event.peer->connectID` |
| `outgoingPeerID` | `enet_uint16` | Peer's outgoing ID | `event.peer->outgoingPeerID` |
| `incomingPeerID` | `enet_uint16` | Peer's incoming ID | `event.peer->incomingPeerID` |

#### Bandwidth Information

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `incomingBandwidth` | `enet_uint32` | **Downstream** bandwidth (bytes/sec) | `event.peer->incomingBandwidth` |
| `outgoingBandwidth` | `enet_uint32` | **Upstream** bandwidth (bytes/sec) | `event.peer->outgoingBandwidth` |

#### Data Transfer Statistics

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `incomingDataTotal` | `enet_uint32` | Total bytes received | `event.peer->incomingDataTotal` |
| `outgoingDataTotal` | `enet_uint32` | Total bytes sent | `event.peer->outgoingDataTotal` |
| `lastSendTime` | `enet_uint32` | Last send timestamp | `event.peer->lastSendTime` |
| `lastReceiveTime` | `enet_uint32` | Last receive timestamp | `event.peer->lastReceiveTime` |

#### Packet Loss & Quality

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `packetsSent` | `enet_uint32` | Total packets sent | `event.peer->packetsSent` |
| `packetsLost` | `enet_uint32` | Total packets lost | `event.peer->packetsLost` |
| `packetLoss` | `enet_uint32` | Mean packet loss ratio | `event.peer->packetLoss` |
| `packetLossVariance` | `enet_uint32` | Packet loss variance | `event.peer->packetLossVariance` |

#### Round Trip Time (RTT)

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `roundTripTime` | `enet_uint32` | **Mean RTT** in milliseconds | `event.peer->roundTripTime` |
| `lastRoundTripTime` | `enet_uint32` | Last measured RTT | `event.peer->lastRoundTripTime` |
| `lowestRoundTripTime` | `enet_uint32` | Lowest RTT recorded | `event.peer->lowestRoundTripTime` |
| `roundTripTimeVariance` | `enet_uint32` | RTT variance | `event.peer->roundTripTimeVariance` |

#### Network Configuration

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `mtu` | `enet_uint32` | Maximum Transmission Unit | `event.peer->mtu` |
| `windowSize` | `enet_uint32` | Congestion window size | `event.peer->windowSize` |
| `channelCount` | `size_t` | Number of channels | `event.peer->channelCount` |

#### Timeout Configuration

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `timeoutLimit` | `enet_uint32` | Timeout limit | `event.peer->timeoutLimit` |
| `timeoutMinimum` | `enet_uint32` | Minimum timeout | `event.peer->timeoutMinimum` |
| `timeoutMaximum` | `enet_uint32` | Maximum timeout | `event.peer->timeoutMaximum` |
| `nextTimeout` | `enet_uint32` | Next timeout timestamp | `event.peer->nextTimeout` |

#### Application Data

| Field | Type | Description | Example Usage |
|-------|------|-------------|---------------|
| `data` | `void*` | **Custom peer class** (Gurotopia uses this!) | `static_cast<::peer*>(event.peer->data)` |
| `eventData` | `enet_uint32` | Event data (used in disconnect) | `event.peer->eventData` |

---

### Common ENetPeer Usage Patterns

#### Pattern 1: Get Custom Peer Data

**Most Common:**
```cpp
::peer *peer = static_cast<::peer*>(event.peer->data);
```

**Why:** ENet's `data` field is `void*`, must cast to `::peer*`

---

#### Pattern 2: Check Connection State

```cpp
if (event.peer->state == ENET_PEER_STATE_CONNECTED)
{
    // Peer is connected
}
else if (event.peer->state == ENET_PEER_STATE_DISCONNECTED)
{
    // Peer disconnected
}
```

**ENetPeerState Values:**
- `ENET_PEER_STATE_DISCONNECTED` (0)
- `ENET_PEER_STATE_CONNECTING` (1)
- `ENET_PEER_STATE_ACKNOWLEDGING_CONNECT` (2)
- `ENET_PEER_STATE_CONNECTION_PENDING` (3)
- `ENET_PEER_STATE_CONNECTED` (4)
- `ENET_PEER_STATE_DISCONNECT_LATER` (5)

---

#### Pattern 3: Get Peer IP Address

```cpp
ENetAddress addr = event.peer->address;

// Get IP as string
char ip[256];
enet_address_get_host_ip(&addr, ip, sizeof(ip));
std::printf("Peer IP: %s\n", ip);

// Get port
enet_uint16 port = addr.port;
```

---

#### Pattern 4: Check Bandwidth

```cpp
// Incoming (download) bandwidth
enet_uint32 download = event.peer->incomingBandwidth;
std::printf("Download: %u bytes/sec\n", download);

// Outgoing (upload) bandwidth
enet_uint32 upload = event.peer->outgoingBandwidth;
std::printf("Upload: %u bytes/sec\n", upload);
```

---

#### Pattern 5: Check Connection Quality

```cpp
// Ping (RTT)
enet_uint32 ping = event.peer->roundTripTime;
std::printf("Ping: %u ms\n", ping);

// Packet loss
float loss_percent = (float)event.peer->packetLoss / 65535.0f * 100.0f;
std::printf("Packet Loss: %.2f%%\n", loss_percent);

// Total data transferred
enet_uint32 total_in = event.peer->incomingDataTotal;
enet_uint32 total_out = event.peer->outgoingDataTotal;
std::printf("Data: In %u, Out %u bytes\n", total_in, total_out);
```

---

#### Pattern 6: Disconnect with Event Data

```cpp
// Disconnect with custom event data
event.peer->eventData = 12345;  // Custom code
enet_peer_disconnect(event.peer, 0);

// Later, in disconnect handler:
void disconnect(ENetEvent& event)
{
    enet_uint32 code = event.peer->eventData;
    // Use code to determine disconnect reason
}
```

---

### Complete Example: Using Both ENetPeer and Custom Peer

```cpp
void some_function(ENetEvent& event)
{
    // === ENet Peer (Connection Info) ===
    ENetPeer* enet_peer = event.peer;
    
    // Get connection state
    if (enet_peer->state != ENET_PEER_STATE_CONNECTED)
        return;
    
    // Get IP address
    char ip[256];
    enet_address_get_host_ip(&enet_peer->address, ip, sizeof(ip));
    
    // Get ping
    enet_uint32 ping = enet_peer->roundTripTime;
    
    // Get bandwidth
    enet_uint32 upload = enet_peer->outgoingBandwidth;
    
    // === Custom Peer (Player Info) ===
    ::peer* custom_peer = static_cast<::peer*>(enet_peer->data);
    
    // Get player name
    std::string name = custom_peer->ltoken[0];
    
    // Get player level
    int level = custom_peer->level[0];
    
    // Log info
    std::printf("Player: %s, IP: %s, Ping: %u ms, Level: %d\n",
        name.c_str(), ip, ping, level);
}
```

---

### ENetPeer vs Custom Peer - Key Differences

| Aspect | ENetPeer (`event.peer`) | Custom Peer (`event.peer->data`) |
|--------|------------------------|----------------------------------|
| **Type** | `ENetPeer*` | `::peer*` |
| **Purpose** | Network connection | Player game data |
| **Managed by** | ENet library | Gurotopia code |
| **Lifetime** | Created/destroyed by ENet | Created on connect, deleted on disconnect |
| **Contains** | Connection state, IP, bandwidth, ping | Name, level, gems, inventory, position |
| **Access** | Direct: `event.peer->state` | Cast: `static_cast<::peer*>(event.peer->data)` |

---

### Quick Reference: What to Use When

**Use `event.peer->` (ENetPeer) for:**
- ✅ Getting player IP address
- ✅ Checking connection state
- ✅ Measuring ping/latency
- ✅ Checking bandwidth
- ✅ Monitoring packet loss
- ✅ Getting connection statistics
- ✅ Disconnecting with custom data

**Use `event.peer->data` (Custom Peer) for:**
- ✅ Getting player name (GrowID)
- ✅ Accessing inventory
- ✅ Modifying level/XP
- ✅ Changing gems
- ✅ Getting/setting position
- ✅ Accessing world data
- ✅ Modifying clothing/equipment
- ✅ Any game logic

---

### Memory Layout Visualization

```
ENetEvent
├── type (ENetEventType)
├── peer (ENetPeer*)  ←───┐
│   │                      │
│   ├── address            │ ENet connection
│   ├── state              │ (managed by ENet)
│   ├── roundTripTime      │
│   ├── incomingBandwidth  │
│   ├── outgoingBandwidth  │
│   ├── packetsSent        │
│   ├── packetsLost        │
│   └── data (void*) ──────┼───┐
│                           │   │
│                           │   │ Cast to ::peer*
│                           │   │
│                           ▼   ▼
│                       ┌─────────────────┐
│                       │ ::peer          │ ← Custom peer class
│                       │ ├─ ltoken[0]    │   (managed by Gurotopia)
│                       │ ├─ user_id      │
│                       │ ├─ level[0]     │
│                       │ ├─ level[1]     │
│                       │ ├─ gems         │
│                       │ ├─ slots        │
│                       │ ├─ pos          │
│                       │ └─ ...          │
│                       └─────────────────┘
└── packet (ENetPacket*)
```

---

### Important Notes

1. **Always Cast Safely:**
```cpp
if (event.peer != nullptr && event.peer->data != nullptr)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    // Safe to use p
}
```

2. **Don't Mix Them Up:**
```cpp
// WRONG: Can't access custom data from ENetPeer
std::string name = event.peer->ltoken[0];  // ERROR!

// RIGHT: Must cast first
::peer *p = static_cast<::peer*>(event.peer->data);
std::string name = p->ltoken[0];  // OK
```

3. **ENetPeer is Read-Only (Mostly):**
- Most ENetPeer fields are read-only
- Don't modify ENetPeer fields directly
- Use ENet API functions instead

4. **Custom Peer is Yours:**
- You own the `::peer` object
- Can read/write any field
- Responsible for memory management

---

### ENetPeer Field Reference Table

| Field | Read/Write | Common Use |
|-------|-----------|------------|
| `address` | Read | Get IP/port |
| `state` | Read | Check connection |
| `connectID` | Read | Connection tracking |
| `data` | Read/Write | Store custom data |
| `incomingBandwidth` | Read | Bandwidth monitoring |
| `outgoingBandwidth` | Read | Bandwidth monitoring |
| `roundTripTime` | Read | Ping display |
| `packetLoss` | Read | Quality monitoring |
| `packetsSent` | Read | Statistics |
| `packetsLost` | Read | Statistics |
| `incomingDataTotal` | Read | Data usage |
| `outgoingDataTotal` | Read | Data usage |
| `mtu` | Read | Network config |
| `eventData` | Read/Write | Disconnect codes |

---

## Player System - Deep Dive

### Role System

#### Role Enum

**File:** `include/database/peer.hpp`

```cpp
enum role : u_char {
    PLAYER,      // 0 - Default player
    DOU_ZUN,   // 1 - DOU_ZUN with special privileges
    BAN_SHENG    // 2 - BAN_SHENG with highest privileges
};
```

#### Role Prefixes

Each role has a specific prefix displayed before the player's name:

| Role | Prefix | Display Example | Color |
|------|--------|----------------|-------|
| PLAYER | `w` | `wPlayerName`` | White |
| PLAYER (World Owner) | `2` | `2PlayerName`` | Green |
| PLAYER (World Admin) | `c` | `cPlayerName`` | Cyan |
| DOU_ZUN | `#@` | `#@PlayerName`` | Cyan with # prefix |
| BAN_SHENG | `8@` | `8@PlayerName`` | Special BAN_SHENG prefix |

**Code:**
```cpp
// From action/enter_game.cpp
peer->prefix = (peer->role == DOU_ZUN) ? "#@" : 
               (peer->role == BAN_SHENG) ? "8@" : peer->prefix;

// From action/dialog_return/peer_edit.cpp
_p->prefix = (_p->role == DOU_ZUN) ? "#@" : 
             (_p->role == BAN_SHENG) ? "8@" : _p->prefix;
```

#### Role-Based Permissions

**World Ownership:**
- When placing World Lock: `world->owner = peer->user_id`
- Owner gets green prefix (`2`)
- Can edit world lock settings

**World Admin:**
- Admins added via lock edit dialog
- Get cyan prefix (`c`)
- Can build/break in locked worlds

**DOU_ZUN/BAN_SHENG:**
- Bypass world lock restrictions
- Can edit any player via `/edit`
- Special prefixes (`#@` for mod, `8@` for dev)

---

### Level & XP System

#### Level Structure

**File:** `include/database/peer.hpp`

```cpp
std::array<u_short, 2zu> level{ 1, 0 };  // {level, xp}
```

- `level[0]` - Current level (starts at 1)
- `level[1]` - Current XP within level

#### XP Formula

**Credit:** https://www.growtopiagame.com/forums/member/553046-kasete

```cpp
u_short xp_needed = 50 * (level * level + 2);
```

**XP Requirements:**

| Level | XP Needed | Total XP |
|-------|-----------|----------|
| 1 → 2 | 150 | 150 |
| 2 → 3 | 300 | 450 |
| 3 → 4 | 550 | 1,000 |
| 10 → 11 | 5,050 | 28,875 |
| 50 → 51 | 125,050 | ~1,000,000 |
| 124 → 125 | 768,850 | ~30,000,000 |

#### Level Up Process

**File:** `include/database/peer.cpp`

```cpp
void peer::add_xp(ENetEvent &event, u_short value)
{
    u_short &lvl = this->level.front();
    u_short &xp = this->level.back() += value;

    for (; lvl < 125; )
    {
        u_short xp_formula = 50 * (lvl * lvl + 2);
        if (xp < xp_formula) break;

        xp -= xp_formula;  // Carry over excess XP
        lvl++;

        // Level 50 reward
        if (lvl == 50)
        {
            modify_item_inventory(event, ::slot{1400, 1});  // Mini Growtopian
        }
        
        // Level 125 reward
        if (lvl == 125) 
            on::CountryState(event);  // Blue name

        // Send level up effects
        state_visuals(*event.peer, {
            .type = 0x11,  // Particle effect
            .pos = this->pos,
            .speed = {0, 0x2e}
        });
        
        packet::create(*event.peer, false, 0, {
            "OnTalkBubble", this->netid,
            std::format("`{}{}`` is now level {}!", 
                this->prefix, this->ltoken[0], lvl).c_str()
        });
    }
}
```

**Process:**
1. Add XP to current XP
2. Check if XP >= required for next level
3. If yes:
   - Subtract required XP (carry over excess)
   - Increment level
   - Check for milestone rewards (50, 125)
   - Send particle effect
   - Announce level up
4. Repeat until no more level ups

#### Level Milestones

**Level 50:**
- Reward: Mini Growtopian (ID: 1400)
- Note: "based on account age give peer other items" (TODO)

**Level 125 (MAX):**
- Reward: Blue name (via `on::CountryState()`)
- Sends `|maxLevel` flag with country

**CountryState Packet:**
```cpp
void on::CountryState(ENetEvent& event)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    packet::create(*event.peer, true, 0, {
        "OnCountryState",
        std::format("{}{}", peer->country, 
            (peer->level[0] == 125) ? "|maxLevel" : "").c_str()
    });
}
```

---

### Player Database Schema

**File:** `include/database/peer.cpp`

#### Tables

```sql
-- Core player data
CREATE TABLE peers (
    _n TEXT PRIMARY KEY,      -- GrowID (name)
    role INTEGER,             -- 0=Player, 1=Mod, 2=Dev
    gems INTEGER,             -- Gem count
    lvl INTEGER,              -- Level
    xp INTEGER                -- XP within level
);

-- Inventory items
CREATE TABLE slots (
    _n TEXT,                  -- GrowID (foreign key)
    i INTEGER,                -- Item ID
    c INTEGER,                -- Item count
    FOREIGN KEY(_n) REFERENCES peers(_n)
);

-- Equipped clothing
CREATE TABLE equip (
    _n TEXT,                  -- GrowID (foreign key)
    i INTEGER,                -- Clothing item ID
    FOREIGN KEY(_n) REFERENCES peers(_n)
);
```

#### Player Data Fields

**File:** `include/database/peer.hpp`

```cpp
class peer {
    // Identity
    int netid{ 0 };                      // Network ID in world
    int user_id{};                       // Unique user ID
    std::array<std::string, 2zu> ltoken{};  // {growId, password}
    std::string game_version{};          // Client version
    std::string country{};               // Country code
    std::string prefix{ "w" };           // Name prefix/color
    u_char role{};                       // PLAYER/MOD/DEV
    
    // Appearance
    std::array<float, 10zu> clothing{};  // Equipped items
    u_char punch_effect{};               // Active effect
    u_int skin_color{ 2527912447 };      // Skin color
    int state{};                         // Player state flags
    
    // Billboard
    Billboard billboard{};               // Name tag settings
    
    // Position
    ::pos pos{0,0};                      // Current position
    ::pos rest_pos{0,0};                 // Respawn position
    bool facing_left{};                  // Facing direction
    
    // Inventory
    short slot_size{16};                 // Backpack slots
    std::vector<slot> slots{};           // Inventory items
    std::vector<short> fav{};            // Favorite items
    
    // Currency & Progress
    signed gems{0};                      // Gem count
    std::array<u_short, 2zu> level{ 1, 0 };  // {level, xp}
    
    // Worlds
    std::array<std::string, 6zu> recent_worlds{};   // Recent 6 worlds
    std::array<std::string, 200zu> my_worlds{};     // Owned worlds (200)
    
    // Chat
    std::deque<steady_clock::time_point> messages;  // Anti-spam
    
    // Social
    std::array<Friend, 25> friends;      // Friend list
    
    // Stats
    u_short fires_removed{};             // Fires extinguished
    u_short gbc_pity{};                  // GBC pity counter
};
```

---

## Commands System - Complete Reference

### Command Pool Structure

**File:** `include/commands/__command.hpp`, `__command.cpp`

```cpp
std::unordered_map<std::string_view, 
    std::function<void(ENetEvent&, const std::string_view)>> cmd_pool;
```

Commands are routed via string-keyed function map, triggered by `/command` in chat.

### Command Registration

```cpp
std::unordered_map<std::string_view, std::function<...>> cmd_pool
{
    {"help", help_return },
    {"?", help_return },
    {"find", &find},
    {"warp", &warp},
    {"edit", &edit},
    {"punch", &punch},
    {"skin", &skin},
    {"sb", &sb},
    {"who", &who},
    {"me", &me},
    {"weather", &weather},
    {"ghost", &ghost},
    {"ageworld", &ageworld},
    
    // Emotes (all handled by on::Action)
    {"wave", &on::Action}, {"dance", &on::Action}, 
    {"love", &on::Action}, {"sleep", &on::Action}, 
    {"facepalm", &on::Action}, {"fp", &on::Action},
    {"smh", &on::Action}, {"yes", &on::Action}, 
    {"no", &on::Action}, {"omg", &on::Action}, 
    {"idk", &on::Action}, {"shrug", &on::Action},
    {"furious", &on::Action}, {"rolleyes", &on::Action}, 
    {"foldarms", &on::Action}, {"fa", &on::Action}, 
    {"stubborn", &on::Action}, {"fold", &on::Action},
    {"dab", &on::Action}, {"sassy", &on::Action}, 
    {"dance2", &on::Action}, {"march", &on::Action}, 
    {"grumpy", &on::Action}, {"shy", &on::Action}
};
```

---

### Admin Commands

#### `/edit` - Player Editor

**File:** `include/commands/edit.cpp`

**Signature:**
```cpp
void edit(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/edit {player name}
```

**Purpose:**
Edit player data (role, level, gems) - Admin tool.

**Process:**
1. Validates player name provided
2. Extracts name (after "/edit ")
3. Searches for online player
4. If found online:
   - Shows edit dialog with current values
   - Embeds `status=1` (online)
5. If offline:
   - Loads from database via `peer().read(name)`
   - Shows edit dialog with stored values
   - Embeds `status=0` (offline)

**Dialog Fields:**
- `role` - Role (0=Player, 1=DOU_ZUN, 2=BAN_SHENG)
- `level` - Level (1-125)
- `gems` - Gem count

**Dialog Format:**
```
set_default_color|`o
add_label|big|Username: {name} ({status})|left
embed_data|name|{name}
add_spacer|small|
add_text_input|role|Role: |{role}|3|
add_text_input|level|Level: |{level}|3|
add_text_input|gems|Gems: |{gems}|10|
add_spacer|small|
embed_data|status|{0|1}
end_dialog|peer_edit|Close|`2Edit|
```

**Handler:** `action::dialog_return\peer_edit()`

```cpp
void peer_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const std::string name = pipes[5zu];
    const bool status = atoi(pipes[8zu].c_str());  // 1=online, 0=offline
    const u_char role = atoi(pipes[11zu].c_str());
    const short level = atoi(pipes[13zu].c_str());
    const signed gems = atoi(pipes[15zu].c_str());

    if (status)  // Online player
    {
        peers("", PEER_ALL, [&event, name, role, level, gems](ENetPeer& p)
        {
            ::peer *_p = static_cast<::peer*>(p.data);
            if (_p->ltoken[0] == name)
            {
                _p->level[0] = level;  // Set level
                on::CountryState(event);  // Update blue name if 125

                _p->role = role;  // Set role
                _p->prefix = (_p->role == DOU_ZUN) ? "#@" : 
                             (_p->role == BAN_SHENG) ? "8@" : _p->prefix;
                on::NameChanged(event);  // Update name

                _p->gems = gems;  // Set gems
                on::SetBux(event);  // Update gem display
                return;
            }
        });
    }
    else  // Offline player
    {
        ::peer &offline = ::peer().read(name);
        offline.role = role;
        offline.level[0] = level;
        offline.gems = gems;
        // Saved to DB when peer object is destroyed
    }
}
```

**Notes:**
- `@todo use _p->add_xp()` - Currently sets level directly, doesn't use XP system
- Changes saved to database immediately for offline players
- For online players, changes applied in real-time

---

#### `/punch` - Set Punch Effect

**File:** `include/commands/punch.cpp`

**Signature:**
```cpp
void punch(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/punch {effect ID}
```

**Purpose:**
Set player's punch effect (clothing-based visual effect).

**Process:**
1. Validates ID provided
2. Extracts ID (after "/punch ")
3. Converts to integer
4. Sets `peer->punch_effect`
5. Sends `OnSetClothing` packet

**Error Handling:**
- Shows "Invalid input. id must be a number." on failure

**Punch Effect ID Mapping:**

**File:** `include/commands/punch.cpp`

```cpp
u_char get_punch_id(u_int item_id)
{
    switch (item_id)
    {
        case 138: case 2976: return 1;   // Eye Beam
        case 366: return 2;
        case 472: return 3;
        case 594: return 4;
        case 768: return 5;
        case 900: return 6;
        case 930: return 8;   // or 12
        case 1204: return 10;
        case 1738: return 11;
        case 1484: return 12;
        case 3066: case 5206: case 7504: case 10288: return 17;  // Fire Hose
        case 2636: case 2908: case 3070: case 3108: case 3466: return 29;  // Slasher
        default: return 0;
    }
}
```

**Reference:**
- https://growtopia.fandom.com/wiki/Mods/Eye_Beam
- https://growtopia.fandom.com/wiki/Mods/Fire_Hose
- https://growtopia.fandom.com/wiki/Mods/Slasher

---

#### `/skin` - Set Skin Color

**File:** `include/commands/skin.cpp`

**Signature:**
```cpp
void skin(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/skin {color ID}
```

**Purpose:**
Change player's skin color.

**Process:**
1. Validates ID provided
2. Extracts ID (after "/skin ")
3. Converts to long integer
4. Sets `peer->skin_color`
5. Sends `OnSetClothing` packet

**Error Handling:**
- `invalid_argument` - Shows "id must be a number"
- `out_of_range` - Shows "id is out of range"

**Related:**
- `action::setSkin()` - Same functionality via action packet

---

#### `/sb` - Server Broadcast

**File:** `include/commands/sb.cpp`

**Signature:**
```cpp
void sb(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/sb {message}
```

**Purpose:**
Send server-wide broadcast message (visible to all players).

**Process:**
1. Validates message provided
2. Extracts message (after "/sb ")
3. Gets current world
4. Checks for Signal Jammer (ID: 226) in world
   - If toggled on: display = "`4JAMMED``"
5. Sends to **ALL** players on server:
   - Console message with format

**Format:**
```
CP:0_PL:0_OID:_CT:[SB]_ `5** from (`{prefix}{name}`````5) in [````{world}```5] ** : ````{message}``
```

**Signal Jammer Check:**
```cpp
std::string display = peer->recent_worlds.back();
for (::block &block : world->blocks)
    if (block.fg == 226 && block.state3 & S_TOGGLE)
    {
        display = "`4JAMMED``";
        break;
    }
```

**Notes:**
- Bypasses world boundaries
- Shows current world (or "JAMMED" if signal jammed)
- Format includes color codes for visibility

---

#### `/ghost` - Toggle Ghost Mode

**File:** `include/commands/ghost.cpp`

**Signature:**
```cpp
void ghost(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/ghost
```

**Purpose:**
Toggle ghost mode (invisibility effect).

**Process:**
1. Toggles `S_GHOST` flag in `peer->state`
   ```cpp
   peer->state ^= S_GHOST;  // XOR toggles bit
   ```
2. Sends `OnSetClothing` packet (updates appearance)

**Ghost Effect:**
- In `on::SetClothing()`:
  ```cpp
  std::vector<float>{_p->clothing[ances], 0.0f, 0.0f},
  (_p->state & S_GHOST) ? -140 : _p->skin_color,  // -140 = ghost skin
  ```

**State Flags:**
```cpp
enum pstate : int
{
    S_GHOST       = 0x00000001,  // Ghost mode
    S_DOUBLE_JUMP = 0x00000002,  // Double jump
    S_DUCT_TAPE   = 0x00002000   // Duct tape effect
};
```

---

#### `/ageworld` - Age World

**File:** `include/commands/ageworld.cpp`

**Signature:**
```cpp
void ageworld(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/ageworld
```

**Purpose:**
Age all providers and trees in current world by 1 day (24 hours).

**Process:**
1. Gets current world
2. Iterates all 6000 blocks
3. For PROVIDER and SEED types:
   - Subtracts 86400 seconds (1 day) from `block.tick`
   - Sends tile update to show progress
4. Shows confirmation message

**Code:**
```cpp
void ageworld(ENetEvent& event, const std::string_view text)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    auto world = std::ranges::find(worlds, 
        peer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    for (std::size_t i = 0zu; i < world->blocks.size(); ++i)
    {
        block &block = world->blocks[i];
        auto item = std::ranges::find(items, block.fg, &::item::id);

        if (item->type == type::PROVIDER || item->type == type::SEED)
        {
            block.tick -= 86400s;  // Subtract 1 day
            send_tile_update(event,
            {
                .id = block.fg,
                .punch = ::pos{ (short)i % 100, (short)i / 100 }
            }, block, *world);
        }
    }
    packet::create(*event.peer, false, 0, { 
        "OnConsoleMessage", "aged world by `w1 day``." 
    });
}
```

**Use Cases:**
- Speed up tree growth
- Speed up provider production
- Testing tool for world BAN_SHENGs

---

### Utility Commands

#### `/find` - Item Search

**File:** `include/commands/find.cpp`

**Signature:**
```cpp
void find(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/find
```

**Purpose:**
Open item search dialog (debug/admin tool for spawning items).

**Dialog Format:**
```
set_default_color|`o
add_text_input|n|Search: ||26|
add_searchable_item_list||sourceType:allItems;listType:iconWithCustomLabel;resultLimit:30|n|
add_quick_exit|
end_dialog|find_item|||
```

**Handler:** `action::dialog_return\find_item()`

```cpp
void find_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    std::string id = readch(pipes[5zu], '_')[1];  // After searchableItemListButton
    modify_item_inventory(event, ::slot(atoi(id.c_str()), 200));
}
```

**Notes:**
- Gives 200 of selected item
- Debug/admin tool
- No permission checks (potential exploit)

---

#### `/warp` - Teleport

**File:** `include/commands/warp.cpp`

**Signature:**
```cpp
void warp(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/warp {world name}
```

**Purpose:**
Teleport to specified world instantly.

**Process:**
1. Validates world name provided
2. Extracts world name (after "/warp ")
3. Converts to uppercase
4. Logs command to chat
5. Sends freeze state (visual effect)
6. Sends warp message
7. Calls `quit_to_exit()` to leave current world
8. Calls `join_request()` to enter target world

**Code:**
```cpp
void warp(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("warp ") - 1)
    {
        packet::create(*event.peer, false, 0, { 
            "OnConsoleMessage", "Usage: /warp `w{world name}``" 
        });
        return;
    }
    std::string world_name{ text.substr(sizeof("warp ") - 1) };
    std::for_each(world_name.begin(), world_name.end(), 
        [](char& c) { c = std::toupper(c); });

    packet::action(*event.peer, "log", 
        std::format("msg| `6/warp {}``", world_name));
    packet::create(*event.peer, false, 0, { "OnSetFreezeState", 1 });
    packet::action(*event.peer, "log", 
        std::format("msg|Magically warping to world `5{}``...", world_name));

    action::quit_to_exit(event, "", true);  // Leave current
    action::join_request(event, "", world_name);  // Enter target
}
```

**Notes:**
- Bypasses world select menu
- No permission checks
- Works for any world (including private worlds)

---

#### `/who` - List Players

**File:** `include/commands/who.cpp`

**Signature:**
```cpp
void who(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/who
```

**Purpose:**
List all players in current world.

**Process:**
1. Gets current world name
2. Iterates all players in world
3. For each player (except self):
   - Sends talk bubble with name
   - Collects name to list
4. Logs formatted list to command user

**Output Format:**
```
msg|`wWho's in `{world_name}``:`` {player_list}``
```

**Example:**
```
Who's in WORLDNAME: Player1, Player2, Player3
```

**Code:**
```cpp
void who(ENetEvent& event, const std::string_view text)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    std::vector<std::string> names;
    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, 
        [&peer, event, &names](ENetPeer& p)
    {
        ::peer *_p = static_cast<::peer*>(p.data);
        std::string full_name = std::format("`{}{}", 
            _p->prefix, _p->ltoken[0]);
        
        if (_p->user_id != peer->user_id)
        {
            packet::create(*event.peer, false, 0, { 
                "OnTalkBubble", _p->netid, full_name.c_str(), 1u 
            });
        }
        names.emplace_back(std::move(full_name));
    });
    
    packet::action(*event.peer, "log", std::format(
        "msg|`wWho's in `${}``:`` {}``",
        peer->recent_worlds.back(), join(names, ", ")));
}
```

---

#### `/me` - Emote Message

**File:** `include/commands/me.cpp`

**Signature:**
```cpp
void me(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/me {message}
```

**Purpose:**
Send emote/action message to world (third-person narration).

**Process:**
1. Validates message provided
2. Extracts message (after "/me ")
3. Sends to all players in world:
   - Talk bubble with format
   - Console message with same format

**Format:**
```
`6<```{prefix}{name}`` `#{message}```6>``
```

**Example:**
```
<`wPlayerName`` `#is dancing!``>
```

**Code:**
```cpp
void me(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("me ") - 1)
    {
        packet::create(*event.peer, false, 0, { 
            "OnConsoleMessage", "Usage: /me `w{message}``" 
        });
        return;
    }
    std::string message{ text.substr(sizeof("me ")-1) };
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, 
        [&peer, message](ENetPeer& p)
    {
        packet::create(p, false, 0, {
            "OnTalkBubble",
            peer->netid,
            std::format("player_chat= `6<```{}{}`` `#{}```6>``",
                peer->prefix, peer->ltoken[0], message).c_str(),
            0u
        });
        packet::create(p, false, 0, {
            "OnConsoleMessage",
            std::format("CP:0_PL:0_OID:__CT:[W]_ `6<```{}{}`` `#{}```6>``",
                peer->prefix, peer->ltoken[0], message).c_str()
        });
    });
}
```

---

#### `/weather` - Change Weather

**File:** `include/commands/weather.cpp`

**Signature:**
```cpp
void weather(ENetEvent& event, const std::string_view text)
```

**Usage:**
```
/weather {weather ID}
```

**Purpose:**
Change current weather effect in world.

**Process:**
1. Validates ID provided
2. Extracts ID (after "/weather ")
3. Sends `OnSetCurrentWeather` packet with ID

**Error Handling:**
- Shows "Invalid input. id must be a number." on failure

**Weather ID Mapping:**

**File:** `include/commands/weather.cpp`

| ID | Weather | Item ID (Weather Machine) |
|----|---------|---------------------------|
| 0 | Sunny | - |
| 1 | Beach | 830 (Blast) |
| 2 | Night | 934 |
| 3 | Arid | 946 |
| 5 | Rainy City | 984 |
| 6 | Harvest Moon | 1060 (Blast) |
| 7 | Mars | 1136 (Blast) |
| 8 | Spooky | 1210 |
| 10 | Nothingness | 1490 |
| 11 | Snowy | 1364 |
| 14 | Undersea | 1532 (Blast) |
| 15 | Warp Speed | 1750 |
| 17 | Comet | 2046 |
| 18 | Party | 2284 |
| 19 | Pineapples | 2744 |
| 20 | Snowy Night | 3252 |
| 21 | Spring | 3446 |
| 22 | Howling Sky | 3534 |
| 23 | Heatwave | 3694 |
| 29 | Stuff | 3832 |
| 30 | Pagoda | 4242 |
| 31 | Apocalypse | 4486 |
| 32 | Jungle | 4774 (Blast) |
| 33 | Balloon Warz | 4892 |
| 34 | Background | 5000 |
| 35 | Autumn | 5112 |
| 36 | Valentine's | 5654 |
| 37 | St. Paddy's | 5716 |
| 42 | Digital Rain | 6854 |
| 43 | Monochrome | 7380 (Blast) |
| 44 | Frozen Cliffs | 7644 |
| 45 | Surg World | 8556 (Blast) |
| 46 | Bountiful | 8738 (Blast) |
| 47 | Stargazing | 8836 |
| 48 | Meteor Shower | 8896 |
| 51 | Celebrity Hills | 10286 |
| 59 | Plaza | 11880 |
| 60 | Nebula | 12054 |
| 61 | Protostar Landing | 12056 |
| 62 | Dark Mountains | 12408 |
| 64 | Mt. Growmore | 12844 |
| 65 | Crack in Reality | 13004 |
| 66 | Nian Mountains | 13070 |
| 69 | Realm of Spirits | 13640 |
| 70 | Black Hole | 13690 |
| 71 | Raining Gems | 14032 |
| 72 | Holiday Heaven | 14094 |
| 76 | Atlantis | 14598 |
| 77 | Petal Purrfect Haven | 14802 |
| 78 | Candyland Blast | 14896 (Blast) |
| 79 | Dragon's Keep | 15150 |
| 80 | Emerald City | 15240 |

**Related:**
- `get_weather_id()` - Maps weather machine item IDs to weather IDs
- Weather machines toggle weather when punched twice

---

### Emote Commands

All emote commands are handled by `on::Action()`.

**File:** `include/on/Action.hpp`, `Action.cpp`

**Supported Emotes:**

| Command | Description |
|---------|-------------|
| `/wave` | Wave emote |
| `/dance` | Dance emote |
| `/love` | Love emote |
| `/sleep` | Sleep emote |
| `/facepalm`, `/fp` | Facepalm emote |
| `/smh` | Shaking my head |
| `/yes` | Yes nod |
| `/no` | No shake |
| `/omg` | OMG emote |
| `/idk` | I don't know shrug |
| `/shrug` | Shrug emote |
| `/furious` | Furious emote |
| `/rolleyes` | Roll eyes |
| `/foldarms`, `/fa` | Fold arms |
| `/stubborn` | Stubborn emote |
| `/fold` | Fold emote |
| `/dab` | Dab emote |
| `/sassy` | Sassy emote |
| `/dance2` | Dance 2 emote |
| `/march` | March emote |
| `/grumpy` | Grumpy emote |
| `/shy` | Shy emote |

**Handler:**
```cpp
void on::Action(ENetEvent& event, const std::string_view text)
{
    // Sends action packet with emote name
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format("* {} {}", 
            peer->ltoken[0].c_str(), 
            text.data()).c_str()
    });
}
```

---

### Help Command

**File:** `include/commands/__command.cpp`

**Usage:**
```
/help
/?
```

**Purpose:**
Show list of available commands.

**Output:**
```
>> Commands: /find /warp {world} /edit {player} /punch {id} /skin {id} /sb {msg} /who /me {msg} /weather {id} /ghost /ageworld /wave /dance /love /sleep /facepalm /fp /smh /yes /no /omg /idk /shrug /furious /rolleyes /foldarms /stubborn /fold /dab /sassy /dance2 /march /grumpy /shy
```

**Code:**
```cpp
auto help_return = [](ENetEvent& event, const std::string_view text)
{
    packet::action(*event.peer, "log", 
        "msg|>> Commands: /find /warp {world} /edit {player} /punch {id} /skin {id} /sb {msg} /who /me {msg} /weather {id} /ghost /ageworld /wave /dance /love /sleep /facepalm /fp /smh /yes /no /omg /idk /shrug /furious /rolleyes /foldarms /stubborn /fold /dab /sassy /dance2 /march /grumpy /shy");
};
```

---

### Command System Architecture

#### Command Execution Flow

**File:** `include/action/input.cpp`

```cpp
void action::input(ENetEvent& event, const std::string& header)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes.size() < 5) return;
    if (pipes[3] != "text") return;

    std::string text = pipes[4];
    // ... validation ...

    if (text.starts_with('/'))
    {
        packet::action(*event.peer, "log", 
            std::format("msg| `6{}``", text));
        
        std::string command = text.substr(1, text.find(' ') - 1);

        if (auto it = cmd_pool.find(command); it != cmd_pool.end())
            it->second(std::ref(event), std::move(text.substr(1)));
        else
            packet::action(*event.peer, "log", 
                "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
    }
    // ... normal chat ...
}
```

**Flow:**
1. Player types message starting with `/`
2. Extract command name (up to first space)
3. Look up in `cmd_pool` map
4. If found: call handler with full text
5. If not found: show unknown command message

#### Anti-Spam System

Commands are subject to anti-spam checks:

```cpp
steady_clock::time_point now = steady_clock::now();
peer->messages.push_back(now);
if (peer->messages.size() > 5) peer->messages.pop_front();

if (peer->messages.size() == 5 && 
    duration_cast<seconds>(now - peer->messages.front()).count() < 6)
{
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        "`6>>`4Spam detected! ``Please wait a bit..."
    });
}
```

**Limit:** 5 messages per 6 seconds

---

## Summary Tables

### Role Summary

| Role | Value | Prefix | Permissions |
|------|-------|--------|-------------|
| Player | 0 | `w` (white), `2` (green/owner), `c` (cyan/admin) | Basic |
| DOU_ZUN | 1 | `#@` | Edit players, bypass locks |
| BAN_SHENG | 2 | `8@` | Full access |

### Level Milestones

| Level | Reward | Notes |
|-------|--------|-------|
| 1 | Start | Default |
| 50 | Mini Growtopian | Account age items (TODO) |
| 125 | Blue Name | Max level, `|maxLevel` flag |

### Commands Summary

| Command | Category | Permission | Description |
|---------|----------|------------|-------------|
| `/help`, `/?` | Utility | All | Show command list |
| `/find` | Utility | All | Item search (gives 200) |
| `/warp` | Utility | All | Teleport to world |
| `/edit` | Admin | **Any** | Edit player data |
| `/punch` | Utility | All | Set punch effect |
| `/skin` | Utility | All | Set skin color |
| `/sb` | Utility | All | Server broadcast |
| `/who` | Utility | All | List players in world |
| `/me` | Utility | All | Emote message |
| `/weather` | Utility | All | Change weather |
| `/ghost` | Utility | All | Toggle ghost mode |
| `/ageworld` | Utility | All | Age world by 1 day |
| `/wave` to `/shy` | Emote | All | 22 emote commands |

**Security Note:**
- Currently **NO permission checks** on any command
- `/edit` can modify any player (online/offline)
- `/find` gives items without restrictions
- `/warp` can enter any world
- `/sb` broadcasts to all players
- Consider adding permission checks for production use

### Peer Database (`db/peers.db`)

**Tables:**

```sql
-- Player core data
CREATE TABLE peers (
    _n TEXT PRIMARY KEY,      -- GrowID
    role INTEGER,             -- 0: Player, 1: DOU_ZUN, 2: BAN_SHENG
    gems INTEGER,
    lvl INTEGER,
    xp INTEGER
);

-- Player inventory
CREATE TABLE slots (
    _n TEXT,
    i INTEGER,                -- Item ID
    c INTEGER,                -- Count
    FOREIGN KEY(_n) REFERENCES peers(_n)
);

-- Equipped clothing
CREATE TABLE equip (
    _n TEXT,
    i INTEGER,                -- Clothing item ID
    FOREIGN KEY(_n) REFERENCES peers(_n)
);
```

### Peer Class Structure

```cpp
class peer {
    int netid;                      // Network ID in current world
    int user_id;                    // Unique user identifier
    std::array<std::string, 2> ltoken;  // {growId, password}
    std::string game_version;
    std::string country;
    std::string prefix;             // Name color prefix
    u_char role;                    // PLAYER, DOU_ZUN, BAN_SHENG
    std::array<float, 10> clothing; // Equipped items
    u_char punch_effect;            // Active effect
    u_int skin_color;
    int state;                      // Player state flags
    Billboard billboard;            // Name tag settings
    pos pos, rest_pos;              // Current & spawn position
    bool facing_left;
    short slot_size;                // Backpack slots
    std::vector<slot> slots;        // Inventory
    std::vector<short> fav;         // Favorite items
    signed gems;
    std::array<u_short, 2> level;   // {level, xp}
    std::array<std::string, 6> recent_worlds;
    std::array<std::string, 200> my_worlds;
    std::deque<steady_clock::time_point> messages;  // Anti-spam
    std::array<Friend, 25> friends;
    u_short fires_removed;
    u_short gbc_pity;               // GBC pity counter
};
```

### World Database (`db/worlds.db`)

**Tables:**

```sql
-- World metadata
CREATE TABLE worlds (
    _n TEXT PRIMARY KEY,    -- World name
    owner INTEGER,          -- Owner user_id
    min_level INTEGER       -- Minimum entry level
);

-- Block data (6000 blocks per world)
CREATE TABLE blocks (
    _n TEXT,
    _p INTEGER,             -- Position (y * 100 + x)
    fg INTEGER,             -- Foreground item
    bg INTEGER,             -- Background item
    tick INTEGER,           -- Growth/production timer
    l TEXT,                 -- Label (signs/doors)
    s3 INTEGER,             -- State3 (direction, toggle)
    s4 INTEGER,             -- State4 (water, fire, paint)
    PRIMARY KEY (_n, _p)
);

-- Display blocks
CREATE TABLE displays (
    _n TEXT,
    _p INTEGER,
    i INTEGER,              -- Displayed item ID
    PRIMARY KEY (_n, _p)
);

-- Random blocks (dice, roshambo)
CREATE TABLE random_blocks (
    _n TEXT,
    _p INTEGER,
    val INTEGER,            -- Current value
    PRIMARY KEY (_n, _p)
);

-- Dropped items
CREATE TABLE objects (
    _n TEXT,
    uid INTEGER,            -- Unique object ID
    i INTEGER,              -- Item ID
    c INTEGER,              -- Count
    x REAL, y REAL,         -- Position
    PRIMARY KEY (_n, uid)
);
```

### World Class Structure

```cpp
class world {
    std::string name;
    int owner;                      // Owner user_id
    std::array<int, 6> admin;       // Admin user_ids
    bool is_public;                 // Public build permission
    u_char lock_state;              // Music, render flags
    u_char minimum_entry_level;
    u_char visitors;                // Current player count
    u_char netid_counter;           // NetID generator
    std::vector<block> blocks;      // 6000 blocks
    u_int last_object_uid;          // Object ID generator
    std::vector<object> objects;    // Dropped items
    std::vector<door> doors;        // Door teleport data
    std::vector<display> displays;  // Display block data
    std::vector<random_block> random_blocks;
    pos 现weather;                   // Weather machine position
};
```

---

## Item System

### Items.dat Parser

The server parses `items.dat` binary file containing all item definitions:

```cpp
class item {
    u_short id;                     // Item ID
    u_char property;                // Item properties
    u_char cat;                     // Category flags
    u_char type;                    // Block type
    std::string raw_name;           // Item name
    int ingredient;                 // Splicing ingredient
    u_char collision;               // Collision type
    u_char hits;                    // Hits to break
    int hit_reset;                  // Hit reset time (seconds)
    u_char cloth_type;              // Clothing slot
    short rarity;                   // Rarity value
    int tick;                       // Growth/production time
    std::string info;               // Item description
    std::array<u_short, 2> splice;  // Splice seeds
};
```

### Item Types

```cpp
enum type : u_char {
    FIST = 0x00, WRENCH = 0x01, DOOR = 0x02, LOCK = 0x03,
    GEM = 0x04, TREASURE = 0x05, DEADLY = 0x06,
    TRAMPOLINE = 0x07, CONSUMEABLE = 0x08, ENTRANCE = 0x09,
    SIGN = 0x0A, SFX_BLOCK = 0x0B, TOGGLEABLE_ANIMATED_BLOCK = 0x0C,
    MAIN_DOOR = 0x0D, PLATFORM = 0x0E, STRONG = 0x0F,
    FIRE_PAIN = 0x10, FOREGROUND = 0x11, BACKGROUND = 0x12,
    SEED = 0x13, CLOTHING = 0x14, ANIMATED = 0x15,
    SFX_BACKGROUND = 0x16, ART_WALL = 0x17, BOUNCY = 0x18,
    STING_PAIN = 0x19, PORTAL = 0x1A, CHECKPOINT = 0x1B,
    MUSIC_SHEET = 0x1C, SLIPPERY = 0x1D, TOGGLEABLE_BLOCK = 0x1F,
    CHEST = 0x20, MAILBOX = 0x21, BULLETIN = 0x22,
    PINATA = 0x23, RANDOM = 0x24, COMPONENT = 0x25,
    PROVIDER = 0x26, CHEMICAL_COMBINER = 0x27, ACHIEVEMENT = 0x28,
    WEATHER_MACHINE = 0x29, SCOREBOARD = 0x2A, SUNGATE = 0x2B,
    TOGGLEABLE_DEADLY = 0x2D, HEART_MONITOR = 0x2E,
    DONATION_BOX = 0x2F, MANNEQUIN = 0x31, CCTV = 0x32,
    MAGIC_EGG = 0x33, GAME_BLOCK = 0x34, GAME_GENERATOR = 0x35,
    XENONITE = 0x36, BOOTH = 0x37, CRYSTAL = 0x38,
    CRIME_IN_PROGRESS = 0x39, GRINDER = 0x3A, SPOTLIGHT = 0x3B,
    PUSHING_BLOCK = 0x3C, DISPLAY_BLOCK = 0x3D,
    VENDING_MACHINE = 0x3E, FISH_TANK_PORT = 0x3F, FISH = 0x40,
    SOLAR_COLLECTOR = 0x41, FORGE = 0x42, GIVING_TREE = 0x43,
    GIVING_TREE_STUMP = 0x44, STEAM_BLOCK = 0x45, STEAM_VENT = 0x46,
    STEAM_ORGAN = 0x47, SILKWORM = 0x48, SEWING_MACHINE = 0x49,
    COUNTRY_FLAG = 0x4A, LOBSTER_TRAP = 0x4B, PAINTING_EASEL = 0x4C,
    BATTLE_PET_CAGE = 0x4D, PET_TRAINER = 0x4E, STEAM_ENGINE = 0x4F,
    LOCK_BOT = 0x50, SFX_WEATHER_MACHINE = 0x51,
    SPIRIT_STORAGE = 0x52, DISPLAY_SHELF = 0x53,
    VIP_ENTRANCE = 0x54, CHALLENGE_TIMER = 0x55,
    CHALLENGE_FLAG = 0x56, FISH_MOUNT = 0x57, PORTRAIT = 0x58,
    SPRITE_WEATHER_MACHINE = 0x59, FOSSIL = 0x5A,
    FOSSIL_PREP_STATION = 0x5B, DNA_PROCESSOR = 0x5C,
    TRICKSTER = 0x5D, VALHOWLA_TREASURE = 0x5E,
    CHEMSYNTH_PROCESSOR = 0x5F, CHEMSYNTH_TANK = 0x60,
    STORAGE_BOX = 0x61, COOKING_OVEN = 0x62, AUDIO_BLOCK = 0x63,
    GEIGER_CHARGER = 0x64, THE_ADVENTURE_BEGINS = 0x65,
    TOMB_ROBBER = 0x66, BALLOON_O_MATIC = 0x67,
    TEAM_ENTRANCE_PUNCH = 0x68, TEAM_ENTRANCE_GROW = 0x69,
    TEAM_ENTRANCE_BUILD = 0x6A, AURA = 0x6B,
    LEMON_JELLY_BLOCK = 0x6C, TRAINING_PORT = 0x6D,
    FISHING_BLOCK = 0x6E, MAGPLANT = 0x6F, MAGPLANT_REMOTE = 0x70,
    CYBLOCK_BOT = 0x71, CYBLOCK_COMMAND = 0x72, LUCKY_TOKEN = 0x73,
    GROWSCAN = 0x74, CONTAINMENT_FIELD_POWER_NODE = 0x75,
    SPIRIT_BOARD = 0x76, WORLD_ARCHITECT = 0x77,
    STARTOPIA_BLOCK = 0x78, TOGGLEABLE_MULTI_FRAME_BLOCK = 0x7A,
    AUTO_BREAK_BLOCK = 0x7B, AUTO_BREAK_TREE = 0x7C,
    AUTO_BREAK = 0x7D, STORM_CLOUD = 0x7E,
    DISAPPEAR_WHEN_STEPPED_ON = 0x7F, PUDDLE_BLOCK = 0x80,
    ROOT_CUTTING = 0x81, SAFE_VAULT = 0x82,
    ANGELIC_COUNTING_CLOUD = 0x83, MINING_EXPLOSIVE = 0x84,
    INFINITY_WEATHER_MACHINE = 0x86, GHOST_BLOCK = 0x87,
    ACID = 0x88, WAVING_INFLATABLE_ARM_GUY = 0x8A,
    PINEAPPLE_GUZZLER = 0x8C, KRANKEN_GALACTIC = 0x8D,
    FRIEND_ENTRANCE = 0x8E
};
```

### Item Categories

```cpp
enum cat : u_char {
    CAT_RETURN = 0x02,              // Returns to inventory
    CAT_SUPRISING_FRUIT = 0x08,     // Can bear fruit
    CAT_PUBLIC = 0x10,              // Public access
    CAT_HOLIDAY = 0x40,             // Holiday-only
    CAT_UNTRADEABLE = 0x80          // Cannot trade
};
```

### Clothing System

```cpp
enum clothing : u_char {
    hair, shirt, legs, feet, face, hand,
    back, head, charm, ances, none
};
```

### Store System (`resources/store.txt`)

Store items are defined in `shouhin_tachi`:

```cpp
class shouhin {
    std::string btn;        // Button ID
    std::string name;       // Display name
    std::string rttx;       // Texture path
    std::string description;
    char tex1, tex2;        // Texture variants
    int cost;               // Price
    std::vector<std::pair<short, short>> im;  // {item_id, count}
};
```

**Store Tabs:**
- Tab 0: Main (Gem packs)
- Tab 1: Locks
- Tab 2: Item Packs
- Tab 3: Big Items
- Tab 4: Weather Machines
- Tab 5: Growtoken Items

---

## Dialog System

### Dialog Creation (`include/tools/create_dialog.hpp`)

Fluent builder pattern for creating game dialogs:

```cpp
create_dialog()
    .set_default_color("`o")
    .add_label_with_icon("big", "Title", icon_id)
    .add_textbox("Description text")
    .add_spacer("small")
    .add_text_input("id", "Label", "default", 50)
    .embed_data("key", "value")
    .add_button("btn_id", "Button Text")
    .add_image_button("btn_id", "texture.rttex", "layout", "url")
    .add_quick_exit()
    .end_dialog("dialog_id", "Close", "OK");
```

### Dialog Return Handlers

| Dialog | Handler | Description |
|--------|---------|-------------|
| `lock_edit` | `lock_edit` | Lock configuration |
| `drop_item` | `drop_item` | Item dropping |
| `trash_item` | `trash_item` | Item recycling |
| `find_item` | `find_item` | Item search (debug) |
| `create_blast` | `create_blast` | World creation |
| `peer_edit` | `peer_edit` | Player editing |
| `billboard_edit` | `billboard_edit` | Name tag editing |
| `megaphone` | `megaphone` | Global broadcast |
| `socialportal` | `socialportal` | Friends list |

---

## HTTPS Server

### Login Server (`include/https/https.cpp`)

Listens on port 443 for Growtopia login requests:

```cpp
void https::listener()
{
    // SSL/TLS setup with OpenSSL
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_use_certificate_file(ctx, "resources/ctx/server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "resources/ctx/server.key", SSL_FILETYPE_PEM);
    
    // Listen on port 443
    // Respond to POST /growtopia/server_data.php
    // Returns server configuration
}
```

### Server Data Response

```
server|127.0.0.1
port|17091
type|1
type2|1
loginurl|login-gurotopia.vercel.app
meta|gurotopia
RTENDMARKERBS1001
```

---

## Event System

### Event Pool (`include/event_type/__event_type.cpp`)

```cpp
std::unordered_map<ENetEventType, std::function<void(ENetEvent&)>> event_pool
{
    {ENET_EVENT_TYPE_CONNECT, &_connect},
    {ENET_EVENT_TYPE_DISCONNECT, &disconnect},
    {ENET_EVENT_TYPE_RECEIVE, &receive}
};
```

### Connect Handler
- Checks server capacity
- Initializes peer data
- Sends connection acknowledgment

### Receive Handler
- Parses packet type (2, 3, 4)
- Routes to action_pool or state_pool
- Manages packet lifecycle

### Disconnect Handler
- Removes player from world
- Saves player data
- Cleans up resources

---

## Packet Creation System

### Packet Namespace (`include/packet/packet.hpp`)

```cpp
namespace packet
{
    void create(ENetPeer& p, bool netid, signed delay, const std::vector<std::any>& params);
    void action(ENetPeer& p, const std::string& action, const std::string& str);
}
```

### Packet Types

**`packet::create`** - Creates structured game packets:
- `netid`: true for player-related, false for system
- `delay`: Packet delay in milliseconds
- `params`: Type-safe parameters (const char*, int, u_int, float)

**`packet::action`** - Creates action packets:
- Format: `action|{action_name}\n{data}`

### Server Response Packets (On*)

| Packet | Description |
|--------|-------------|
| `OnConsoleMessage` | Chat/system message |
| `OnTalkBubble` | Speech bubble |
| `OnSpawn` | Spawn player |
| `OnRemove` | Remove player |
| `OnSetClothing` | Update appearance |
| `OnSetBux` | Update gems |
| `OnSetPos` | Set position |
| `OnSetFreezeState` | Freeze/unfreeze |
| `OnKilled` | Player death |
| `OnNameChanged` | Name update |
| `OnCountryState` | Country/level display |
| `OnBillboardChange` | Name tag update |
| `OnEmoticonDataChanged` | Emote unlock |
| `OnRequestWorldSelectMenu` | World selection UI |
| `OnStoreRequest` | Store interface |
| `OnDialogRequest` | Dialog window |
| `OnTextOverlay` | Center overlay |
| `OnFailedToEnterWorld` | World entry failed |

---

## Utility Functions

### String Utilities (`include/tools/string.hpp`)

```cpp
std::vector<std::string> readch(const std::string &str, char c);  // Split string
std::string join(const std::vector<std::string>& range, const std::string& del);
bool alpha(const std::string& str);    // Alphabetic check
bool number(const std::string& str);   // Numeric check
bool alnum(const std::string& str);    // Alphanumeric check
std::string base64_decode(const std::string& encoded);
std::size_t fnv1a(const std::string& value);  // Hash function
```

### Random Utilities (`include/tools/ransuu.hpp`)

```cpp
class ransuu {
    int operator[](Range _range);      // Random int in range
    float shosu(Range _range, float right);  // Random float
};
```

### Dialog Builder (`include/tools/create_dialog.hpp`)

Fluent interface for dialog creation with methods:
- `set_default_color`, `add_label`, `add_label_with_icon`
- `add_textbox`, `add_text_input`, `add_smalltext`
- `embed_data`, `add_spacer`, `add_button`
- `add_image_button`, `add_custom_button`
- `add_quick_exit`, `end_dialog`

---

## Holiday System

### Holiday Detection (`include/automate/holiday.hpp`)

```cpp
enum holiday : u_char {
    H_NONE,
    H_VALENTINES
};

void check_for_holiday();  // Checks system time
std::string game_theme_string();  // Returns theme name
std::pair<std::string, std::string> holiday_greeting();
```

### Valentine's Week
- Activates: February 13-20
- Theme: `valentines-theme`
- Special greeting and gazette content

---

## World Generation

### Default World Generation

```cpp
void generate_world(world &world, const std::string& name)
{
    // Creates 100x60 world (6000 blocks)
    // Main door at random x position (2-58)
    // Terrain layers:
    // - Y 0-36: Air
    // - Y 37: Cave background starts
    // - Y 38-49: Random rocks
    // - Y 50-53: Lava level
    // - Y 54+: Bedrock
}
```

### Blast Worlds

**Thermonuclear Blast** (ID: 1402):
- Creates completely flat world
- Only bedrock layer at Y 54+
- Main door at random position

---

## Player States

### State Flags (`pstate`)

```cpp
enum pstate : int {
    S_GHOST = 0x00000001,       // Ghost mode
    S_DOUBLE_JUMP = 0x00000002, // Double jump
    S_DUCT_TAPE = 0x00002000    // Duct tape effect
};
```

### Role System

```cpp
enum role : u_char {
    PLAYER,      // 0 - White prefix
    DOU_ZUN,   // 1 - #c prefix
    BAN_SHENG    // 2 - 8@ prefix
};
```

### Prefix Colors

- `w` - White (default player)
- `2` - Green (world owner)
- `c` - Cyan (world admin)
- `#c` - Cyan mod prefix
- `8@` - BAN_SHENG prefix

---

## Weather System

### Weather Machines

Weather machines toggle world weather when punched twice:

| Item ID | Weather ID | Name |
|---------|------------|------|
| 830 | 1 | Beach Blast |
| 934 | 2 | Night |
| 946 | 3 | Arid |
| 984 | 5 | Rainy City |
| 1060 | 6 | Harvest Moon |
| 1136 | 7 | Mars |
| 1210 | 8 | Spooky |
| 1490 | 10 | Nothingness |
| 1364 | 11 | Snowy |
| 1532 | 14 | Undersea |
| 1750 | 15 | Warp Speed |
| 2046 | 17 | Comet |
| 2284 | 18 | Party |
| 2744 | 19 | Pineapples |
| 3252 | 20 | Snowy Night |
| 3446 | 21 | Spring |
| 3534 | 22 | Howling Sky |
| 3694 | 23 | Heatwave |
| 3832 | 29 | Stuff |
| 4242 | 30 | Pagoda |
| 4486 | 31 | Apocalypse |
| 4774 | 32 | Jungle |
| 4892 | 33 | Balloon Warz |
| 5000 | 34 | Background |
| 5112 | 35 | Autumn |
| 5654 | 36 | Valentine's |
| 5716 | 37 | St. Paddy's |
| 6854 | 42 | Digital Rain |
| 7380 | 43 | Monochrome |
| 7644 | 44 | Frozen Cliffs |
| 8556 | 45 | Surg World |
| 8738 | 46 | Bountiful |
| 8836 | 47 | Stargazing |
| 8896 | 48 | Meteor Shower |
| 10286 | 51 | Celebrity Hills |
| 11880 | 59 | Plaza |
| 12054 | 60 | Nebula |
| 12056 | 61 | Protostar Landing |
| 12408 | 62 | Dark Mountains |
| 12844 | 64 | Mt. Growmore |
| 13004 | 65 | Crack in Reality |
| 13070 | 66 | Nian Mountains |
| 13640 | 69 | Realm of Spirits |
| 13690 | 70 | Black Hole |
| 14032 | 71 | Raining Gems |
| 14094 | 72 | Holiday Heaven |
| 14598 | 76 | Atlantis |
| 14802 | 77 | Petal Purrfect Haven |
| 14896 | 78 | Candyland Blast |
| 15150 | 79 | Dragon's Keep |
| 15240 | 80 | Emerald City |

---

## Emoticon System

### Available Emoticons

56 emoticons available, mapped to Unicode characters:

| Emote | Code | Emote | Code |
|-------|------|-------|------|
| wl | ā | yes | â |
| no | ã | love | ä |
| oops | å | shy | æ |
| wink | ç | tongue | è |
| agree | é | sleep | ê |
| punch | ë | music | ì |
| build | í | megaphone | î |
| sigh | ï | mad | ð |
| wow | ñ | dance | ò |
| bheart | ô | heart | õ |
| grow | ö | gems | ö |
| kiss | ø | gtoken | ù |
| lol | ú | smile | û |
| cool | ü | cry | ý |
| bunny | þ | cactus | ÿ |
| ... | ... | ... | ... |

---

## Anti-Spam System

### Chat Rate Limiting

```cpp
std::deque<steady_clock::time_point> messages;  // Last 5 messages

// Check: 5 messages within 6 seconds = spam
if (messages.size() == 5 && 
    duration_cast<seconds>(now - messages.front()).count() < 6)
{
    // Block message, show warning
}
```

---

## Key Features Summary

### Implemented Features

1. **Authentication System**: Full login via HTTPS server
2. **World Management**: Create, load, save worlds dynamically
3. **Player Database**: Persistent storage for all player data
4. **Item System**: Complete items.dat parsing with all item types
5. **Store System**: Functional gem/growtoken store
6. **Command System**: Admin and utility commands
7. **Chat System**: World chat, broadcasts, emotes
8. **Weather System**: 40+ weather machines
9. **Holiday System**: Automatic holiday detection
10. **Friend System**: Friend list management
11. **Billboard System**: Player name tags with ads
12. **Drop System**: Item drops with physics
13. **Block Interaction**: Full block breaking/placement
14. **Special Blocks**: Providers, weather machines, locks, etc.
15. **PvP License**: Card battle system UI
16. **Level System**: XP gain and level progression
17. **Role System**: Player, DOU_ZUN, BAN_SHENG roles
18. **Anti-Spam**: Chat rate limiting

### Notable Implementation Details

- **Memory-Efficient**: Uses `std::span` for peer iteration
- **Thread-Safe**: HTTPS listener runs on separate thread
- **RAII Pattern**: Database connections use RAII
- **Modern C++**: Uses C++23 features (std::ranges, std::format)
- **Type Safety**: Packet creation uses std::any for type checking
- **Fluent Interfaces**: Dialog builder uses fluent pattern

---

## Configuration Files

### `resources/store.txt`

Store item definitions:
```
tab_id|btn_id|name|texture|description|tex1|tex2|cost|items
0|gems_bundle|Gem Pack|btn|Description|1|2|1000|112:100
```

### `resources/ctx/`

SSL certificates:
- `server.crt` - HTTPS certificate
- `server.key` - Private key

---

## Build Requirements

### Dependencies

- **Compiler**: GCC with C++23 support
- **Libraries**:
  - ENet 1.3.18
  - OpenSSL 3.6.1
  - SQLite3 3.51.2
  - WinMM (Windows)
  - WS2_32 (Windows)

### Build Commands

**Windows (MSYS2):**
```bash
pacman -S --needed mingw-w64-ucrt-x86_64-{gcc,openssl,sqlite} make
make -j$(nproc)
```

**Linux:**
```bash
sudo apt-get install build-essential libssl-dev libsqlite3-dev
make -j$(nproc)
```

---

## File Requirements

### Required Files

- `items.dat` - Item database (must be provided)
- `resources/store.txt` - Store configuration
- `resources/ctx/server.crt` - SSL certificate
- `resources/ctx/server.key` - SSL private key

### Generated Files

- `db/peers.db` - Player database
- `db/worlds.db` - World database

---

## Network Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 443 | HTTPS | Login server |
| 17091 | UDP (ENet) | Game server |

---

## Hosts File Configuration

For local testing, add to hosts file:

**Windows:** `C:\Windows\System32\drivers\etc\hosts`
**Linux:** `/etc/hosts`

```
127.0.0.1 www.growtopia1.com
127.0.0.1 www.growtopia2.com
```

---

## Credits

- **ENet**: UDP networking library
- **OpenSSL**: Cryptography and SSL/TLS
- **SQLite**: Embedded database
- **Growtopia**: Game inspiration

---

*Documentation generated based on source code analysis - 2026*
