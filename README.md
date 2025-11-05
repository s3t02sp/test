# GameSense-Style Hit Log System for CS2

A professional hit log system inspired by GameSense's design, featuring smooth animations, color-coded messages, and clean visual presentation.

## Features

- ✨ **Smooth Fade Effects**: Messages smoothly fade in and out
- 🎨 **Color-Coded Messages**: 
  - 🟢 Green for hits
  - 🔴 Red for misses  
  - 🟡 Gold for kills
  - Custom colors supported
- 👁️ **Clean Visual Design**: Shadow text effect for better readability
- ⚡ **Performance Optimized**: Uses deque for efficient log management
- 🔄 **Auto-Cleanup**: Automatically clears expired logs and cleans up when not in game
- ⚙️ **Highly Configurable**: Customize position, max logs, colors, and durations

## Files

- `hit_log.h` - Header file with structure definitions and class declaration
- `hit_log.cpp` - Implementation of the hit log system
- `example_usage.cpp` - Comprehensive usage examples

## Quick Start

### 1. Include the header
```cpp
#include "hit_log.h"
```

### 2. Call draw function every frame
```cpp
void c_menu::draw_logs()
{
    g_hit_log.draw_logs();
}
```

### 3. Add logs when events occur

**Hit Event:**
```cpp
g_hit_log.add_hit("enemy", 95, 5, "head");
// Output: "hit enemy in head for 95 (5 remaining)"
```

**Miss Event:**
```cpp
g_hit_log.add_miss("enemy", "spread");
// Output: "missed enemy due to spread"
```

**Kill Event:**
```cpp
g_hit_log.add_kill("enemy", 100, "chest");
// Output: "killed enemy in chest for 100 damage"
```

**Custom Log:**
```cpp
g_hit_log.add_log("Custom message", 5.0f, 1.0f, 255, 255, 255);
```

## API Reference

### `add_hit(targetName, damage, health, hitbox)`
Adds a hit log entry with green color.
- `targetName`: Name of the player you hit
- `damage`: Damage dealt
- `health`: Remaining health of the target
- `hitbox`: Hitbox name (e.g., "head", "chest")

### `add_miss(targetName, reason)`
Adds a miss log entry with red color.
- `targetName`: Name of the player you missed
- `reason`: Reason for the miss (e.g., "spread", "resolver")

### `add_kill(targetName, damage, hitbox)`
Adds a kill log entry with gold color.
- `targetName`: Name of the player you killed
- `damage`: Final damage dealt
- `hitbox`: Hitbox name

### `add_log(message, displayTime, fadeDuration, r, g, b)`
Adds a custom log entry.
- `message`: The message to display
- `displayTime`: How long to display before fading (default: 5.0s)
- `fadeDuration`: How long the fade animation takes (default: 1.0s)
- `r, g, b`: RGB color values (default: white)

### `set_position(x, y)`
Sets the screen position for logs.
- `x`: X coordinate (default: 5.0)
- `y`: Y coordinate (default: 5.0)

### `set_max_logs(max)`
Sets maximum number of visible logs.
- `max`: Maximum logs to display (default: 10)

### `clear_logs()`
Clears all log entries immediately.

## Configuration Example

```cpp
void setup_hit_log()
{
    // Position logs in top-left corner
    g_hit_log.set_position(10.0f, 10.0f);
    
    // Show maximum 8 logs at once
    g_hit_log.set_max_logs(8);
}
```

## Hitbox Names

Common hitbox names you can use:
- `"head"`
- `"neck"`
- `"chest"`
- `"stomach"`
- `"left arm"` / `"right arm"`
- `"left leg"` / `"right leg"`

## Miss Reasons

Common miss reasons:
- `"spread"`
- `"resolver"`
- `"prediction error"`
- `"bad angle"`
- `"occlusion"`
- `"lag compensation"`

## Technical Details

### Data Structure
```cpp
struct hit_log_entry
{
    std::string message;
    float startTime;
    float fadeStartTime;
    float fadeDuration;
    int r, g, b; // RGB color
};
```

### Animation System
- Logs are added to the front of a deque
- Each log has a start time, fade start time, and fade duration
- Alpha value is calculated based on current game time
- Expired logs are automatically removed during rendering

### Performance
- Uses `std::deque` for O(1) insertion at front and removal from back
- Automatically limits to max number of logs
- Only renders when in-game and alive
- Efficient ImGui draw calls

## Integration Tips

1. **Call `draw_logs()` once per frame** in your render/menu loop
2. **Add logs in your event handlers** (bullet impact, player hurt, etc.)
3. **Configure position and limits** based on your UI layout
4. **Use appropriate colors** to match your cheat's theme

## Credits

Inspired by GameSense CS2 hit log design.

## License

Free to use and modify.
