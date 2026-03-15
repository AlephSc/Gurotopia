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

```cpp
// Each punch increments hit counter
(block.fg == 0) ? ++block.hits.back() : ++block.hits.front();

// Check if block breaks
if (block.hits.front() >= item->hits) block.fg = 0, block.hits.front() = 0;
else if (block.hits.back() >= item->hits) block.bg = 0, block.hits.back() = 0;
else return; // Block didn't break yet
```

#### Damage Visuals

```cpp
void tile_apply_damage(ENetEvent& event, state state, block &block, u_int value)
{
    // value is encoded in upper byte: (value << 24) | 0x000008
    state.type = (value << 24) | 0x000008;  // Packet type 0x08 with damage value
    state.id = 6;  // Unknown ID
    state.netid = peer->netid;
    state_visuals(*event.peer, std::move(state));
}
```

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

### Block Placement System

```cpp
else  // Placing a block
{
    // Check if placing on existing block
    if (block.fg != 0)
    {
        bool update_tile = false;
        switch (items[block.fg].type)
        {
            case type::DISPLAY_BLOCK:
            {
                world->displays.emplace_back(::display(item->id, state.punch));
                update_tile = true;
                break;
            }
            case type::SEED:  // Splicing
            {
                for (::item &splice_item : items)
                {
                    if ((splice_item.splice[0] == state.id && splice_item.splice[1] == block.fg) ||
                        (splice_item.splice[1] == state.id && splice_item.splice[0] == block.fg))
                    {
                        // Successful splice!
                        block.tick = steady_clock::now();
                        block.fg = splice_item.id;  // New tree
                        update_tile = true;
                        break;
                    }
                }
                break;
            }
        }
        if (update_tile)
            send_tile_update(event, state, block, *world);
        return;
    }
    
    // Collision check
    if (item->collision == collision::FULL)
    {
        if (state.punch.x == state.pos.x && state.punch.y == state.pos.y)
            return;  // Can't place in player's position
    }
    
    // Special placement handling
    switch (item->type)
    {
        case type::LOCK:
        {
            if (!world->owner)
            {
                world->owner = peer->user_id;
                lock_visuals = true;
                
                // Update prefix to green for world owner
                if (!peer->role)
                {
                    peer->prefix.front() = '2';
                    on::NameChanged(event);
                }
                
                // Add to player's worlds list
                if (std::ranges::find(peer->my_worlds, world->name) == peer->my_worlds.end())
                {
                    std::ranges::rotate(peer->my_worlds, peer->my_worlds.begin() + 1);
                    peer->my_worlds.back() = world->name;
                }
                
                // Announce world lock
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
            block.state3 |= S_PUBLIC;  // Default to public
            break;
        }
        case type::PROVIDER:
        case type::SEED:
        {
            block.tick = steady_clock::now();  // Start timer
            if (item->type == type::SEED)
                block.state3 |= 0x11;  // Growth state
            break;
        }
    }
    
    // Set placement direction
    block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;
    
    // Place block
    (item->type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
    
    // Remove from inventory
    peer->emplace(::slot(item->id, -1));
    
    // Send visuals
    state.netid = peer->netid;
    state_visuals(*event.peer, std::move(state));
    
    // Send lock visuals if world lock
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
}
```

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

## Database System

### Peer Database (`db/peers.db`)

**Tables:**

```sql
-- Player core data
CREATE TABLE peers (
    _n TEXT PRIMARY KEY,      -- GrowID
    role INTEGER,             -- 0: Player, 1: Moderator, 2: Developer
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
    u_char role;                    // PLAYER, MODERATOR, DEVELOPER
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
    MODERATOR,   // 1 - #c prefix
    DEVELOPER    // 2 - 8@ prefix
};
```

### Prefix Colors

- `w` - White (default player)
- `2` - Green (world owner)
- `c` - Cyan (world admin)
- `#c` - Cyan mod prefix
- `8@` - Developer prefix

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
17. **Role System**: Player, Moderator, Developer roles
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
