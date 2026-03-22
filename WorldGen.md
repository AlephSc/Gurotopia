# World Generation System - Deep Dive

## `generate_world()` Function

**File:** `include/database/world.cpp`

**Signature:**
```cpp
void generate_world(world &world, const std::string& name)
```

**Purpose:**
Generates default terrain for a new world with caves, lava, bedrock, and a main door.

**World Dimensions:**
- Width: 100 tiles (X: 0-99)
- Height: 60 tiles (Y: 0-59)
- Total blocks: 6000 (100 × 60)

---

## Complete Code Analysis

```cpp
void generate_world(world &world, const std::string& name)
{
    // Step 1: Initialize random number generator
    ransuu ransuu;
    
    // Step 2: Calculate random main door X position
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    
    // Step 3: Create empty world (6000 blocks)
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    // Step 4: Generate terrain
    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        ::block &block = blocks[i];
        
        // Underground layer (Y >= 37)
        if (i >= cord(0, 37))
        {
            block.bg = 14; // Cave background
            
            // Rock layer (Y: 38-49)
            if (i >= cord(0, 38) && i < cord(0, 50) && ransuu[{0, 38}] <= 1)
                block.fg = 10; // Rock
            
            // Lava layer (Y: 51-53)
            else if (i > cord(0, 50) && i < cord(0, 54) && ransuu[{0, 8}] < 3)
                block.fg = 4; // Lava
            
            // Bedrock (Y >= 54) or Dirt (Y: 37-53)
            else
                block.fg = (i >= cord(0, 54)) ? 8 : 2;
        }
        
        // Main door placement
        if (i == cord(main_door, 36))
        {
            block.fg = 6;      // Main door
            block.label = "EXIT";
        }
        else if (i == cord(main_door, 37))
        {
            block.fg = 8;  // Bedrock below door
        }
    }
    
    // Step 5: Assign to world
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}
```

---

## Step-by-Step Breakdown

### Step 1: Random Number Generator

```cpp
ransuu ransuu;
```

**What it does:**
- Creates a `ransuu` object (random number generator)
- Uses Mersenne Twister engine (`std::mt19937`)
- Seeded with `std::random_device{}()` (true random seed)

**Ransuu Class:**
```cpp
class ransuu
{
    std::mt19937 engine;  // Mersenne Twister RNG

public:
    ransuu() : engine(std::random_device{}()) {}
    
    // Get random int in range [min, max]
    int operator[](Range _range);
    
    // Get random float: [min * right, max * right]
    float shosu(Range _range, float right = 0.1f);
};
```

**Usage:**
```cpp
ransuu rng;
int value = rng[{min, max}];  // Random int between min and max (inclusive)
```

---

### Step 2: Main Door Position

```cpp
u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
```

**Calculation:**
```
100 * 60 / 100 - 4
= 6000 / 100 - 4
= 60 - 4
= 56

Range: [2, 56]
```

**Why this range?**
- **Minimum (2):** Door can't be at edge (X=0 or X=1)
- **Maximum (56):** Door can't be too close to right edge (X=97, 98, 99)
- **Valid positions:** X coordinates 2 through 56 (55 possible positions)

**Result:**
- Main door will be placed at random X position between 2 and 56
- Y position is always 36 (surface level)

---

### Step 3: Create Empty World

```cpp
std::vector<block> blocks(100 * 60, block{0, 0});
```

**What it does:**
- Creates vector with 6000 blocks (100 width × 60 height)
- Each block initialized with `block{0, 0}` (empty fg and bg)

**Block Structure:**
```cpp
struct block {
    short fg{0};  // Foreground item ID
    short bg{0};  // Background item ID
    std::chrono::steady_clock::time_point tick{};
    std::string label{};
    u_char state3{};  // Direction, toggle, public
    u_char state4{};  // Water, fire, paint
    std::array<int, 2zu> hits{0, 0};
};
```

---

### Step 4: Terrain Generation Loop

```cpp
for (std::size_t i = 0zu; i < blocks.size(); ++i)
{
    ::block &block = blocks[i];
    // ... terrain generation ...
}
```

**Loop Details:**
- Iterates through all 6000 blocks
- `i` is linear index (0 to 5999)
- Convert to 2D coordinates:
  - `X = i % 100`
  - `Y = i / 100`

**`cord()` Macro:**
```cpp
#define cord(x,y) (y * 100 + x)
```

**Examples:**
- `cord(0, 37)` = 37 * 100 + 0 = 3700
- `cord(50, 54)` = 54 * 100 + 50 = 5450
- `cord(main_door, 36)` = 36 * 100 + main_door

---

## Terrain Layers

### Layer 1: Sky (Y: 0-36)

**Code:**
```cpp
// No code - blocks remain empty (fg=0, bg=0)
```

**Result:**
- Air/empty space
- Where players build

---

### Layer 2: Cave Background Start (Y: 37)

**Code:**
```cpp
if (i >= cord(0, 37))
{
    block.bg = 14; // Cave background
    // ...
}
```

**Block ID 14:**
- Cave background tile
- Visual only (background layer)

**Result:**
- Starting at Y=37, all blocks get cave background
- Continues to bottom of world

---

### Layer 3: Rock Layer (Y: 38-49)

**Code:**
```cpp
if (i >= cord(0, 38) && i < cord(0, 50) /* (above) lava level */ 
    && ransuu[{0, 38}] <= 1)
    block.fg = 10; // Rock
```

**Conditions:**
1. `i >= cord(0, 38)` → Y >= 38
2. `i < cord(0, 50)` → Y < 50
3. `ransuu[{0, 38}] <= 1` → Random chance

**Random Chance Calculation:**
```cpp
ransuu[{0, 38}]  // Returns random int from 0 to 38 (inclusive)
// Total possibilities: 39 values (0, 1, 2, ..., 38)
// Success values: 0, 1 (2 values)
// Probability: 2/39 ≈ 5.13%
```

**Block ID 10:**
- Rock tile
- Can be broken
- Drops gems/seeds

**Result:**
- Random rock formations in Y: 38-49 layer
- ~5% of blocks become rock
- Creates natural-looking cave ceiling

---

### Layer 4: Lava Layer (Y: 51-53)

**Code:**
```cpp
else if (i > cord(0, 50) && i < cord(0, 54) /* (above) bedrock level */ 
         && ransuu[{0, 8}] < 3)
    block.fg = 4; // Lava
```

**Conditions:**
1. `i > cord(0, 50)` → Y > 50 (Y: 51-53)
2. `i < cord(0, 54)` → Y < 54
3. `ransuu[{0, 8}] < 3` → Random chance

**Random Chance Calculation:**
```cpp
ransuu[{0, 8}]  // Returns random int from 0 to 8 (inclusive)
// Total possibilities: 9 values (0, 1, 2, ..., 8)
// Success values: 0, 1, 2 (3 values)
// Probability: 3/9 = 1/3 ≈ 33.33%
```

**Block ID 4:**
- Lava tile
- Damages players (FIRE_PAIN type)
- Can be extinguished with water

**Result:**
- Random lava pockets in Y: 51-53 layer
- ~33% of blocks become lava
- Creates dangerous underground layer

---

### Layer 5: Dirt/Bedrock (Y: 37-59)

**Code:**
```cpp
else
    block.fg = (i >= cord(0, 54)) ? 8 : 2;
```

**Logic:**
- If Y >= 54 → Block ID 8 (Bedrock)
- Else (Y: 37-53) → Block ID 2 (Dirt)

**Block ID 8:**
- Bedrock (unbreakable)
- `type::STRONG`
- Message: "It's too strong to break."

**Block ID 2:**
- Dirt tile
- Basic building block
- Can be broken

**Result:**
- Y: 37-53: Dirt (if not rock/lava)
- Y: 54-59: Bedrock (6 rows of unbreakable blocks)

---

### Layer 6: Main Door Placement

**Code:**
```cpp
if (i == cord(main_door, 36))
{
    block.fg = 6;
    block.label = "EXIT";
}
else if (i == cord(main_door, 37))
{
    block.fg = 8;
}
```

**Main Door (ID 6):**
- Placed at Y=36 (surface level)
- Random X position (2-56)
- Label: "EXIT"
- `type::MAIN_DOOR`
- Sets player's `rest_pos` (respawn point)

**Bedrock Below Door (ID 8):**
- Placed at Y=37 (directly below door)
- Prevents breaking under door
- Unbreakable support

**Visual Layout:**
```
Y=36: [ ][ ][Door][ ][ ]  ← Main door with "EXIT" label
Y=37: [ ][ ][Bedrock][ ][ ]  ← Unbreakable support
Y=38: [ ][ ][Dirt/Rock][ ][ ]  ← Underground starts
```

---

## Terrain Visualization

**Cross-section of generated world:**

```
Y=0   ████████████████████  ← Sky (empty)
Y=10  ████████████████████
Y=20  ████████████████████
Y=30  ████████████████████
Y=36  ██████[DOOR]████████  ← Main door at random X
Y=37  ██████[BEDR]████████  ← Bedrock below door, cave bg starts
Y=38  ██[Rock]██[Rock]████  ← Cave bg + random rocks (5% chance)
Y=40  ████[Rock]████[Rock]█
Y=45  ████████████████████  ← Mostly dirt
Y=50  ████████████████████
Y=51  ██[Lava]██[Lava]████  ← Lava layer starts (33% chance)
Y=52  ████[Lava]████[Lava]█
Y=53  ██[Lava]████████[Lava]
Y=54  ████████████████████  ← Bedrock starts (unbreakable)
Y=59  ████████████████████  ← Bottom of world
      X=0                X=99
```

---

## `ransuu` Function Deep Dive

### `operator[]` - Random Integer

**Implementation:**
```cpp
int ransuu::operator[](Range _range) {
    std::uniform_int_distribution<int> dist(_range.min, _range.max);
    return dist(engine);
}
```

**How it works:**
1. Creates `uniform_int_distribution` with min/max
2. Generates random number using Mersenne Twister engine
3. Returns value in range [min, max] (inclusive)

**Examples:**
```cpp
ransuu rng;

rng[{0, 10}]    // Random int: 0, 1, 2, ..., 10 (11 possibilities)
rng[{2, 56}]    // Random int: 2, 3, 4, ..., 56 (55 possibilities)
rng[{0, 38}]    // Random int: 0, 1, 2, ..., 38 (39 possibilities)
rng[{0, 8}]     // Random int: 0, 1, 2, ..., 8 (9 possibilities)
```

**Probability Formula:**
```
P(value = x) = 1 / (max - min + 1)

Example: rng[{0, 38}]
P(value = 0) = 1 / 39 ≈ 2.56%
P(value <= 1) = 2 / 39 ≈ 5.13%
```

---

### `shosu()` - Random Float

**Implementation:**
```cpp
float ransuu::shosu(Range _range, float right = 0.1f) {
    return static_cast<float>((*this)[_range]) * right;
}
```

**How it works:**
1. Calls `operator[]` to get random int
2. Converts to float
3. Multiplies by `right` parameter

**Examples:**
```cpp
ransuu rng;

rng.shosu({7, 50}, 0.01f)  
// Returns: 0.07 to 0.50 (step 0.01)
// Used for: Drop position offsets

rng.shosu({1, 100}, 0.1f)
// Returns: 0.1 to 10.0 (step 0.1)
```

**Usage in Codebase:**
```cpp
// From add_drop():
pos.f_x() + rng.shosu({7, 50}, 0.01f)  // Offset: 0.07 to 0.50 tiles
pos.f_y() + rng.shosu({7, 50}, 0.01f)
```

---

## `blast::thermonuclear()` Function

**File:** `include/database/world.cpp`

**Signature:**
```cpp
void blast::thermonuclear(world &world, const std::string& name)
```

**Purpose:**
Generates a completely flat world (used by Thermonuclear Blast item).

**Code:**
```cpp
void blast::thermonuclear(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});

    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        // Only bedrock at Y >= 54, everything else is air
        blocks[i].fg = (i >= cord(0, 54)) ? 8 : 0;

        // Main door
        if (i == cord(main_door, 36))
            blocks[i].fg = 6;
        else if (i == cord(main_door, 37))
            blocks[i].fg = 8;
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}
```

**Differences from `generate_world()`:**

| Aspect | `generate_world()` | `blast::thermonuclear()` |
|--------|-------------------|-------------------------|
| **Cave Background** | Yes (Y >= 37) | No |
| **Rock Layer** | Yes (5% chance) | No |
| **Lava Layer** | Yes (33% chance) | No |
| **Dirt** | Yes (Y: 37-53) | No |
| **Bedrock** | Yes (Y >= 54) | Yes (Y >= 54) |
| **Main Door** | Yes | Yes |
| **Result** | Natural terrain | Completely flat |

**Visual Comparison:**

```
generate_world():          blast::thermonuclear():
Y=0   ████████████         Y=0   ████████████
Y=36  █████[DOOR]████      Y=36  █████[DOOR]████
Y=37  █████[BEDR]████      Y=37  █████[BEDR]████
Y=38  ██[Rock]██[Rock]██   Y=38  ██████████████  ← Empty!
Y=50  ████████████████     Y=50  ██████████████
Y=54  ████████████████     Y=54  ██████████████
Y=59  ████████████████     Y=59  ██████████████
```

---

## `door_mover()` Function

**File:** `include/database/world.cpp`

**Signature:**
```cpp
bool door_mover(world &world, const ::pos &pos)
```

**Purpose:**
Moves the main door to a new position (used by Door Mover item ID: 1404).

**Code:**
```cpp
bool door_mover(world &world, const ::pos &pos)
{
    std::vector<block> &blocks = world.blocks;

    // Check if target position is valid (needs 2 vertical empty spaces)
    if (blocks[cord(pos.x, pos.y)].fg != 0 ||
        blocks[cord(pos.x, (pos.y + 1))].fg != 0)
        return false;

    // Find and remove old door
    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        if (blocks[i].fg == 6)
        {
            blocks[i].fg = 0;  // Remove door
            blocks[cord(i % 100, (i / 100 + 1))].fg = 0;  // Remove bedrock below
            break;
        }
    }
    
    // Place door at new position
    blocks[cord(pos.x, pos.y)].fg = 6;
    blocks[cord(pos.x, (pos.y + 1))].fg = 8;
    return true;
}
```

**Process:**
1. **Validate Target:**
   - Check if `pos.y` is empty (fg == 0)
   - Check if `pos.y + 1` is empty (fg == 0)
   - Returns `false` if either is occupied

2. **Remove Old Door:**
   - Search all blocks for door (ID 6)
   - Remove door (set fg = 0)
   - Remove bedrock below (set fg = 0)

3. **Place New Door:**
   - Place door at `pos` (ID 6)
   - Place bedrock below (ID 8)
   - Returns `true` on success

**Requirements:**
- Target must have 2 vertical empty spaces
- Door placed at Y coordinate
- Bedrock placed at Y+1 coordinate

**Example:**
```cpp
// Move door to position (50, 30)
::pos newPos{50, 30};
if (door_mover(world, newPos))
{
    // Success! Door moved to (50, 30)
    // Old door position is now empty
}
else
{
    // Failed! Position not empty
}
```

---

## Block IDs Reference

| ID | Name | Type | Layer | Breakable |
|----|------|------|-------|-----------|
| 0 | Air | - | All | N/A |
| 2 | Dirt | FOREGROUND | Y: 37-53 | Yes |
| 4 | Lava | FIRE_PAIN | Y: 51-53 | Yes |
| 6 | Main Door | MAIN_DOOR | Y: 36 | No |
| 8 | Bedrock | STRONG | Y: 37, 54-59 | No |
| 10 | Rock | FOREGROUND | Y: 38-49 | Yes |
| 14 | Cave Background | BACKGROUND | Y: 37-59 | N/A |

---

## Probability Summary

| Layer | Y Range | Block ID | Probability | Description |
|-------|---------|----------|-------------|-------------|
| Rock | 38-49 | 10 | 2/39 ≈ 5.13% | Random rock formations |
| Lava | 51-53 | 4 | 3/9 ≈ 33.33% | Random lava pockets |
| Dirt | 37-53 | 2 | Remainder | Base underground material |
| Bedrock | 54-59 | 8 | 100% | Unbreakable bottom layer |

---

## Coordinate System

**Linear to 2D Conversion:**
```cpp
#define cord(x,y) (y * 100 + x)

// Examples:
cord(0, 0)    = 0 * 100 + 0     = 0      (top-left)
cord(99, 0)   = 0 * 100 + 99    = 99     (top-right)
cord(0, 59)   = 59 * 100 + 0    = 5900   (bottom-left)
cord(99, 59)  = 59 * 100 + 99   = 5999   (bottom-right)
```

**2D to Linear Conversion:**
```cpp
X = i % 100
Y = i / 100

// Examples:
i = 0      → X=0,   Y=0
i = 99     → X=99,  Y=0
i = 100    → X=0,   Y=1
i = 5999   → X=99,  Y=59
```

---

## Performance Notes

**Time Complexity:**
- `generate_world()`: O(n) where n = 6000 blocks
- `blast::thermonuclear()`: O(n) where n = 6000 blocks
- `door_mover()`: O(n) where n = 6000 blocks (searches for door)

**Memory:**
- Creates temporary `std::vector<block>` (6000 blocks)
- Moved to world with `std::move()` (no copy)
- Each block: ~40 bytes
- Total: ~240 KB per world

**Optimization Opportunities:**
1. `door_mover()` could cache door position instead of searching
2. Terrain generation could use noise functions for more natural look
3. Rock/lava probabilities are hardcoded

---

## Customization Guide

### Changing Terrain Layers

**Example: Add diamond ore layer:**
```cpp
void generate_world(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 56}];
    std::vector<block> blocks(6000, block{0, 0});

    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        ::block &block = blocks[i];
        
        if (i >= cord(0, 37))
        {
            block.bg = 14;
            
            // Diamond ore layer (Y: 45-49)
            if (i >= cord(0, 45) && i < cord(0, 50) && ransuu[{0, 100}] < 5)
                block.fg = 999;  // Diamond ore (example ID)
            
            // ... rest of terrain
        }
        // ... door placement
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}
```

### Changing Probabilities

**Make rocks more common:**
```cpp
// Original: 2/39 ≈ 5.13%
if (ransuu[{0, 38}] <= 1) block.fg = 10;

// New: 10/39 ≈ 25.64%
if (ransuu[{0, 38}] <= 9) block.fg = 10;
```

**Make lava more common:**
```cpp
// Original: 3/9 ≈ 33.33%
if (ransuu[{0, 8}] < 3) block.fg = 4;

// New: 6/9 ≈ 66.67%
if (ransuu[{0, 8}] < 6) block.fg = 4;
```

### Changing Main Door Range

**Wider door placement range:**
```cpp
// Original: [2, 56] (55 positions)
u_short main_door = ransuu[{2, 56}];

// Wider: [1, 98] (98 positions)
u_short main_door = ransuu[{1, 98}];
```

---

*Documentation generated from `include/database/world.cpp` - 2026*
