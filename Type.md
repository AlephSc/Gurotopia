# Gurotopia - Item Types Reference

Complete reference for all item types (`enum type`) used in Gurotopia.

---

## Overview

**File:** `include/database/items.hpp`

**Type:** `enum type : u_char` (0-255)

Item types determine how blocks behave when punched, placed, or interacted with.

---

## Item Types

### `type::FIST` (0x00)
**ID:** `0`

**Description:** Default fist/punch type. Used when player has no item equipped or is punching with fist.

**Behavior:**
- Punches/breaks blocks
- Triggers `state.id == 18` branch in `tile_change()`

**Related Items:**
- Item ID 18 (Fist)

---

### `type::WRENCH` (0x01)
**ID:** `1`

**Description:** Wrench/tool type. Used for interacting with blocks without breaking them.

**Behavior:**
- Opens configuration dialogs for blocks
- Triggers `state.id == 32` branch in `tile_change()`
- Shows lock edit, door edit, sign edit dialogs

**Related Items:**
- Item ID 32 (Wrench)

---

### `type::DOOR` (0x02)
**ID:** `2`

**Description:** Door blocks that can teleport players.

**Behavior:**
- Can be punched to open/close
- Can be configured with destination world/door
- Shows door edit dialog when wrench used

**Related Items:**
- Regular doors
- Portal doors

---

### `type::LOCK` (0x03)
**ID:** `3`

**Description:** Lock blocks that claim world ownership.

**Behavior:**
- First lock placed becomes World Lock
- Sets `world->owner = peer->user_id`
- Changes player prefix to green (`2`)
- Only one World Lock per world
- Can set world to public/private
- Can add admins

**Related Items:**
- World Lock (ID: 202)
- Small Lock (ID: 202)
- Big Lock (ID: 204)
- Huge Lock (ID: 206)
- Builder Lock (ID: 4994)

**Special:**
- `is_tile_lock(id)` checks for small/big/huge/builder locks

---

### `type::GEM` (0x04)
**ID:** `4`

**Description:** Gem currency items.

**Behavior:**
- Added directly to `peer->gems` when collected
- No inventory space needed
- Different denominations (1, 5, 10, 50, 100)

**Related Items:**
- Gem (ID: 112) - various denominations

---

### `type::TREASURE` (0x05)
**ID:** `5`

**Description:** Treasure/chest items containing rewards.

**Behavior:**
- Contains random items
- Opens when punched

**Related Items:**
- Treasure chests
- Pinatas

---

### `type::DEADLY` (0x06)
**ID:** `6`

**Description:** Deadly blocks that damage/kill players.

**Behavior:**
- Kills player on contact
- Triggers respawn

**Related Items:**
- Spikes
- Lava (when deadly)
- Other hazard blocks

---

### `type::TRAMPOLINE` (0x07)
**ID:** `7`

**Description:** Trampoline blocks that bounce players.

**Behavior:**
- Bounces players upward
- Direction based on placement

**Related Items:**
- Trampoline
- Bouncy blocks

---

### `type::CONSUMEABLE` (0x08)
**ID:** `8`

**Description:** Items that are consumed when used.

**Behavior:**
- Used (not placed) when right-clicked
- Removed from inventory after use
- Various effects based on item ID

**Special Cases:**
- **Blast items:** Opens world creation dialog
- **Paint buckets:** Requires paintbrush (ID: 3494)
- **Water bucket:** Extinguishes fire or adds water
- **Pocket lighter:** Starts fire
- **Love Potion #8:** Transforms rock to heartstone
- **Experience potion:** Gives 10000 XP
- **Megaphone:** Opens broadcast dialog
- **Duct tape:** Applies duct tape effect

**Related Items:**
- All blast items (Thermonuclear Blast, etc.)
- Paint buckets (8 colors + vanish)
- Water bucket
- Block glue
- Pocket lighter
- Love Potion #8
- Experience potion
- Megaphone
- Duct tape
- Lollipops (Sour/Sweet)

---

### `type::ENTRANCE` (0x09)
**ID:** `9`

**Description:** Entrance/portal blocks.

**Behavior:**
- Can be set to public/private
- Teleports players
- Shows gateway edit dialog when wrench used

**Related Items:**
- Cave entrance
- Portal entrances

---

### `type::SIGN` (0x0a)
**ID:** `10`

**Description:** Sign blocks with text labels.

**Behavior:**
- Can display custom text
- Text edited via sign edit dialog
- Label stored in `block->label`

**Related Items:**
- All sign variants

---

### `type::SFX_BLOCK` (0x0b)
**ID:** `11`

**Description:** Sound effect blocks.

**Behavior:**
- Plays sound when punched
- Special sound effects

**Related Items:**
- Roulette wheel (ID: 758)
- Other SFX blocks

---

### `type::TOGGLEABLE_ANIMATED_BLOCK` (0x0c)
**ID:** `12`

**Description:** Toggleable blocks with animation.

**Behavior:**
- Can be toggled on/off
- Has animation when toggled
- `block->state3 ^= S_TOGGLE`

**Related Items:**
- Animated toggleable blocks

---

### `type::MAIN_DOOR` (0x0d)
**ID:** `13`

**Description:** Main world entrance door.

**Behavior:**
- Sets player's `rest_pos` (respawn point)
- Labeled "EXIT"
- One per world
- Can be moved with Door Mover item

**Related Items:**
- Main door (placed automatically in world gen)

**Special:**
- Cannot be punched directly
- Message: "(stand over and punch to use)"

---

### `type::PLATFORM` (0x0e)
**ID:** `14`

**Description:** Platform blocks.

**Behavior:**
- Players can stand on top
- Can pass through from below

**Related Items:**
- All platform variants

---

### `type::STRONG` (0x0f)
**ID:** `15`

**Description:** Unbreakable blocks.

**Behavior:**
- Cannot be broken
- Error: "It's too strong to break."

**Related Items:**
- Bedrock (ID: 8)
- Other indestructible blocks

---

### `type::FIRE_PAIN` (0x10)
**ID:** `16`

**Description:** Fire/lava blocks that damage players.

**Behavior:**
- Damages players on contact
- Can be extinguished with water bucket
- Fire hose removes fire and gives XP

**Related Items:**
- Lava (ID: 4)
- Fire blocks

---

### `type::FOREGROUND` (0x11)
**ID:** `17`

**Description:** Foreground blocks.

**Behavior:**
- Placed in foreground layer (`block->fg`)
- Solid collision

**Related Items:**
- Most solid blocks

---

### `type::BACKGROUND` (0x12)
**ID:** `18`

**Description:** Background blocks.

**Behavior:**
- Placed in background layer (`block->bg`)
- Behind foreground blocks

**Related Items:**
- Background variants of blocks
- Pure background blocks

---

### `type::SEED` (0x13)
**ID:** `19`

**Description:** Seed/tree blocks that grow over time.

**Behavior:**
- Grows over time (`block->tick`)
- Can be harvested for fruit
- Can be spliced with other seeds
- `block->state3 |= 0x11` on placement

**Harvesting:**
- When mature: `block.hits[0] = 99` (force break)
- Drops fruit (2-12 count)

**Splicing:**
- Two seeds placed together can splice
- Creates new tree type

**Related Items:**
- All seed variants
- Trees (grown seeds)

---

### `type::CLOTHING` (0x14)
**ID:** `20`

**Description:** Wearable clothing items.

**Behavior:**
- Equipped when used from inventory
- Toggles on/off
- Updates `peer->clothing[clothing_type]`
- Can provide punch effects

**Clothing Slots:**
- `hair` (0)
- `shirt` (1)
- `legs` (2)
- `feet` (3)
- `face` (4)
- `hand` (5)
- `back` (6)
- `head` (7)
- `charm` (8)
- `ances` (9) - Aura

**Related Items:**
- All clothing items

---

### `type::ANIMATED` (0x15)
**ID:** `21`

**Description:** Animated blocks.

**Behavior:**
- Has animation frames
- Visual animation

**Related Items:**
- Animated blocks

---

### `type::SFX_BACKGROUND` (0x16)
**ID:** `22`

**Description:** Background sound effect blocks.

**Behavior:**
- Plays background music/sounds
- Audio effects

**Related Items:**
- Music blocks
- Audio background blocks

---

### `type::ART_WALL` (0x17)
**ID:** `23`

**Description:** Art/wall decoration blocks.

**Behavior:**
- Decorative wall items
- Visual art pieces

**Related Items:**
- Wall art
- Paintings

---

### `type::BOUNCY` (0x18)
**ID:** `24`

**Description:** Bouncy blocks.

**Behavior:**
- Bounces players
- Similar to trampoline

**Related Items:**
- Bouncy blocks

---

### `type::STING_PAIN` (0x19)
**ID:** `25`

**Description:** Stinging/damaging blocks.

**Behavior:**
- Damages players on contact
- Sting effect

**Related Items:**
- Cactus
- Other stinging blocks

---

### `type::PORTAL` (0x1a)
**ID:** `26`

**Description:** Portal blocks for teleportation.

**Behavior:**
- Teleports players
- Can be configured with destination
- Shows portal edit dialog

**Related Items:**
- Portals
- Gateway portals

---

### `type::CHECKPOINT` (0x1b)
**ID:** `27`

**Description:** Checkpoint blocks.

**Behavior:**
- Sets respawn point when touched
- Updates `peer->rest_pos`
- Shows visual feedback

**Related Items:**
- Checkpoint flags
- Save points

---

### `type::MUSIC_SHEET` (0x1c)
**ID:** `28`

**Description:** Music sheet blocks.

**Behavior:**
- Plays musical notes
- Part of music creation system

**Related Items:**
- Music sheets
- Note blocks

---

### `type::SLIPPERY` (0x1d)
**ID:** `29`

**Description:** Slippery ice blocks.

**Behavior:**
- Players slide on surface
- Reduced friction

**Related Items:**
- Ice blocks
- Slippery variants

---

### `type::TOGGLEABLE_BLOCK` (0x1f)
**ID:** `31**

**Description:** Toggleable blocks (on/off).

**Behavior:**
- Can be toggled on/off
- `block->state3 ^= S_TOGGLE`
- Different behavior based on state

**Special Cases:**
- **Signal Jammer (ID: 226):** Hides world from universe when toggled
- **Chests:** Toggle open/closed state

**Related Items:**
- Signal jammer
- Toggleable blocks
- Chests

---

### `type::CHEST` (0x20)
**ID:** `32**

**Description:** Treasure chest blocks.

**Behavior:**
- Stores items
- Opens/closes when punched
- `block->state3 ^= S_TOGGLE`

**Special Drops:**
- Heartstone, GBC, Super GBC have special loot tables

**Related Items:**
- Treasure chests
- Booty chests
- Golden Booty Chest (GBC)
- Super GBC

---

### `type::MAILBOX` (0x21)
**ID:** `33**

**Description:** Mailbox blocks.

**Behavior:**
- Receives messages/items
- Can be toggled (full/empty)

**Related Items:**
- Mailboxes

---

### `type::BULLETIN` (0x22)
**ID:** `34**

**Description:** Bulletin board blocks.

**Behavior:**
- Displays public messages
- Community board

**Related Items:**
- Bulletin boards

---

### `type::PINATA` (0x23)
**ID:** `35**

**Description:** Pinata blocks.

**Behavior:**
- Breaks open when punched
- Drops items/candy

**Related Items:**
- Pinatas

---

### `type::RANDOM` (0x24)
**ID:** `36**

**Description:** Random value blocks.

**Behavior:**
- Stores random value
- Value changes when punched
- Stored in `world->random_blocks`

**Special Cases:**
- **Dice (ID: 456):** Random 0-5
- **Roshambo (ID: 1300):** Random 1-3 (rock/paper/scissors)

**Related Items:**
- Dice
- Roshambo (rock-paper-scissors)

---

### `type::COMPONET` (0x25)
**ID:** `37**

**Description:** Component/crafting parts.

**Behavior:**
- Used in crafting
- Component items

**Related Items:**
- Crafting components

---

### `type::PROVIDER` (0x26)
**ID:** `38**

**Description:** Provider blocks that produce items over time.

**Behavior:**
- Produces items on timer (`block->tick`)
- Can be collected when ready
- Timer resets after collection

**Provider Types:**
- **ATM Machine (ID: 1008):** Produces 1-100 gems (denominated)
- **Chicken (ID: 872):** Produces eggs (1-2)
- **Cow (ID: 866):** Produces steak (1-2)
- **Coffee Maker (ID: 1632):** Produces coffee (1-2)
- **Sheep (ID: 3888):** Produces wool (1-2)
- **Tea Set (ID: 5116):** Produces teacups (1-2)
- **Well (ID: 2798):** Produces water buckets (1-2)
- **Science Station (ID: 928):** Produces random chemicals (P/B/Y/R/G)

**Related Items:**
- All provider blocks

---

### `type::CHEMICAL_COMBINER` (0x27)
**ID:** `39**

**Description:** Chemical combining blocks.

**Behavior:**
- Combines chemicals
- Crafting system

**Related Items:**
- Chemical combiners

---

### `type::ACHIEVEMENT` (0x28)
**ID:** `40**

**Description:** Achievement blocks.

**Behavior:**
- Tracks achievements
- Visual display

**Related Items:**
- Achievement blocks

---

### `type::WEATHER_MACHINE` (0x29)
**ID:** `41**

**Description:** Weather machine blocks.

**Behavior:**
- Changes world weather when toggled
- Punch twice to toggle
- `block->state3 ^= S_TOGGLE`
- Updates `world->现 weather` position
- Sends `OnSetCurrentWeather` packet

**Weather IDs:**
- See `get_weather_id()` in `commands/weather.cpp`
- 80+ weather types

**Related Items:**
- All weather machines (80+ variants)

---

### `type::SCOREBOARD` (0x2a)
**ID:** `42**

**Description:** Scoreboard blocks.

**Behavior:**
- Displays scores
- Game scoring

**Related Items:**
- Scoreboards

---

### `type::SUNGATE` (0x2b)
**ID:** `43**

**Description:** Sungate/stargate blocks.

**Behavior:**
- Portal/teleportation
- Gate system

**Related Items:**
- Sungates

---

### `type::TOGGLEABLE_DEADLY` (0x2d)
**ID:** `45**

**Description:** Toggleable deadly blocks.

**Behavior:**
- Can toggle deadly on/off
- Hazard control

**Related Items:**
- Toggleable hazards

---

### `type::HEART_MONITOR` (0x2e)
**ID:** `46**

**Description:** Heart monitor blocks.

**Behavior:**
- Displays health/heart info
- Medical theme

**Related Items:**
- Heart monitors

---

### `type::DONATION_BOX` (0x2f)
**ID:** `47**

**Description:** Donation box blocks.

**Behavior:**
- Accepts donations
- Tip jar

**Related Items:**
- Donation boxes

---

### `type::MANNEQUIN` (0x31)
**ID:** `49**

**Description:** Mannequin display blocks.

**Behavior:**
- Displays clothing
- Showpiece

**Related Items:**
- Mannequins

---

### `type::CCTV` (0x32)
**ID:** `50**

**Description:** CCTV/security camera blocks.

**Behavior:**
- Security/monitoring
- Visual display

**Related Items:**
- CCTV cameras

---

### `type::MAGIC_EGG` (0x33)
**ID:** `51**

**Description:** Magic egg blocks.

**Behavior:**
- Event item
- Hatches/cracks open

**Related Items:**
- Magic eggs

---

### `type::GAME_BLOCK` (0x34)
**ID:** `52**

**Description:** Game/playable blocks.

**Behavior:**
- Interactive game elements
- Playable blocks

**Related Items:**
- Game blocks

---

### `type::GAME_GENERATOR` (0x35)
**ID:** `53**

**Description:** Game generator blocks.

**Behavior:**
- Generates game elements
- Game creation

**Related Items:**
- Game generators

---

### `type::XENONITE` (0x36)
**ID:** `54**

**Description:** Xenonite crystal blocks.

**Behavior:**
- Special crystal material
- Visual effect

**Related Items:**
- Xenonite blocks

---

### `type::BOOTH` (0x37)
**ID:** `55**

**Description:** Booth/stall blocks.

**Behavior:**
- Vendor/sales booth
- Trading

**Related Items:**
- Booths

---

### `type::CRYSTAL` (0x38)
**ID:** `56**

**Description:** Crystal blocks.

**Behavior:**
- Decorative crystal
- Visual effect

**Related Items:**
- Crystal variants

---

### `type::CRIME_IN_PROGRESS` (0x39)
**ID:** `57**

**Description:** Crime event blocks.

**Behavior:**
- Event trigger
- Crime theme

**Related Items:**
- Crime blocks

---

### `type::GRINDER` (0x3a)
**ID:** `58**

**Description:** Grinder processing blocks.

**Behavior:**
- Processes items
- Grinding mechanic

**Related Items:**
- Grinders

---

### `type::SPOTLIGHT` (0x3b)
**ID:** `59**

**Description:** Spotlight blocks.

**Behavior:**
- Light effect
- Spotlight beam

**Related Items:**
- Spotlights

---

### `type::PUSHING_BLOCK` (0x3c)
**ID:** `60**

**Description:** Pushable/moving blocks.

**Behavior:**
- Can be pushed
- Moving block

**Related Items:**
- Pushing blocks

---

### `type::DISPLAY_BLOCK` (0x3d)
**ID:** `61**

**Description:** Display screen blocks.

**Behavior:**
- Displays items/images
- Stored in `world->displays`
- Shows display dialog when wrench used

**Related Items:**
- Display screens

---

### `type::VENDING_MACHINE` (0x3e)
**ID:** `62**

**Description:** Vending machine blocks.

**Behavior:**
- Sells items automatically
- Stocked with items
- Shows vending dialog when wrench used

**Related Items:**
- Vending machines

---

### `type::FISH_TANK_PORT` (0x3f)
**ID:** `63**

**Description:** Fish tank port blocks.

**Behavior:**
- Fish tank connection
- Can toggle glow

**Related Items:**
- Fish tank ports

---

### `type::FISH` (0x40)
**ID:** `64**

**Description:** Fish items/blocks.

**Behavior:**
- Aquarium fish
- Swimming effect

**Related Items:**
- Fish variants

---

### `type::SOLAR_COLLECTOR` (0x41)
**ID:** `65**

**Description:** Solar panel/collector blocks.

**Behavior:**
- Collects solar energy
- Power generation

**Related Items:**
- Solar collectors

---

### `type::FORGE` (0x42)
**ID:** `66**

**Description:** Forge/smithing blocks.

**Behavior:**
- Smelting/crafting
- Forge processing

**Related Items:**
- Forges

---

### `type::GIVING_TREE` (0x43)
**ID:** `67**

**Description:** Giving tree blocks.

**Behavior:**
- Produces gifts/items
- Tree variant

**Related Items:**
- Giving trees

---

### `type::GIVING_TREE_STUMP` (0x44)
**ID:** `68**

**Description:** Giving tree stump blocks.

**Behavior:**
- Stump variant
- Regrows

**Related Items:**
- Giving tree stumps

---

### `type::STEAM_BLOCK` (0x45)
**ID:** `69**

**Description:** Steam-powered blocks.

**Behavior:**
- Steam mechanic
- Powered by steam

**Related Items:**
- Steam blocks

---

### `type::STEAM_VENT` (0x46)
**ID:** `70**

**Description:** Steam vent blocks.

**Behavior:**
- Releases steam
- Vent effect

**Related Items:**
- Steam vents

---

### `type::STEAM_ORGAN` (0x47)
**ID:** `71**

**Description:** Steam organ/music blocks.

**Behavior:**
- Musical instrument
- Steam-powered music

**Related Items:**
- Steam organs

---

### `type::SILKWORM` (0x48)
**ID:** `72**

**Description:** Silkworm production blocks.

**Behavior:**
- Produces silk
- Farming

**Related Items:**
- Silkworms

---

### `type::SEWING_MACHINE` (0x49)
**ID:** `73**

**Description:** Sewing machine blocks.

**Behavior:**
- Crafting/sewing
- Textile production

**Related Items:**
- Sewing machines

---

### `type::COUNTRY_FLAG` (0x4a)
**ID:** `74**

**Description:** Country flag blocks.

**Behavior:**
- Displays country flag
- National decoration

**Related Items:**
- Country flags

---

### `type::LOBSTER_TRAP` (0x4b)
**ID:** `75**

**Description:** Lobster trap blocks.

**Behavior:**
- Catches lobsters
- Fishing mechanic

**Related Items:**
- Lobster traps

---

### `type::PAINTING_EASEL` (0x4c)
**ID:** `76**

**Description:** Painting easel blocks.

**Behavior:**
- Art creation
- Painting display

**Related Items:**
- Painting easels

---

### `type::BATTLE_PET_CAGE` (0x4d)
**ID:** `77**

**Description:** Battle pet cage blocks.

**Behavior:**
- Houses battle pets
- Pet storage

**Related Items:**
- Battle pet cages

---

### `type::PET_TRAINER` (0x4e)
**ID:** `78**

**Description:** Pet trainer blocks.

**Behavior:**
- Trains pets
- Pet mechanic

**Related Items:**
- Pet trainers

---

### `type::STEAM_ENGINE` (0x4f)
**ID:** `79**

**Description:** Steam engine blocks.

**Behavior:**
- Power generation
- Steam engine

**Related Items:**
- Steam engines

---

### `type::LOCK_BOT` (0x50)
**ID:** `80**

**Description:** Lock bot automation blocks.

**Behavior:**
- Automated locking
- Bot system

**Related Items:**
- Lock bots

---

### `type::SFX_WEATHER_MACHINE` (0x51)
**ID:** `81**

**Description:** Special effects weather machines.

**Behavior:**
- Weather + sound effects
- Special weather

**Related Items:**
- Heatwave weather machine
- Balloon Warz weather machine
- Background weather machine
- Valentine's weather machine
- St. Paddy's weather machine
- Digital Rain weather machine

---

### `type::SPIRIT_STORAGE` (0x52)
**ID:** `82**

**Description:** Spirit storage blocks.

**Behavior:**
- Stores spirits
- Storage system

**Related Items:**
- Spirit storage

---

### `type::DISPLAY_SHELF` (0x53)
**ID:** `83**

**Description:** Display shelf blocks.

**Behavior:**
- Displays items
- Shelving

**Related Items:**
- Display shelves

---

### `type::VIP_ENTRANCE` (0x54)
**ID:** `84**

**Description:** VIP entrance blocks.

**Behavior:**
- VIP-only access
- Special entrance

**Related Items:**
- VIP entrances

---

### `type::CHALLENGE_TIMER` (0x55)
**ID:** `85**

**Description:** Challenge timer blocks.

**Behavior:**
- Times challenges
- Countdown display

**Related Items:**
- Challenge timers

---

### `type::CHALLENGE_FLAG` (0x56)
**ID:** `86**

**Description:** Challenge flag blocks.

**Behavior:**
- Marks challenge start/end
- Flag system

**Related Items:**
- Challenge flags

---

### `type::FISH_MOUNT` (0x57)
**ID:** `87**

**Description:** Fish mount/trophy blocks.

**Behavior:**
- Displays caught fish
- Trophy display

**Related Items:**
- Fish mounts

---

### `type::PORTRAIT` (0x58)
**ID:** `88**

**Description:** Portrait blocks.

**Behavior:**
- Displays portrait
- Picture frame

**Related Items:**
- Portraits

---

### `type::SPRITE_WEATHER_MACHINE` (0x59)
**ID:** `89**

**Description:** Sprite-based weather machines.

**Behavior:**
- Sprite animation weather
- Special effects

**Related Items:**
- Stuff weather machine
- Guild weather machine
- Epoch weather machine

---

### `type::FOSSIL` (0x5a)
**ID:** `90**

**Description:** Fossil blocks.

**Behavior:**
- Excavation item
- Fossil display

**Related Items:**
- Fossils

---

### `type::FOSSIL_PREP_STATION` (0x5b)
**ID:** `91**

**Description:** Fossil preparation station blocks.

**Behavior:**
- Prepares fossils
- Processing

**Related Items:**
- Fossil prep stations

---

### `type::DNA_PROCESSOR` (0x5c)
**ID:** `92**

**Description:** DNA processor blocks.

**Behavior:**
- Processes DNA
- Genetic system

**Related Items:**
- DNA processors

---

### `type::TRICKSTER` (0x5d)
**ID:** `93**

**Description:** Trickster event blocks.

**Behavior:**
- Event item
- Trick mechanic

**Related Items:**
- Trickster blocks

---

### `type::VALHOWLA_TREASURE` (0x5e)
**ID:** `94**

**Description:** Valhalla treasure blocks.

**Behavior:**
- Treasure chest variant
- Event item

**Related Items:**
- Valhalla treasures

---

### `type::CHEMSYNTH_PROCESSOR` (0x5f)
**ID:** `95**

**Description:** Chemsynth processor blocks.

**Behavior:**
- Chemical synthesis
- Processing

**Related Items:**
- Chemsynth processors

---

### `type::CHEMSYNTH_TANK` (0x60)
**ID:** `96**

**Description:** Chemsynth tank blocks.

**Behavior:**
- Chemical storage
- Tank system

**Related Items:**
- Chemsynth tanks

---

### `type::STORAGE_BOX` (0x61)
**ID:** `97**

**Description:** Storage box blocks.

**Behavior:**
- Item storage
- Box system

**Related Items:**
- Storage boxes

---

### `type::COOKING_OVEN` (0x62)
**ID:** `98**

**Description:** Cooking oven blocks.

**Behavior:**
- Cooking system
- Food preparation

**Related Items:**
- Cooking ovens

---

### `type::AUDIO_BLOCK` (0x63)
**ID:** `99**

**Description:** Audio/sound blocks.

**Behavior:**
- Plays audio
- Sound system

**Related Items:**
- Audio blocks

---

### `type::GEIGER_CHARGER` (0x64)
**ID:** `100**

**Description:** Geiger charger blocks.

**Behavior:**
- Radiation charging
- Power system

**Related Items:**
- Geiger chargers

---

### `type::THE_ADVENTURE_BEGINS` (0x65)
**ID:** `101**

**Description:** Adventure start blocks.

**Behavior:**
- Quest/Adventure trigger
- Story element

**Related Items:**
- Adventure blocks

---

### `type::TOMB_ROBBER` (0x66)
**ID:** `102**

**Description:** Tomb robber event blocks.

**Behavior:**
- Event trigger
- Tomb theme

**Related Items:**
- Tomb robber blocks

---

### `type::BALLOON_O_MATIC` (0x67)
**ID:** `103**

**Description:** Balloon-o-matic blocks.

**Behavior:**
- Balloon creation
- Event item

**Related Items:**
- Balloon-o-matic

---

### `type::TEAM_ENTRANCE_PUNCH` (0x68)
**ID:** `104**

**Description:** Team entrance (Punch) blocks.

**Behavior:**
- Team-specific entrance
- Punch team

**Related Items:**
- Punch team entrances

---

### `type::TEAM_ENTRANCE_GROW` (0x69)
**ID:** `105**

**Description:** Team entrance (Grow) blocks.

**Behavior:**
- Team-specific entrance
- Grow team

**Related Items:**
- Grow team entrances

---

### `type::TEAM_ENTRANCE_BUILD` (0x6a)
**ID:** `106**

**Description:** Team entrance (Build) blocks.

**Behavior:**
- Team-specific entrance
- Build team

**Related Items:**
- Build team entrances

---

### `type::AURA` (0x6b)
**ID:** `107**

**Description:** Aura effect blocks.

**Behavior:**
- Aura visual effect
- Includes ances (aura)
- Equipped in `clothing[ances]` slot

**Related Items:**
- Aura items
- Ances

---

### `type::LEMON_JELLY_BLOCK` (0x6c)
**ID:** `108**

**Description:** Lemon jelly blocks.

**Behavior:**
- Jelly/wobbly effect
- Food theme

**Related Items:**
- Lemon jelly blocks

---

### `type::TRAINING_PORT` (0x6d)
**ID:** `109**

**Description:** Training portal blocks.

**Behavior:**
- Training area entrance
- Practice zone

**Related Items:**
- Training ports

---

### `type::FISHING_BLOCK` (0x6e)
**ID:** `110**

**Description:** Fishing blocks.

**Behavior:**
- Fishing mechanic
- Catch fish

**Related Items:**
- Fishing blocks

---

### `type::MAGPLANT` (0x6f)
**ID:** `111**

**Description:** Magplant device blocks.

**Behavior:**
- Magnetic planting
- Automation

**Related Items:**
- Magplants

---

### `type::MAGPLANT_REMOTE` (0x70)
**ID:** `112**

**Description:** Magplant remote blocks.

**Behavior:**
- Remote control
- Magplant control

**Related Items:**
- Magplant remotes

---

### `type::CYBLOCK_BOT` (0x71)
**ID:** `113**

**Description:** Cyblock bot blocks.

**Behavior:**
- Bot automation
- Cyber block

**Related Items:**
- Cyblock bots

---

### `type::CYBLOCK_COMMAND` (0x72)
**ID:** `114**

**Description:** Cyblock command blocks.

**Behavior:**
- Command system
- Programming

**Related Items:**
- Cyblock commands

---

### `type::LUCKY_TOKEN` (0x73)
**ID:** `115**

**Description:** Lucky token blocks.

**Behavior:**
- Token/fortune item
- Event item

**Related Items:**
- Lucky tokens

---

### `type::GROWSCAN` (0x74)
**ID:** `116**

**Description:** Growscan device blocks.

**Behavior:**
- Scanning device
- Analysis tool

**Related Items:**
- Growscan devices

---

### `type::CONTAINMENT_FIELD_POWER_NODE` (0x75)
**ID:** `117**

**Description:** Containment field power node blocks.

**Behavior:**
- Power system
- Containment field

**Related Items:**
- Power nodes

---

### `type::SPIRIT_BOARD` (0x76)
**ID:** `118**

**Description:** Spirit board blocks.

**Behavior:**
- Spirit communication
- Ouija board

**Related Items:**
- Spirit boards

---

### `type::WORLD_ARCHITECT` (0x77)
**ID:** `119**

**Description:** World architect blocks.

**Behavior:**
- World editing tool
- Architecture system

**Related Items:**
- World architect tools

---

### `type::STARTOPIA_BLOCK` (0x78)
**ID:** `120**

**Description:** Startopia blocks.

**Behavior:**
- Space station theme
- Crossover item

**Related Items:**
- Startopia blocks

---

### `type::TOGGLEABLE_MULTI_FRAME_BLOCK` (0x7a)
**ID:** `122**

**Description:** Toggleable multi-frame blocks.

**Behavior:**
- Multiple animation frames
- Toggleable

**Related Items:**
- Multi-frame blocks

---

### `type::AUTO_BREAK_BLOCK` (0x7b)
**ID:** `123**

**Description:** Auto-break blocks.

**Behavior:**
- Automatically breaks
- Self-destruct

**Related Items:**
- Auto-break blocks

---

### `type::AUTO_BREAK_TREE` (0x7c)
**ID:** `124**

**Description:** Auto-break tree blocks.

**Behavior:**
- Automatically harvests
- Self-harvesting tree

**Related Items:**
- Auto-break trees

---

### `type::AUTO_BREAK` (0x7d)
**ID:** `125**

**Description:** Auto-break generic blocks.

**Behavior:**
- Automatic breaking
- Self-destruct

**Related Items:**
- Auto-break items

---

### `type::STORM_CLOUD` (0x7e)
**ID:** `126**

**Description:** Storm cloud blocks.

**Behavior:**
- Weather effect
- Storm cloud

**Related Items:**
- Storm clouds

---

### `type::DISAPPEAR_WHEN_STEPPED_ON` (0x7f)
**ID:** `127**

**Description:** Disappearing blocks.

**Behavior:**
- Vanishes when stepped on
- Trap mechanic

**Related Items:**
- Disappearing blocks

---

### `type::PUDDLE_BLOCK` (0x80)
**ID:** `128**

**Description:** Puddle blocks.

**Behavior:**
- Water puddle effect
- Liquid surface

**Related Items:**
- Puddle blocks

---

### `type::ROOT_CUTTING` (0x81)
**ID:** `129**

**Description:** Root cutting blocks.

**Behavior:**
- Plant propagation
- Root system

**Related Items:**
- Root cuttings

---

### `type::SAFE_VAULT` (0x82)
**ID:** `130**

**Description:** Safe vault blocks.

**Behavior:**
- Secure storage
- Vault system

**Related Items:**
- Safe vaults

---

### `type::ANGELIC_COUNTING_CLOUD` (0x83)
**ID:** `131**

**Description:** Angelic counting cloud blocks.

**Behavior:**
- Counting/display
- Angel theme

**Related Items:**
- Angelic clouds

---

### `type::MINING_EXPLOSIVE` (0x84)
**ID:** `132**

**Description:** Mining explosive blocks.

**Behavior:**
- Explosive mining
- Demolition

**Related Items:**
- Mining explosives

---

### `type::INFINITY_WEATHER_MACHINE` (0x86)
**ID:** `134**

**Description:** Infinity weather machine blocks.

**Behavior:**
- Special weather effect
- Infinity theme

**Related Items:**
- Infinity weather machines

---

### `type::GHOST_BLOCK` (0x87)
**ID:** `135**

**Description:** Ghost/phantom blocks.

**Behavior:**
- Invisible/ghostly
- Phase through

**Related Items:**
- Ghost blocks

---

### `type::ACID` (0x88)
**ID:** `136**

**Description:** Acid hazard blocks.

**Behavior:**
- Damages players
- Corrosive effect

**Related Items:**
- Acid pools

---

### `type::WAVING_INFLATABLE_ARM_GUY` (0x8a)
**ID:** `138**

**Description:** Waving inflatable arm guy blocks.

**Behavior:**
- Waving animation
- Advertisement

**Related Items:**
- Inflatable arm guys

---

### `type::PINEAPPLE_GUZZLER` (0x8c)
**ID:** `140**

**Description:** Pineapple guzzler blocks.

**Behavior:**
- Consumes pineapples
- Processing

**Related Items:**
- Pineapple guzzlers

---

### `type::KRANKEN_GALACTIC` (0x8d)
**ID:** `141**

**Description:** Krankengalactic blocks.

**Behavior:**
- Event item
- Space theme

**Related Items:**
- Krankengalactic items

---

### `type::FRIEND_ENTRANCE` (0x8e)
**ID:** `142**

**Description:** Friend entrance blocks.

**Behavior:**
- Friend-only access
- Social entrance

**Related Items:**
- Friend entrances

---

## Category Flags (`enum cat`)

Item categories that modify behavior:

### `CAT_RETURN` (0x02)
**Description:** Item returns to inventory when broken.

**Behavior:**
- Doesn't get destroyed
- Returns to backpack if space available
- Calls `item_change_object()` then `item_activate_object()`

### `CAT_SUPRISING_FRUIT` (0x08)
**Description:** Tree can bear surprising fruit.

**Behavior:**
- Special fruit drops
- Random rewards

### `CAT_PUBLIC` (0x10)
**Description:** Item is public access.

**Behavior:**
- Anyone can use/break
- Bypasses lock checks

### `CAT_HOLIDAY` (0x40)
**Description:** Holiday-only item.

**Behavior:**
- Can only be created during holidays
- Seasonal item

### `CAT_UNTRADEABLE` (0x80)
**Description:** Item cannot be traded or dropped.

**Behavior:**
- Cannot be dropped
- Cannot be traded
- Error: "You can't drop that."

---

## Clothing Types (`enum clothing`)

Clothing equipment slots:

| Value | Slot | Description |
|-------|------|-------------|
| 0 | `hair` | Hair/hat items |
| 1 | `shirt` | Shirt/chest items |
| 2 | `legs` | Pants/leg items |
| 3 | `feet` | Shoes/feet items |
| 4 | `face` | Face items |
| 5 | `hand` | Hand-held items |
| 6 | `back` | Back items |
| 7 | `head` | Head items |
| 8 | `charm` | Charm/pet items |
| 9 | `ances` | Aura items |
| 10 | `none` | No clothing slot |

---

## Quick Reference

### Most Common Types

| Type | ID | Usage |
|------|-----|-------|
| `LOCK` | 3 | World locks |
| `SEED` | 19 | Trees/farming |
| `CONSUMEABLE` | 8 | Usable items |
| `CLOTHING` | 20 | Wearables |
| `PROVIDER` | 38 | Resource generation |
| `WEATHER_MACHINE` | 41 | Weather control |
| `TOGGLEABLE_BLOCK` | 31 | On/off blocks |
| `CHEST` | 32 | Storage |

---

*Documentation generated from `include/database/items.hpp` - 2026*
