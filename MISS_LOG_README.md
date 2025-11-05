# Rage Bot Miss Log System

Comprehensive miss logging system for tracking and analyzing shot misses due to spread, prediction errors, and other factors.

## Features

### 📊 Shot Tracking
- Tracks every shot fired including:
  - Target information (name, handle)
  - Aim point and angles
  - Hitbox targeted
  - Expected damage
  - Hit chance percentage
  - Spread and inaccuracy values
  - Tick count and simulation time

### 🎯 Miss Detection
The system detects and categorizes misses into different types:

1. **MISS_REASON_SPREAD** - Shot missed due to weapon spread/inaccuracy
2. **MISS_REASON_PREDICTION_ERROR** - Shot missed due to movement prediction error
3. **MISS_REASON_RESOLVER** - Shot missed due to anti-aim resolver failure
4. **MISS_REASON_HITBOX** - Shot hit but on wrong hitbox

### 📈 Data Collection
- Stores last 64 shots in history
- Keeps last 32 misses with full details
- Tracks bullet impact positions
- Monitors player hurt events

### 🖥️ Visualization
- **2D Overlay**: Shows recent misses with color-coded reasons
  - Red: Spread/Inaccuracy
  - Orange: Prediction Error
  - Yellow: Resolver Issue
  - Gray: Other/Unknown
  
- **3D World Visualization**: 
  - Green line: Intended shot path to aim point
  - Colored line: Actual bullet trajectory to impact
  - Spheres marking aim and impact positions

### 📝 Console Logging
Detailed console output for each miss:
```
[MISS] Target: PlayerName | Hitbox: Head | Reason: Spread/Inaccuracy | HC: 85% | Spread: 15 | Inaccuracy: 8 | Distance from point: 42 units
```

Or for wrong hitbox hits:
```
[SPREAD MISS] Target: PlayerName | Aimed: Head | Hit: Chest | HC: 75% | Spread: 12 | Inaccuracy: 6
```

## Integration Guide

### 1. Add to Your Project

Include the following files in your project:
- `rage_bot.hpp` - Main header with structures and class definition
- `rage_bot.cpp` - Implementation with miss log functionality
- `event_handler.cpp` - Game event hooks for bullet impact and player hurt
- `miss_log_render.cpp` - Rendering system for miss log visualization
- `config_example.hpp` - Configuration options

### 2. Hook Game Events

In your game event listener, register these events:

```cpp
void setup_events() {
    // Register bullet impact event
    g_interfaces->m_event_manager->add_listener("bullet_impact", 
        [](c_game_event* event) { 
            g_event_handler->on_bullet_impact(event); 
        });
    
    // Register player hurt event
    g_interfaces->m_event_manager->add_listener("player_hurt", 
        [](c_game_event* event) { 
            g_event_handler->on_player_hurt(event); 
        });
    
    // Optional: Clear logs on round start
    g_interfaces->m_event_manager->add_listener("round_start", 
        [](c_game_event* event) { 
            g_event_handler->on_round_start(event); 
        });
}
```

### 3. Add Rendering

In your render loop:

```cpp
// In your 2D overlay render function
void render_2d() {
    // ... other 2D rendering ...
    
    g_miss_log_render->render();
}

// In your 3D world render function
void render_3d() {
    // ... other 3D rendering ...
    
    g_miss_log_render->render_3d_visualizations();
}
```

### 4. Configure Settings

Add the miss log settings to your config:

```cpp
struct config_t {
    // ... existing config ...
    
    rage_bot_config_t rage_bot;
};
```

## Usage

### Enable Miss Logging
```cpp
g_cfg->rage_bot.m_log_misses = true;
```

### Access Miss Logs Programmatically
```cpp
// Get all miss logs
auto& miss_logs = g_rage_bot->get_miss_logs();

// Iterate through misses
for (const auto& miss : miss_logs) {
    std::cout << "Target: " << miss.m_shot_info.m_target_name << std::endl;
    std::cout << "Reason: " << miss.m_reason_string << std::endl;
    std::cout << "Hit Chance: " << miss.m_shot_info.m_hit_chance << "%" << std::endl;
    std::cout << "Spread: " << miss.m_shot_info.m_spread << std::endl;
}

// Clear miss logs
g_rage_bot->clear_miss_logs();
```

### Understanding Miss Reasons

#### Spread/Inaccuracy (Red)
- Occurs when weapon spread or inaccuracy causes bullet to deviate
- More common with:
  - High spread weapons (SMGs, shotguns)
  - Moving while shooting
  - Not scoped with snipers
  - Rapid fire
- **Fix**: Increase hit chance requirement, enable auto-stop, use static scale

#### Prediction Error (Orange)
- Bullet landed close but missed target
- Target moved differently than predicted
- **Fix**: Improve movement prediction, lower backtrack tick selection

#### Resolver (Yellow)
- Common with head shots
- Anti-aim not properly resolved
- **Fix**: Improve resolver algorithm, try body aim

#### Wrong Hitbox (Hit but wrong)
- Shot connected but hit different hitbox than intended
- Usually due to spread affecting trajectory
- **Fix**: Adjust multi-point configuration, increase point scale

## Configuration Options

### Basic Settings
```cpp
m_log_misses = true;              // Master switch for miss logging
m_show_miss_log = true;           // Show 2D overlay
m_show_3d_miss_log = true;        // Show 3D visualization
```

### Display Settings
```cpp
m_miss_log_x = 10;                // Screen X position
m_miss_log_y = 300;               // Screen Y position
m_max_displayed_misses = 10;      // Number of misses to show
m_miss_log_fade_time = 10.f;      // Fade out time in seconds
```

### Filtering
```cpp
m_log_spread_misses = true;       // Log spread-related misses
m_log_prediction_misses = true;   // Log prediction errors
m_log_resolver_misses = true;     // Log resolver issues
m_log_wrong_hitbox = true;        // Log wrong hitbox hits
```

### Console Output
```cpp
m_print_misses_to_console = true; // Print to console
m_verbose_miss_logging = false;   // Extra details
```

## Performance Considerations

- Miss log has minimal performance impact
- Only stores last 64 shots and 32 misses
- Automatic cleanup of old entries
- 3D visualization only renders for last 3 seconds

## Advanced Features

### Custom Miss Analysis
```cpp
// Analyze miss patterns
int spread_misses = 0;
int resolver_misses = 0;

for (const auto& miss : g_rage_bot->get_miss_logs()) {
    if (miss.m_reason == MISS_REASON_SPREAD)
        spread_misses++;
    else if (miss.m_reason == MISS_REASON_RESOLVER)
        resolver_misses++;
}

// Adjust settings based on patterns
if (spread_misses > resolver_misses) {
    // Too many spread misses, increase hit chance
    g_cfg->rage_bot.m_hit_chance += 5;
}
```

### Shot Statistics
```cpp
// Calculate hit rate
int total_shots = m_shot_records.size();
int total_misses = m_miss_logs.size();
float hit_rate = (1.0f - (float)total_misses / total_shots) * 100.f;
```

## Troubleshooting

### Miss Log Not Working
1. Ensure `m_log_misses` is enabled
2. Check game event hooks are registered
3. Verify rage bot is enabled and firing

### Events Not Firing
1. Check event names match your game version
2. Verify event listener is properly registered
3. Ensure user ID to handle conversion is correct

### 3D Visualization Not Showing
1. Check render hooks are in correct render stage
2. Verify 3D rendering is enabled in config
3. Ensure world-to-screen is working

## Example Output

Console log example:
```
[MISS] Target: player123 | Hitbox: Head | Reason: Spread/Inaccuracy | HC: 82% | Spread: 18 | Inaccuracy: 10 | Distance from point: 56 units
[SPREAD MISS] Target: enemy456 | Aimed: Chest | Hit: Stomach | HC: 91% | Spread: 5 | Inaccuracy: 3
[MISS] Target: target789 | Hitbox: Head | Reason: Resolver | HC: 95% | Spread: 2 | Inaccuracy: 1 | Distance from point: 128 units
```

## Credits

This miss log system provides comprehensive tracking and visualization to help improve aim bot accuracy and understand why shots miss.
