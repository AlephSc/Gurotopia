# Gurotopia - Function Reference Documentation

Complete reference documentation for all functions in the Gurotopia server.

---

## Table of Contents

1. [Main Entry Point](#main-entry-point)
2. [Action Functions](#action-functions)
3. [Command Functions](#command-functions)
4. [State Functions](#state-functions)
5. [Event Handler Functions](#event-handler-functions)
6. [Packet Functions](#packet-functions)
7. [Database Functions](#database-functions)
8. [Tool/Utility Functions](#toolutility-functions)
9. [Dialog Functions](#dialog-functions)
10. [ON/Response Functions](#onresponse-functions)
11. [Automate Functions](#automate-functions)
12. [HTTPS Functions](#https-functions)

---

## Main Entry Point

### `main()`

**File:** `main.cpp`

**Signature:**
```cpp
int main()
```

**Purpose:**
Server entry point. Initializes all systems and starts the main game loop.

**Execution Flow:**
1. Sets up signal handlers for graceful shutdown
2. Prints library version information
3. Creates `db/` directory for databases
4. Initializes ENet networking
5. Creates server host on configured port
6. Starts HTTPS listener thread
7. Enables compression and checksum
8. Loads game data (items, store, holidays)
9. Enters main event loop

**Code:**
```cpp
int main()
{
    // Signal handlers for Ctrl+C
    std::signal(SIGINT, safe_disconnect_peers);
#ifdef SIGHUP
    std::signal(SIGHUP, safe_disconnect_peers);  // Unix PUTTY/SSH
#endif

    // Print version info
    std::printf("ZTzTopia/enet %d.%d.%d\n", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    std::printf("sqlite/sqlite3 %s\n", sqlite3_libversion());
    std::printf("openssl/openssl %s\n", OpenSSL_version(OPENSSL_VERSION_STRING));

    // Create database directory
    std::filesystem::create_directory("db");

    // Initialize ENet
    enet_initialize();
    {
        g_server_data = init_server_data();
        ENetAddress address{
            .type = ENET_ADDRESS_TYPE_IPV4,
            .port = g_server_data.port
        };

        host = enet_host_create(ENET_ADDRESS_TYPE_IPV4, &address, 50zu, 2zu, 0, 0);
        std::thread(&https::listener).detach();  // HTTPS login server
    }
    host->usingNewPacketForServer = true;
    host->checksum = enet_crc32;
    enet_host_compress_with_range_coder(host);

    // Load game data
    decode_items();      // Parse items.dat
    parse_store();       // Parse store.txt
    check_for_holiday(); // Check for active holidays

    // Main event loop
    ENetEvent event{};
    while (true)
        while (enet_host_service(host, &event, 1000/*ms*/) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);

    return 0;
}
```

**Returns:**
- `0` on normal exit (never reached in practice)

**Notes:**
- Server runs indefinitely until SIGINT/SIGHUP
- HTTPS listener runs in separate thread
- Event loop processes ENet events with 1 second timeout

---

## Action Functions

Action functions handle client-initiated requests (packets type 2 and 3).

### `action::protocol()`

**File:** `include/action/protocol.hpp`, `include/action/protocol.cpp`

**Signature:**
```cpp
void action::protocol(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event containing peer connection
- `header` - Raw packet data string

**Purpose:**
Handles login authentication by decoding base64 ltoken to extract growId and password.

**Process:**
1. Splits header by `|` delimiter
2. Validates packet has at least 4 parts
3. Checks if packet type is "ltoken"
4. Decodes base64 token
5. Extracts `growId=` and `password=` from decoded string
6. Validates credentials (currently accepts any)
7. Sends `SetHasGrowID` packet
8. Sends `OnSendToServer` redirect to game server

**Code:**
```cpp
void action::protocol(ENetEvent& event, const std::string& header)
{
    std::string growid{}, password{};
    try
    {
        std::vector<std::string> pipes = readch(header, '|');
        if (pipes.size() < 4zu) throw std::runtime_error("");

        if (pipes[2zu] == "ltoken")
        {
            const std::string decoded = base64_decode(pipes[3zu]);
            if (decoded.empty()) throw std::runtime_error("");

            if (std::size_t pos = decoded.find("growId="); pos != std::string::npos)
            {
                pos += sizeof("growId=")-1zu;
                growid = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("password="); pos != std::string::npos)
            {
                pos += sizeof("password=")-1zu;
                password = decoded.substr(pos);
            }
        }
        if (growid.empty() || password.empty()) throw std::runtime_error("");
    }
    catch (...) {
        packet::action(*event.peer, "logon_fail", "");
        return;
    }

    packet::create(*event.peer, false, 0, {"SetHasGrowID", 1, growid.c_str(), ""});

    packet::create(*event.peer, false, 0, {
        "OnSendToServer",
        (signed)g_server_data.port,
        0,
        (signed)fnv1a(growid),
        std::format("{}|0|0", g_server_data.server).c_str(),
        1,
        growid.c_str()
    });
}
```

**Returns:** `void`

**Packets Sent:**
- `SetHasGrowID` - Confirms growId ownership
- `OnSendToServer` - Redirects to game server (triggers `PACKET_DISCONNECT`)

---

### `action::tankIDName()`

**File:** `include/action/tankIDName.hpp`, `include/action/tankIDName.cpp`

**Signature:**
```cpp
void action::tankIDName(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Client identification data

**Purpose:**
Processes client identification packet containing player info (growId, game version, country, etc).

**Data Extracted:**
- `tankIDName` - Player's growId
- `game_version` - Client version
- `country` - Country code
- `user` - User ID

**Process:**
1. Splits header by `|`
2. Validates header has at least 41 parts
3. Extracts ltoken, game_version, country, user_id
4. Loads player data from database via `peer::read()`
5. Sends GDPR override packet
6. Sends role skins and titles packet
7. Sends game theme string
8. Sends super main start packet with full server configuration

**Packets Sent:**
- `OnOverrideGDPRFromServer`
- `OnSetRoleSkinsAndTitles`
- `OnSuperMainStartAcceptLogonHrdxs47254722215a` - Full server config
- `OnEventButtonDataSet` - Event button configuration

---

### `action::enter_game()`

**File:** `include/action/enter_game.hpp`, `include/action/enter_game.cpp`

**Signature:**
```cpp
void action::enter_game(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused

**Purpose:**
Handles player entering the game after successful login.

**Process:**
1. Checks if player has no items (new player)
   - Gives Fist (ID: 18), Wrench (ID: 32), My First World Lock (ID: 9640)
2. Sets prefix based on role
3. Sends welcome message
4. Sends holiday greeting
5. Sends inventory state
6. Sends gem count (SetBux)
7. Sends pearl and voucher counts
8. Sends SetHasGrowID
9. Sends today's date
10. Shows world select menu
11. Shows Gazette (news) dialog
12. Sends ping request

**Packets Sent:**
- `OnFtueButtonDataSet`
- `OnEventButtonDataSet` (multiple)
- `OnConsoleMessage` (welcome, holiday, settings)
- Inventory state packet
- `OnSetBux`
- `OnSetPearl`
- `OnSetVouchers`
- `SetHasGrowID`
- `OnTodaysDate`
- `OnRequestWorldSelectMenu`
- `OnDialogRequest` (Gazette)
- Ping request (type 0x16)

---

### `action::dialog_return()`

**File:** `include/action/dialog_return.hpp`, `include/action/dialog_return.cpp`

**Signature:**
```cpp
void action::dialog_return(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Dialog response data

**Purpose:**
Routes dialog responses to appropriate handlers based on dialog name.

**Process:**
1. Splits header by `|`
2. Validates header has at least 4 parts (dialog name at index 3)
3. Looks up handler in `dialog_return_pool`
4. Calls handler with event and pipes

**Routing:**
```cpp
std::vector<std::string> pipes = readch(header, '|');
if (pipes.size() <= 3zu) return;

if (const auto it = dialog_return_pool.find(pipes[3zu]); it != dialog_return_pool.end())
    it->second(std::ref(event), std::move(pipes));
```

**Dialog Handlers:**
- `lock_edit` - Lock configuration
- `door_edit` - Door/portal editing
- `sign_edit` - Sign text editing
- `gateway_edit` - Entrance configuration
- `drop_item` - Item dropping
- `trash_item` - Item recycling
- `find_item` - Item search (debug)
- `create_blast` - World creation
- `peer_edit` - Player editing
- `billboard_edit` - Name tag editing
- `megaphone` - Global broadcast
- `socialportal` - Friends list
- `popup` - Generic popup
- `unfavorite_items_dialog` - Favorites management
- `vending` - Vending machine
- `continue` - Generic continue dialog

---

### `action::buy()`

**File:** `include/action/buy.hpp`, `include/action/buy.cpp`

**Signature:**
```cpp
void action::buy(ENetEvent& event, const std::string& header, const std::string_view selection = "")
```

**Parameters:**
- `event` - ENet event
- `header` - Purchase request data
- `selection` - Optional selected item for store navigation

**Purpose:**
Handles store purchases from all tabs (main, locks, itempack, bigitems, weather, token).

**Process:**
1. Splits header and validates format
2. Determines store tab from `pipes[3]`
3. If tab != 0 (not main):
   - Builds store request with tab-specific items
   - Handles backpack upgrade pricing formula: `100 * No * No - 200 * No + 200`
   - Shows/hides tabs based on current selection
   - Returns early
4. If tab == 0 (main/purchase):
   - Finds item in `shouhin_tachi`
   - Validates player has enough gems/growtokens
   - Handles special items:
     - `basic_splice` - Splicing kit with 10 random rarity 2 seeds
     - `rare_seed` - 5 random seeds (rarity 13-60)
     - `clothes_pack` - 3 random clothing (rarity ≤10)
     - `rare_clothes_pack` - 3 random clothing (rarity 13-60)
   - Adds items to inventory
   - Handles backpack sprite (ID 9412) specially (adds 10 slots)
   - Deducts gems/growtokens
   - Sends purchase result message

**Purchase Result Format:**
```
You've purchased {item_name} for ${cost} Gems.
You have ${remaining} Gems left.

Received: {item_list}
```

**Returns:** `void`

---

### `action::drop()`

**File:** `include/action/drop.hpp`, `include/action/drop.cpp`

**Signature:**
```cpp
void action::drop(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Drop request with item ID

**Purpose:**
Shows drop dialog for selected item.

**Process:**
1. Extracts item ID from `pipes[4]`
2. Finds item in items database
3. Checks if item is untradeable (`CAT_UNTRADEABLE`)
   - Shows "You can't drop that." if true
4. Searches player inventory for item
5. Shows drop dialog with quantity input

**Dialog Format:**
```
set_default_color|`o
add_label_with_icon|big|`wDrop {item_name}``|left|{item_id}|
add_textbox|How many to drop?|
add_text_input|count||{max_count}|5|
embed_data|itemID|{item_id}
end_dialog|drop_item|||
```

**Returns:** `void`

---

### `action::wrench()`

**File:** `include/action/wrench.hpp`, `include/action/wrench.cpp`

**Signature:**
```cpp
void action::wrench(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Wrench request with netID

**Purpose:**
Shows wrench menu for self or other players.

**Process:**
1. Extracts netID from `pipes[4]`
2. Searches for player in same world
3. If found:
   - **Wrenching self** (`_p->user_id == peer->user_id`):
     - Shows full wrench menu with:
       - Player info (level, XP)
       - Renew PvP license button
       - Profile customization buttons (17 buttons total)
       - Active effects
       - Fires removed counter
       - Backpack slots info
       - Current world position
       - Play time stats
   - **Wrenching others**:
     - Shows player info
     - Trade button
     - Send PM button
     - Add friend button
     - View clothes button
     - Ignore/Report buttons

**Returns:** `void`

---

### `action::friends()`

**File:** `include/action/friends.hpp`, `include/action/friends.cpp`

**Signature:**
```cpp
void action::friends(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused

**Purpose:**
Opens social portal dialog.

**Dialog Options:**
- Show Friends
- Community Hub
- Show Apprentices
- Create Guild
- Trade History

**Returns:** `void`

---

### `action::store()`

**File:** `include/action/store.hpp`, `include/action/store.cpp`

**Signature:**
```cpp
void action::store(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Store request (empty for main store)

**Purpose:**
Opens the main store interface.

**Process:**
1. If header empty or `pipes[3] == "gem"`:
   - Builds store request with all tabs
   - Adds banners and featured items
   - Sets payment site URL (configurable)
   - Sends `OnStoreRequest` packet

**Store Tabs:**
- `main_menu` - Home (gem packs)
- `locks_menu` - Locks And Stuff
- `itempack_menu` - Item Packs
- `bigitems_menu` - Awesome Items
- `weather_menu` - Weather Machines
- `token_menu` - Growtoken Items

**Returns:** `void`

---

### `action::storenavigate()`

**File:** `include/action/storenavigate.hpp`, `include/action/storenavigate.cpp`

**Signature:**
```cpp
void action::storenavigate(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Navigation request

**Purpose:**
Handles store tab navigation and item selection.

**Process:**
1. Extracts `item` and `selection` from pipes
2. Calls `buy()` with selection parameter

**Returns:** `void`

---

### `action::join_request()`

**File:** `include/action/join_request.hpp`, `include/action/join_request.cpp`

**Signature:**
```cpp
void action::join_request(ENetEvent& event, const std::string& header, const std::string_view world_name = "")
```

**Parameters:**
- `event` - ENet event
- `header` - Join request data (ignored if world_name provided)
- `world_name` - Optional world name override

**Purpose:**
Handles player joining/entering a world.

**Process:**
1. Gets world name from header or parameter
2. Validates world name (alphanumeric, uppercase)
3. Finds or creates world in `worlds` vector
4. Generates world if new (`generate_world()`)
5. Builds map data packet:
   - World name, dimensions (100x60)
   - All block data (fg, bg, state3, state4)
   - Special block data (locks, doors, signs, providers, etc.)
   - Weather machine state
   - Object/drop data
6. Sends map data packet
7. Updates player's recent worlds list
8. Sends emoticon data
9. Sets player prefix based on world role
10. Assigns netID
11. Spawns player for others
12. Spawns others for player
13. Sends spawn packet for self
14. Sends billboard if equipped
15. Sends position packet
16. Sends enter world message
17. Increments world visitor count
18. Sends clothing and country state

**Error Handling:**
- Catches exceptions and sends `OnFailedToEnterWorld`

**Returns:** `void`

---

### `action::quit_to_exit()`

**File:** `include/action/quit_to_exit.hpp`, `include/action/quit_to_exit.cpp`

**Signature:**
```cpp
void action::quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused
- `skip_selection` - If true, don't show world select menu

**Purpose:**
Handles player leaving current world.

**Process:**
1. Finds player's current world
2. Notifies all players in world:
   - Sends leave message
   - Sends `OnRemove` packet
3. Decrements world visitor count
4. Removes world if no visitors left
5. Resets player netID to 0
6. Resets prefix to 'w' if was owner/admin
7. Shows world select menu if not skipping

**Returns:** `void`

---

### `action::respawn()`

**File:** `include/action/respawn.hpp`, `include/action/respawn.cpp`

**Signature:**
```cpp
void action::respawn(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused

**Purpose:**
Handles player respawning after death.

**Process:**
1. Sends freeze state (value 2)
2. Sends `OnKilled` packet
3. Waits 1900ms (via packet delay)
4. Sends position packet to respawn position
5. Sends unfreeze packet

**Returns:** `void`

---

### `action::setSkin()`

**File:** `include/action/setSkin.hpp`, `include/action/setSkin.cpp`

**Signature:**
```cpp
void action::setSkin(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Skin color value

**Purpose:**
Changes player's skin color.

**Process:**
1. Extracts skin color from `pipes[3]`
2. Converts to integer
3. Updates `peer->skin_color`
4. Sends `OnSetClothing` packet

**Error Handling:**
- `invalid_argument` - Shows "id must be a number"
- `out_of_range` - Shows "id is out of range"

**Returns:** `void`

---

### `action::input()`

**File:** `include/action/input.hpp`, `include/action/input.cpp`

**Signature:**
```cpp
void action::input(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Chat input data

**Purpose:**
Handles player chat input and command execution.

**Process:**
1. Extracts text from `pipes[4]`
2. Validates text not empty or whitespace
3. Trims whitespace
4. Adds timestamp to message queue (anti-spam)
5. Checks spam (5 messages in 6 seconds)
6. If starts with `/`:
   - Extracts command name
   - Looks up in `cmd_pool`
   - Executes command handler
   - Shows unknown command if not found
7. If normal chat:
   - Applies duct tape effect if active ("mfmm")
   - Sends talk bubble to world
   - Sends console message to world

**Returns:** `void`

---

### `action::trash()`

**File:** `include/action/trash.hpp`, `include/action/trash.cpp`

**Signature:**
```cpp
void action::trash(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Item ID to trash

**Purpose:**
Shows trash/recycle dialog for selected item.

**Process:**
1. Extracts item ID from `pipes[4]`
2. Finds item in database
3. Checks if item is Fist or Wrench
   - Shows "You'd be sorry if you lost that!" if true
4. Searches player inventory
5. Shows trash dialog with quantity input

**Dialog Format:**
```
set_default_color|`o
add_label_with_icon|big|`4Recycle`` `w{item_name}``|left|{item_id}|
add_textbox|How many to `4destroy``? (you have {count})|
add_text_input|count||0|5|
embed_data|itemID|{item_id}
end_dialog|trash_item|||
```

**Returns:** `void`

---

### `action::info()`

**File:** `include/action/info.hpp`, `include/action/info.cpp`

**Signature:**
```cpp
void action::info(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Item ID to show info for

**Purpose:**
Shows item information dialog.

**Process:**
1. Extracts item ID from `pipes[4]`
2. Finds item in database
3. Builds info dialog:
   - Label with icon
   - Item description
   - Rarity (if < 999)
   - Property descriptions
4. Shows dialog

**Property Descriptions:**
- `01` - "This item can be placed in two directions"
- `02` - "This item has special properties you can adjust with the Wrench"
- `04` - "This item never drops any seeds"
- `08` - "This item can't be destroyed - smashing it will return it to your backpack"
- `10` - "This item can be transmuted"
- `20` - "This item can kill zombies during a Pandemic!"
- `90` - "This item can only be used in World-Locked worlds"

**Returns:** `void`

---

### `action::itemfavourite()`

**File:** `include/action/itemfavourite.hpp`, `include/action/itemfavourite.cpp`

**Signature:**
```cpp
void action::itemfavourite(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Item ID to favorite/unfavorite

**Purpose:**
Toggles favorite status for an item.

**Process:**
1. Extracts item ID from `pipes[4]`
2. Checks if already favorited
3. Validates favorite count (< 20)
4. Sends `OnFavItemUpdated` packet
5. Adds/removes from `peer->fav` list

**Returns:** `void`

---

### `action::inventoryfavuitrigger()`

**File:** `include/action/inventoryfavuitrigger.hpp`, `include/action/inventoryfavuitrigger.cpp`

**Signature:**
```cpp
void action::inventoryfavuitrigger(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused

**Purpose:**
Shows favorites help dialog.

**Dialog Content:**
- Explains favorited items are in inventory
- Explains how to unfavorite items

**Returns:** `void`

---

### `action::refresh_item_data()`

**File:** `include/action/refresh_item_data.hpp`, `include/action/refresh_item_data.cpp`

**Signature:**
```cpp
void action::refresh_item_data(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused

**Purpose:**
Sends item data to client (items.dat).

**Process:**
1. Sends compressed item data packet (type 0x10)
2. Includes full items.dat binary data

**Returns:** `void`

---

### `action::quit()`

**File:** `include/action/quit.hpp`, `include/action/quit.cpp`

**Signature:**
```cpp
void action::quit(ENetEvent& event, const std::string& header)
```

**Parameters:**
- `event` - ENet event
- `header` - Unused

**Purpose:**
Handles player disconnect/logout.

**Process:**
1. Calls `quit_to_exit()` to leave world
2. Deletes peer data
3. Resets peer data pointer
4. Resets ENet peer

**Returns:** `void`

---

## Command Functions

Command functions handle chat commands (starting with `/`).

### `find()`

**File:** `include/commands/find.hpp`, `include/commands/find.cpp`

**Signature:**
```cpp
void find(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Command text (unused)

**Purpose:**
Opens item search dialog (debug/admin tool).

**Dialog Format:**
```
set_default_color|`o
add_text_input|n|Search: ||26|
add_searchable_item_list||sourceType:allItems;listType:iconWithCustomLabel;resultLimit:30|n|
add_quick_exit|
end_dialog|find_item|||
```

**Returns:** `void`

---

### `warp()`

**File:** `include/commands/warp.hpp`, `include/commands/warp.cpp`

**Signature:**
```cpp
void warp(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/warp "

**Purpose:**
Teleports player to specified world.

**Process:**
1. Validates world name provided
2. Extracts world name (after "/warp ")
3. Converts to uppercase
4. Logs command
5. Sends freeze state
6. Sends warp message
7. Calls `quit_to_exit()` to leave current world
8. Calls `join_request()` to enter target world

**Usage:**
```
/warp {world name}
```

**Returns:** `void`

---

### `edit()`

**File:** `include/commands/edit.hpp`, `include/commands/edit.cpp`

**Signature:**
```cpp
void edit(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/edit "

**Purpose:**
Opens player editing dialog (admin tool).

**Process:**
1. Validates player name provided
2. Extracts name (after "/edit ")
3. Searches for online player
4. If found online:
   - Shows edit dialog with current values
   - Embeds status=1 (online)
5. If offline:
   - Loads from database
   - Shows edit dialog with stored values
   - Embeds status=0 (offline)

**Dialog Fields:**
- Role (0=Player, 1=DOU_ZUN, 2=BAN_SHENG)
- Level
- Gems

**Usage:**
```
/edit {player name}
```

**Returns:** `void`

---

### `punch()`

**File:** `include/commands/punch.hpp`, `include/commands/punch.cpp`

**Signature:**
```cpp
void punch(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/punch "

**Purpose:**
Sets player's punch effect (clothing effect).

**Process:**
1. Validates ID provided
2. Extracts ID (after "/punch ")
3. Converts to integer
4. Sets `peer->punch_effect`
5. Sends `OnSetClothing` packet

**Usage:**
```
/punch {effect ID}
```

**Error Handling:**
- Shows "Invalid input. id must be a number." on failure

**Returns:** `void`

**Related:**
- `get_punch_id()` - Maps item IDs to punch effect IDs

---

### `skin()`

**File:** `include/commands/skin.hpp`, `include/commands/skin.cpp`

**Signature:**
```cpp
void skin(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/skin "

**Purpose:**
Changes player's skin color (alias for setSkin action).

**Process:**
1. Validates ID provided
2. Extracts ID (after "/skin ")
3. Converts to integer
4. Sets `peer->skin_color`
5. Sends `OnSetClothing` packet

**Usage:**
```
/skin {color ID}
```

**Returns:** `void`

---

### `sb()`

**File:** `include/commands/sb.hpp`, `include/commands/sb.cpp`

**Signature:**
```cpp
void sb(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/sb "

**Purpose:**
Sends server-wide broadcast message.

**Process:**
1. Validates message provided
2. Extracts message (after "/sb ")
3. Gets current world
4. Checks for Signal Jammer (shows "JAMMED")
5. Sends to ALL players on server:
   - Console message with format:
   ```
   CP:0_PL:0_OID:_CT:[SB]_ `5** from (`{prefix}{name}`````5) in [`{world}```5] ** : ````{message}``
   ```

**Usage:**
```
/sb {message}
```

**Returns:** `void`

---

### `who()`

**File:** `include/commands/who.hpp`, `include/commands/who.cpp`

**Signature:**
```cpp
void who(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Unused

**Purpose:**
Lists all players in current world.

**Process:**
1. Gets current world name
2. Iterates all players in world
3. Sends talk bubble for each player
4. Collects names
5. Logs formatted list

**Output Format:**
```
msg|`wWho's in `{world_name}``:`` {player_list}``
```

**Returns:** `void`

---

### `me()`

**File:** `include/commands/me.hpp`, `include/commands/me.cpp`

**Signature:**
```cpp
void me(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/me "

**Purpose:**
Sends emote/action message to world.

**Process:**
1. Validates message provided
2. Extracts message (after "/me ")
3. Sends to all players in world:
   - Talk bubble with format:
   ```
   player_chat= `6<```{prefix}{name}`` `#{message}```6>``
   ```
   - Console message with same format

**Usage:**
```
/me {message}
```

**Returns:** `void`

---

### `weather()`

**File:** `include/commands/weather.hpp`, `include/commands/weather.cpp`

**Signature:**
```cpp
void weather(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Full command text including "/weather "

**Purpose:**
Changes current weather effect.

**Process:**
1. Validates ID provided
2. Extracts ID (after "/weather ")
3. Sends `OnSetCurrentWeather` packet with ID

**Usage:**
```
/weather {weather ID}
```

**Returns:** `void`

**Related:**
- `get_weather_id()` - Maps weather machine item IDs to weather IDs

---

### `ghost()`

**File:** `include/commands/ghost.hpp`, `include/commands/ghost.cpp`

**Signature:**
```cpp
void ghost(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Unused

**Purpose:**
Toggles ghost mode (invisibility).

**Process:**
1. Toggles `S_GHOST` flag in `peer->state`
2. Sends `OnSetClothing` packet (updates appearance)

**Returns:** `void`

---

### `ageworld()`

**File:** `include/commands/ageworld.hpp`, `include/commands/ageworld.cpp`

**Signature:**
```cpp
void ageworld(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Unused

**Purpose:**
Ages all providers and trees in current world by 1 day.

**Process:**
1. Gets current world
2. Iterates all blocks
3. For PROVIDER and SEED types:
   - Subtracts 86400 seconds (1 day) from tick timer
   - Sends tile update
4. Shows confirmation message

**Returns:** `void`

---

### `on::Action()`

**File:** `include/on/Action.hpp`, `include/on/Action.cpp`

**Signature:**
```cpp
void on::Action(ENetEvent& event, const std::string_view text)
```

**Parameters:**
- `event` - ENet event
- `text` - Emote command name

**Purpose:**
Handles emote commands (wave, dance, love, etc.).

**Process:**
1. Sends action packet with emote name

**Supported Emotes:**
wave, dance, love, sleep, facepalm, fp, smh, yes, no, omg, idk, shrug, furious, rolleyes, foldarms, fa, stubborn, fold, dab, sassy, dance2, march, grumpy, shy

**Returns:** `void`

---

## State Functions

State functions handle state packets (packet type 4).

### `movement()`

**File:** `include/state/movement.hpp`, `include/state/movement.cpp`

**Signature:**
```cpp
void movement(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Movement state data

**Purpose:**
Handles player movement updates.

**Process:**
1. Updates `peer->pos` from state
2. Updates `peer->facing_left` from state flags
3. Sets state netID
4. Sends movement visuals to all players in world

**Returns:** `void`

---

### `tile_change()`

**File:** `include/state/tile_change.hpp`, `include/state/tile_change.cpp`

**Signature:**
```cpp
void tile_change(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Tile interaction state

**Purpose:**
Handles all block interactions (punching, placing, using items).

**Lines of Code:** 737

**Process:**
See [Tile Change System - Deep Dive](Documentation.md#tile-change-system---deep-dive) for complete documentation.

**Returns:** `void`

---

### `tile_activate()`

**File:** `include/state/tile_activate.hpp`, `include/state/tile_activate.cpp`

**Signature:**
```cpp
void tile_activate(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Tile activation state

**Purpose:**
Handles tile activation (standing on tiles, etc.).

**Returns:** `void`

---

### `item_activate()`

**File:** `include/state/item_activate.hpp`, `include/state/item_activate.cpp`

**Signature:**
```cpp
void item_activate(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Item activation state with item ID

**Purpose:**
Handles using items from inventory (not placing them).

**Process:**
1. Finds item in items database
2. If clothing item:
   - Toggles equip/unequip
   - Updates punch effects from all equipped clothing
   - Sends `OnEquipNewItem` packet
   - Sends `OnSetClothing` packet
3. If World Lock (242) or Diamond Lock (1796):
   - Handles conversion (100 WL → 1 DL, or 1 DL → 100 WL)
   - Shows compression/shatter message
4. If Growtoken (1486) or Golden Growtoken (6802):
   - Handles conversion (100 GT → 1 GGT, or 1 GGT → 100 GT)

**Returns:** `void`

---

### `item_activate_object()`

**File:** `include/state/item_activate_object.hpp`, `include/state/item_activate_object.cpp`

**Signature:**
```cpp
void item_activate_object(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Object activation state with object UID

**Purpose:**
Handles picking up dropped items/objects.

**Process:**
1. Finds world
2. Finds object by UID
3. Finds item definition
4. If not gem:
   - Shows collection message (with rarity for rare items)
   - Adds to inventory
5. If gem:
   - Adds directly to `peer->gems`
   - Sends `OnSetBux` packet
6. Updates or removes object
7. Erases object if count reaches 0

**Returns:** `void`

---

### `ping_reply()`

**File:** `include/state/ping_reply.hpp`, `include/state/ping_reply.cpp`

**Signature:**
```cpp
void ping_reply(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Ping state

**Purpose:**
Handles ping/keep-alive packets.

**Returns:** `void`

---

### `disconnect()`

**File:** `include/state/disconnect.hpp`, `include/state/disconnect.cpp`

**Signature:**
```cpp
void disconnect(ENetEvent& event, state state)
```

**Parameters:**
- `event` - ENet event
- `state` - Unused

**Purpose:**
Handles state-based disconnect (type 0x1a).

**Process:**
1. Calls `action::quit()`

**Returns:** `void`

---

## Event Handler Functions

### `_connect()`

**File:** `include/event_type/connect.hpp`, `include/event_type/connect.cpp`

**Signature:**
```cpp
void _connect(ENetEvent& event)
```

**Parameters:**
- `event` - ENet connect event

**Purpose:**
Handles new player connections.

**Process:**
1. Checks if server at capacity
   - If full: sends overload message and `logon_fail`
2. If space available:
   - Sends connection acknowledgment packet (01 00 00 00)
   - Allocates new `peer` object for `event.peer->data`

**Returns:** `void`

---

### `disconnect()`

**File:** `include/event_type/disconnect.hpp`, `include/event_type/disconnect.cpp`

**Signature:**
```cpp
void disconnect(ENetEvent& event)
```

**Parameters:**
- `event` - ENet disconnect event

**Purpose:**
Handles player disconnections.

**Process:**
1. Calls `action::quit()`

**Returns:** `void`

---

### `receive()`

**File:** `include/event_type/receive.hpp`, `include/event_type/receive.cpp`

**Signature:**
```cpp
void receive(ENetEvent& event)
```

**Parameters:**
- `event` - ENet receive event with packet data

**Purpose:**
Routes incoming packets to appropriate handlers.

**Process:**
1. Gets packet data as span
2. Switches on packet type (data[0]):
   - **Type 2 or 3** (Action packets):
     - Extracts header string
     - Replaces `\n` with `|`
     - Splits by `|`
     - Determines action name
     - Looks up in `action_pool`
     - Calls handler
   - **Type 4** (State packet):
     - Validates packet size
     - Parses state structure
     - Looks up in `state_pool`
     - Calls handler
3. Destroys packet

**Returns:** `void`

---

## Packet Functions

### `packet::create()`

**File:** `include/packet/packet.hpp`, `include/packet/packet.cpp`

**Signature:**
```cpp
void packet::create(ENetPeer& p, bool netid, signed delay, const std::vector<std::any>& params)
```

**Parameters:**
- `p` - Target peer
- `netid` - If true, uses peer's netID; if false, uses 0xFFFFFFFF
- `delay` - Packet delay in milliseconds
- `params` - Type-safe parameters (const char*, int, u_int, float vector)

**Purpose:**
Creates and sends structured game packets.

**Parameter Types:**
- `const char*` - String (type 0x02)
- `int` - Signed integer (type 0x09)
- `u_int` - Unsigned integer (type 0x05)
- `std::vector<float>` - Float array (type 0x01/0x03/0x04 based on size)

**Packet Structure:**
```
[State header: 60 bytes][Elements: variable]
```

**Element Format:**
```
[Index: 1 byte][Type: 1 byte][Length: 4 bytes (for strings)][Data: variable]
```

**Returns:** `void`

---

### `packet::action()`

**File:** `include/packet/packet.hpp`, `include/packet/packet.cpp`

**Signature:**
```cpp
void packet::action(ENetPeer& p, const std::string& action, const std::string& str)
```

**Parameters:**
- `p` - Target peer
- `action` - Action name (e.g., "log", "quit")
- `str` - Action data

**Purpose:**
Creates and sends action packets.

**Packet Format:**
```
action|{action}\n{str}
```

**Packet Structure:**
```
[Size: 4 bytes][Data: action|{action}\n{str}]
```

**Returns:** `void`

---

## Database Functions

### `peer::read()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
peer& peer::read(const std::string& name)
```

**Parameters:**
- `name` - Player's growId

**Purpose:**
Loads player data from database.

**Process:**
1. Opens peer database
2. Queries `peers` table for role, gems, level, XP
3. Queries `slots` table for inventory
4. Queries `equip` table for equipped clothing
5. Populates peer object fields
6. Returns reference to self

**Returns:** `peer&` (self reference)

---

### `peer::exists()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
bool peer::exists(const std::string& name)
```

**Parameters:**
- `name` - Player's growId

**Purpose:**
Checks if player exists in database.

**Process:**
1. Opens peer database
2. Queries for existence
3. Returns true if found

**Returns:** `bool`

---

### `peer::emplace()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
short peer::emplace(slot s)
```

**Parameters:**
- `s` - Slot {item ID, count}

**Purpose:**
Adds or updates item in inventory.

**Process:**
1. Searches for existing item with same ID
2. If found:
   - Adds count (supports negative for removal)
   - Clamps to max 200
   - Removes if count reaches 0
   - Updates clothing if removed clothing item
   - Returns excess amount
3. If not found:
   - Creates new slot entry
   - Returns 0

**Returns:** `short` - Excess amount if exceeds 200

---

### `peer::add_xp()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
void peer::add_xp(ENetEvent &event, u_short value)
```

**Parameters:**
- `event` - ENet event
- `value` - XP amount to add

**Purpose:**
Adds XP to player and handles level ups.

**XP Formula:**
```
XP needed for level = 50 * (level² + 2)
```

**Process:**
1. Adds XP to current XP
2. Checks for level up in loop
3. If can level up:
   - Subtracts required XP
   - Increments level
   - If level 50: gives Mini Growtopian (ID: 1400)
   - If level 125: sends `OnCountryState` (blue name)
   - Sends particle effect
   - Sends level up announcement
4. Repeats until no more level ups

**Returns:** `void`

---

### `world::world()` (Constructor)

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
world::world(const std::string& name)
```

**Parameters:**
- `name` - World name

**Purpose:**
Loads world data from database.

**Process:**
1. Opens world database
2. Queries `worlds` table for owner, min_level
3. Resizes blocks to 6000
4. Queries `blocks` table
5. Queries `displays` table
6. Queries `random_blocks` table
7. Queries `objects` table
8. Populates world object fields

**Returns:** Constructed world object

---

### `world::~world()` (Destructor)

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
world::~world()
```

**Purpose:**
Saves world data to database.

**Process:**
1. Opens world database
2. Begins transaction
3. Inserts/replaces world metadata
4. Deletes existing blocks
5. Inserts all current blocks
6. Deletes/inserts displays
7. Deletes/inserts random_blocks
8. Deletes/inserts objects
9. Commits transaction

**Returns:** `void`

---

### `send_data()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void send_data(ENetPeer &peer, const std::vector<u_char> &&data)
```

**Parameters:**
- `peer` - Target peer
- `data` - Packet data

**Purpose:**
Sends reliable data packet on channel 1.

**Process:**
1. Creates ENet packet with reliable flag
2. Sends on channel 1

**Returns:** `void`

---

### `state_visuals()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void state_visuals(ENetPeer &peer, state &&state)
```

**Parameters:**
- `peer` - Source peer
- `state` - State data to broadcast

**Purpose:**
Sends state visuals to all players in same world.

**Process:**
1. Gets peer's current world
2. Iterates all players in world
3. Sends compressed state to each

**Returns:** `void`

---

### `tile_apply_damage()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void tile_apply_damage(ENetEvent& event, state state, block &block, u_int value)
```

**Parameters:**
- `event` - ENet event
- `state` - State data
- `block` - Target block
- `value` - Damage value (0-255)

**Purpose:**
Sends block damage visuals.

**Process:**
1. Increments hit counter
2. Encodes damage value in state type: `(value << 24) | 0x000008`
3. Sets state ID to 6
4. Sets netID
5. Sends visuals

**Returns:** `void`

---

### `modify_item_inventory()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
short modify_item_inventory(ENetEvent& event, ::slot slot)
```

**Parameters:**
- `event` - ENet event
- `slot` - Item slot {ID, count} (negative for removal)

**Purpose:**
Adds or removes item from inventory.

**Process:**
1. Sets state type based on count:
   - Negative: `(count*-1 << 16) | 0x000d` (removal)
   - Positive: `(count << 24) | 0x000d` (addition)
2. Sets state ID to item ID
3. Sends visuals
4. Calls `peer::emplace()`

**Returns:** `short` - Excess amount from emplace

---

### `item_change_object()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
int item_change_object(ENetEvent& event, ::slot slot, const ::pos& pos, signed uid = 0)
```

**Parameters:**
- `event` - ENet event
- `slot` - Item slot {ID, count}
- `pos` - Drop position
- `uid` - Object UID (0 for new drop)

**Purpose:**
Creates, updates, or removes dropped items.

**Process:**
1. Finds world
2. Searches for existing object:
   - If found (merge drop):
     - Adds to count
     - Sets netID to 0xFFFFFFFD
     - Updates state
   - If count 0 or ID 0 (remove drop):
     - Sets netID to peer netID
     - Sets UID to 0xFFFFFFFF
   - Else (new drop):
     - Creates new object
     - Sets netID to 0xFFFFFFFF
     - Sets UID to new object UID
3. Sends visuals

**Returns:** `int` - Object UID

---

### `add_drop()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void add_drop(ENetEvent& event, ::slot im, ::pos pos)
```

**Parameters:**
- `event` - ENet event
- `im` - Item slot {ID, count}
- `pos` - Base drop position

**Purpose:**
Creates item drop at position with random offset.

**Process:**
1. Generates random offset (0.07 - 0.50 tiles)
2. Calls `item_change_object()` with offset position

**Returns:** `void`

---

### `send_tile_update()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void send_tile_update(ENetEvent &event, state state, block &b, world& w)
```

**Parameters:**
- `event` - ENet event
- `state` - State data
- `b` - Target block
- `w` - World reference

**Purpose:**
Sends complete tile update data to all players in world.

**Process:**
1. Sets state type to 5 (PACKET_SEND_TILE_UPDATE_DATA)
2. Compresses state
3. Appends block data (fg, bg, state3, state4)
4. Appends special data based on block type:
   - **LOCK**: Owner, admin count, lock state
   - **DOOR/PORTAL/SIGN**: Label text
   - **SEED**: Growth timer, fruit state
   - **PROVIDER**: Production timer
   - **DISPLAY_BLOCK**: Displayed item ID
5. Sends to all players in world

**Returns:** `void`

---

### `remove_fire()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void remove_fire(ENetEvent &event, state state, block &block, world& w)
```

**Parameters:**
- `event` - ENet event
- `state` - State data
- `block` - Target block
- `w` - World reference

**Purpose:**
Extinguishes fire on block.

**Process:**
1. Sends water particle effect
2. Removes S_FIRE flag
3. Sends tile update
4. Increments fires_removed counter
5. If counter % 100 == 0:
   - Awards Highly Combustible Box
   - Shows message
6. Awards 1 XP

**Returns:** `void`

---

### `generate_world()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void generate_world(world &world, const std::string& name)
```

**Parameters:**
- `world` - World object to populate
- `name` - World name

**Purpose:**
Generates default terrain for new world.

**Process:**
1. Creates 6000 blocks (100x60)
2. Sets terrain layers:
   - Y 0-36: Air
   - Y 37: Cave background (ID: 14)
   - Y 38-49: Random rocks (ID: 10) with 1/39 chance
   - Y 50-53: Random lava (ID: 4) with 3/9 chance
   - Y 54+: Bedrock (ID: 8)
3. Places main door (ID: 6) at random X (2-58)
4. Places bedrock below door
5. Sets world name

**Returns:** `void`

---

### `door_mover()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
bool door_mover(world &world, const ::pos &pos)
```

**Parameters:**
- `world` - World reference
- `pos` - New door position

**Purpose:**
Moves main door to new position (Door Mover item).

**Process:**
1. Checks if target position is empty (2 blocks vertically)
2. If not empty: returns false
3. If empty:
   - Finds and removes old door
   - Removes bedrock below old door
   - Places door at new position
   - Places bedrock below new door
   - Returns true

**Returns:** `bool` - Success status

---

### `blast::thermonuclear()`

**File:** `include/database/world.hpp`, `include/database/world.cpp`

**Signature:**
```cpp
void blast::thermonuclear(world &world, const std::string& name)
```

**Parameters:**
- `world` - World object
- `name` - World name

**Purpose:**
Generates flat world (Thermonuclear Blast).

**Process:**
1. Creates 6000 blocks
2. Sets all to air except:
   - Y 54+: Bedrock
   - Main door at random position
   - Bedrock below door
3. Sets world name

**Returns:** `void`

---

### `peers()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
std::vector<ENetPeer*> peers(const std::string &world = "", peer_condition condition = PEER_ALL, std::function<void(ENetPeer&)> fun = [](ENetPeer& peer){})
```

**Parameters:**
- `world` - World name filter (for PEER_SAME_WORLD)
- `condition` - PEER_ALL or PEER_SAME_WORLD
- `fun` - Callback function for each peer

**Purpose:**
Iterates connected peers with optional filtering.

**Process:**
1. Reserves vector for host->peerCount
2. Iterates all peers
3. Checks peer state is CONNECTED
4. If PEER_SAME_WORLD:
   - Checks peer in specified world
5. Calls callback function
6. Adds to result vector

**Returns:** `std::vector<ENetPeer*>` - List of matching peers

---

### `safe_disconnect_peers()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
void safe_disconnect_peers(int signal)
```

**Parameters:**
- `signal` - Exit code

**Purpose:**
Gracefully shuts down server.

**Process:**
1. Prints shutdown message
2. Disconnects all connected peers
3. Deinitializes ENet
4. Prints completion message
5. Exits on Windows

**Returns:** `void`

---

### `send_inventory_state()`

**File:** `include/database/peer.hpp`, `include/database/peer.cpp`

**Signature:**
```cpp
void send_inventory_state(ENetEvent &event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Sends complete inventory state to player.

**Process:**
1. Creates state packet type 0x09
2. Appends slot_size
3. Appends inventory size
4. Appends each slot as: `id | (count << 16)`
5. Sends reliable packet

**Returns:** `void`

---

## Tool/Utility Functions

### `readch()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
std::vector<std::string> readch(const std::string &str, char c)
```

**Parameters:**
- `str` - Input string
- `c` - Delimiter character

**Purpose:**
Splits string by delimiter.

**Process:**
1. Counts occurrences of delimiter
2. Reserves result vector
3. Uses std::views::split to split
4. Emplaces each part into result

**Returns:** `std::vector<std::string>` - Split parts

**Example:**
```cpp
readch("a|b|c", '|')  // Returns: {"a", "b", "c"}
```

---

### `join()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
std::string join(const std::vector<std::string>& range, const std::string& del)
```

**Parameters:**
- `range` - String vector to join
- `del` - Delimiter string

**Purpose:**
Joins strings with delimiter.

**Process:**
1. Iterates strings
2. Appends each string
3. Appends delimiter between strings (not after last)

**Returns:** `std::string` - Joined string

**Example:**
```cpp
join({"a", "b", "c"}, ", ")  // Returns: "a, b, c"
```

---

### `alpha()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
bool alpha(const std::string& str)
```

**Parameters:**
- `str` - Input string

**Purpose:**
Checks if string contains only alphabetic characters.

**Returns:** `bool` - True if all characters are [a-zA-Z]

---

### `number()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
bool number(const std::string& str)
```

**Parameters:**
- `str` - Input string

**Purpose:**
Checks if string contains only numeric characters.

**Returns:** `bool` - True if all characters are [0-9]

---

### `alnum()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
bool alnum(const std::string& str)
```

**Parameters:**
- `str` - Input string

**Purpose:**
Checks if string contains only alphanumeric characters.

**Returns:** `bool` - True if all characters are [a-zA-Z0-9]

---

### `base64_decode()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
std::string base64_decode(const std::string& encoded)
```

**Parameters:**
- `encoded` - Base64 encoded string

**Purpose:**
Decodes base64 string.

**Process:**
1. Creates BIO memory buffer
2. Creates base64 filter
3. Chains BIOs
4. Sets NO_NL flag
5. Reads decoded data
6. Resizes string to actual length
7. Cleans up BIO

**Returns:** `std::string` - Decoded data

---

### `fnv1a()`

**File:** `include/tools/string.hpp`, `include/tools/string.cpp`

**Signature:**
```cpp
std::size_t fnv1a(const std::string& value) noexcept
```

**Parameters:**
- `value` - String to hash

**Purpose:**
Computes FNV-1a hash of string.

**Constants:**
- Offset: `14695981039346656037`
- Prime: `1099511628211`

**Process:**
1. Initializes hash to offset
2. For each character:
   - XOR with hash
   - Multiply by prime
3. Returns final hash

**Returns:** `std::size_t` - 64-bit hash value

---

### `to_char` (Lambda)

**File:** `include/tools/string.hpp`

**Signature:**
```cpp
inline constexpr auto to_char = [](bool b) { return b ? '1' : '0'; };
```

**Purpose:**
Converts boolean to '1' or '0' character.

**Returns:** `char` - '1' if true, '0' if false

---

### `ransuu::operator[]()`

**File:** `include/tools/ransuu.hpp`, `include/tools/ransuu.cpp`

**Signature:**
```cpp
int ransuu::operator[](Range _range)
```

**Parameters:**
- `_range` - Range {min, max}

**Purpose:**
Generates random integer in range.

**Process:**
1. Creates uniform_int_distribution
2. Returns random value

**Returns:** `int` - Random value in [min, max]

---

### `ransuu::shosu()`

**File:** `include/tools/ransuu.hpp`, `include/tools/ransuu.cpp`

**Signature:**
```cpp
float ransuu::shosu(Range _range, float right = 0.1f)
```

**Parameters:**
- `_range` - Range {min, max}
- `right` - Multiplier (default 0.1)

**Purpose:**
Generates random float (decimal).

**Process:**
1. Calls operator[] for integer
2. Multiplies by right

**Returns:** `float` - Random float

**Example:**
```cpp
ransuu.shosu({7, 50}, 0.01f)  // Returns: 0.07 to 0.50
```

---

## Dialog Functions

### `create_dialog` Methods

**File:** `include/tools/create_dialog.hpp`, `include/tools/create_dialog.cpp`

**Class:** `create_dialog`

**Purpose:**
Fluent builder for creating dialog strings.

### `set_default_color()`
```cpp
create_dialog& set_default_color(std::string code)
```
Sets default color code for dialog.

### `add_label()`
```cpp
create_dialog& add_label(std::string size, std::string label)
```
Adds label with size (big, small).

### `add_label_with_icon()`
```cpp
create_dialog& add_label_with_icon(std::string size, std::string label, int icon)
```
Adds label with item icon.

### `add_label_with_ele_icon()`
```cpp
create_dialog& add_label_with_ele_icon(std::string size, std::string label, int icon, u_char element)
```
Adds label with elemental icon.

### `add_textbox()`
```cpp
create_dialog& add_textbox(std::string label)
```
Adds text box with label.

### `add_text_input()`
```cpp
create_dialog& add_text_input(std::string id, std::string label, short set_value, short length)
create_dialog& add_text_input(std::string id, std::string label, std::string set_value, short length)
```
Adds text input field.

### `add_smalltext()`
```cpp
create_dialog& add_smalltext(std::string label)
```
Adds small text.

### `embed_data()`
```cpp
create_dialog& embed_data(std::string id, std::string data)
create_dialog& embed_data(std::string id, int data)
```
Embeds hidden data in dialog.

### `add_spacer()`
```cpp
create_dialog& add_spacer(std::string size)
```
Adds vertical spacer.

### `set_custom_spacing()`
```cpp
create_dialog& set_custom_spacing(short x, short y)
```
Sets custom spacing.

### `add_layout_spacer()`
```cpp
create_dialog& add_layout_spacer(std::string layout)
```
Adds layout spacer.

### `add_button()`
```cpp
create_dialog& add_button(std::string btn_id, std::string btn_name)
```
Adds button.

### `add_image_button()`
```cpp
create_dialog& add_image_button(std::string btn_id, std::string image, std::string layout, std::string link)
```
Adds image button with link.

### `add_custom_button()`
```cpp
create_dialog& add_custom_button(std::string btn_id, std::string image)
```
Adds custom button.

### `add_custom_label()`
```cpp
create_dialog& add_custom_label(std::string label, std::string pos)
```
Adds custom positioned label.

### `add_custom_break()`
```cpp
create_dialog& add_custom_break()
```
Adds custom break.

### `add_custom_margin()`
```cpp
create_dialog& add_custom_margin(short x, short y)
```
Adds custom margin.

### `add_achieve()`
```cpp
create_dialog& add_achieve(std::string total)
```
Adds achievement display.

### `add_quick_exit()`
```cpp
create_dialog& add_quick_exit()
```
Adds quick exit button.

### `add_popup_name()`
```cpp
create_dialog& add_popup_name(std::string popup_name)
```
Sets popup name.

### `add_player_info()`
```cpp
create_dialog& add_player_info(std::string label, std::string progress_bar_name, int progress, int total_progress)
```
Adds player info with progress bar.

### `add_item_picker()`
```cpp
create_dialog& add_item_picker(std::string id, std::string label, std::string selection_prompt)
```
Adds item picker.

### `end_dialog()`
```cpp
std::string end_dialog(std::string btn_id, std::string btn_close = "Cancel", std::string btn_return = "OK")
```
Finalizes and returns dialog string.

---

## ON/Response Functions

Server response packet handlers.

### `on::Spawn()`

**File:** `include/on/Spawn.hpp`, `include/on/Spawn.cpp`

**Signature:**
```cpp
void on::Spawn(ENetPeer &peer, signed netID, signed userID, ::pos posXY, std::string name, std::string country, bool mstate, bool smstate, bool local)
```

**Parameters:**
- `peer` - Target peer
- `netID` - Network ID
- `userID` - User ID
- `posXY` - Position
- `name` - Player name
- `country` - Country code
- `mstate` - Mod state
- `smstate` - Super mod state
- `local` - Is local player

**Purpose:**
Sends spawn packet for player.

**Packet Format:**
```
spawn|avatar
netID|{netID}
userID|{userID}
colrect|0|0|20|30
posXY|{x}|{y}
name|{name}``
country|{country}
invis|0
mstate|{0|1}
smstate|{0|1}
onlineID|
{type|local\n if local}
```

**Returns:** `void`

---

### `on::SetClothing()`

**File:** `include/on/SetClothing.hpp`, `include/on/SetClothing.cpp`

**Signature:**
```cpp
void on::SetClothing(ENetPeer &p)
```

**Parameters:**
- `p` - Target peer

**Purpose:**
Sends player clothing/appearance.

**Process:**
1. Gets peer data
2. Sends `OnSetClothing` with 3 vectors of 3 floats each:
   - {hair, shirt, legs}
   - {feet, face, hand}
   - {back, head, charm}
3. Sends skin color (or -140 for ghost)
4. Sends {ances, 0, 0}
5. Creates character state packet (type 0x14)
6. Encodes punch effect in type: `0x808000 + punch_effect`
7. Sets state flags, position, speed
8. Sends visuals

**Returns:** `void`

---

### `on::SetBux()`

**File:** `include/on/SetBux.hpp`, `include/on/SetBux.cpp`

**Signature:**
```cpp
void on::SetBux(ENetEvent& event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Updates player's gem count.

**Process:**
1. Clamps gems to [0, INT_MAX]
2. Sends `OnSetBux` with gems, 1, 1

**Returns:** `void`

---

### `on::CountryState()`

**File:** `include/on/CountryState.hpp`, `include/on/CountryState.cpp`

**Signature:**
```cpp
void on::CountryState(ENetEvent& event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Sends country and level state.

**Process:**
1. Gets peer data
2. Sends `OnCountryState` with:
   - Country code
   - "|maxLevel" if level 125

**Returns:** `void`

---

### `on::BillboardChange()`

**File:** `include/on/BillboardChange.hpp`, `include/on/BillboardChange.cpp`

**Signature:**
```cpp
void on::BillboardChange(ENetEvent& event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Updates player's billboard (name tag).

**Process:**
1. Gets peer data
2. Sends `OnBillboardChange` with:
   - netID
   - billboard item ID
   - show,isBuying flags
   - price
   - perItem flag

**Returns:** `void`

---

### `on::EmoticonDataChanged()`

**File:** `include/on/EmoticonDataChanged.hpp`, `include/on/EmoticonDataChanged.cpp`

**Signature:**
```cpp
void on::EmoticonDataChanged(ENetEvent& event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Sends unlocked emoticons data.

**Process:**
1. Builds emoticon string from map
2. Format: `({name})|{unicode}|1&`
3. Sends `OnEmoticonDataChanged` with:
   - 201560520 (unknown constant)
   - Emoticon data string

**Returns:** `void`

---

### `on::NameChanged()`

**File:** `include/on/NameChanged.hpp`, `include/on/NameChanged.cpp`

**Signature:**
```cpp
void on::NameChanged(ENetEvent& event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Updates player's display name.

**Process:**
1. Gets peer data
2. Sends `OnNameChanged` with formatted name: `` `{prefix}{name}`` ``

**Returns:** `void`

---

### `on::RequestWorldSelectMenu()`

**File:** `include/on/RequestWorldSelectMenu.hpp`, `include/on/RequestWorldSelectMenu.cpp`

**Signature:**
```cpp
void on::RequestWorldSelectMenu(ENetEvent& event)
```

**Parameters:**
- `event` - ENet event

**Purpose:**
Shows world selection menu.

**Process:**
1. Gets peer data
2. Collects popular worlds (with visitors)
3. Builds menu string:
   - Top Worlds (blue)
   - My Worlds (green)
   - Recently Visited (purple)
4. Sends `OnRequestWorldSelectMenu`
5. Sends "Where would you like to go?" message
6. Sends `OnClearItemTransforms`

**Returns:** `void`

---

## Automate Functions

### `localtime()`

**File:** `include/automate/holiday.hpp`, `include/automate/holiday.cpp`

**Signature:**
```cpp
std::tm localtime()
```

**Purpose:**
Gets current local time.

**Returns:** `std::tm` - Current time structure

---

### `check_for_holiday()`

**File:** `include/automate/holiday.hpp`, `include/automate/holiday.cpp`

**Signature:**
```cpp
void check_for_holiday()
```

**Purpose:**
Checks for active holidays.

**Process:**
1. Gets current time
2. Checks if February 13-20 (Valentine's Week)
3. Sets `holiday` enum

**Returns:** `void`

---

### `game_theme_string()`

**File:** `include/automate/holiday.hpp`, `include/automate/holiday.cpp`

**Signature:**
```cpp
std::string game_theme_string()
```

**Purpose:**
Returns theme string for active holiday.

**Returns:** `std::string` - Theme name (e.g., "valentines-theme")

---

### `holiday_greeting()`

**File:** `include/automate/holiday.hpp`, `include/automate/holiday.cpp`

**Signature:**
```cpp
std::pair<std::string, std::string> holiday_greeting()
```

**Purpose:**
Returns holiday greeting messages.

**Returns:** `std::pair<std::string, std::string>` - {Title, Message}

---

## HTTPS Functions

### `https::listener()`

**File:** `include/https/https.hpp`, `include/https/https.cpp`

**Signature:**
```cpp
void https::listener()
```

**Purpose:**
HTTPS server for login authentication.

**Process:**
1. Initializes OpenSSL
2. Creates SSL context with TLS server method
3. Loads certificate and key from `resources/ctx/`
4. Sets minimum TLS version to 1.2
5. Creates socket
6. Sets socket options (REUSEADDR, DEFER_ACCEPT)
7. Binds to port 443
8. Builds server_data response
9. Enters accept loop:
   - Accepts connection
   - Creates SSL object
   - Performs SSL handshake
   - Reads HTTP request
   - If POST to `/growtopia/server_data.php`:
     - Sends server_data response
   - Shuts down SSL
   - Closes connection

**Server Data Format:**
```
server|{server}
port|{port}
type|{type}
type2|{type2}
#maint|{maint}
loginurl|{loginurl}
meta|{meta}
RTENDMARKERBS1001
```

**Returns:** `void` (runs forever)

---

## Index

### By File

See each function's "File" field.

### By Purpose

- **Networking**: `_connect()`, `disconnect()`, `receive()`, `packet::create()`, `packet::action()`
- **Authentication**: `action::protocol()`, `action::tankIDName()`
- **World Management**: `action::join_request()`, `action::quit_to_exit()`, `generate_world()`, `door_mover()`, `blast::thermonuclear()`
- **Block Interaction**: `tile_change()`, `tile_activate()`, `item_activate()`, `item_activate_object()`
- **Inventory**: `modify_item_inventory()`, `item_change_object()`, `add_drop()`, `send_inventory_state()`
- **Player Data**: `peer::read()`, `peer::exists()`, `peer::emplace()`, `peer::add_xp()`
- **Commands**: `find()`, `warp()`, `edit()`, `punch()`, `skin()`, `sb()`, `who()`, `me()`, `weather()`, `ghost()`, `ageworld()`
- **Dialogs**: `create_dialog` methods, `action::dialog_return()`
- **Store**: `action::store()`, `action::buy()`, `action::storenavigate()`
- **Social**: `action::friends()`, `on::Spawn()`, `on::BillboardChange()`
- **Utilities**: `readch()`, `join()`, `alpha()`, `number()`, `alnum()`, `base64_decode()`, `fnv1a()`

---

*Documentation generated from source code analysis - 2026*
