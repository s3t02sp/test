# Legitbot Implementation for CS2

A fully-featured legitbot implementation with trigger bot, smooth aiming, FOV limiting, and humanizer features based on your existing rage bot code structure.

## Features

### 1. **Smooth Aiming**
- Gradually moves crosshair towards target instead of instant snap
- Configurable smoothness factor (1-100)
- Higher values = smoother/slower aim movement
- Makes the aim look more natural and human-like

### 2. **FOV (Field of View) Limiting**
- Only targets enemies within specified FOV from your crosshair
- Prevents unrealistic 180° flicks
- Configurable FOV radius (recommended: 3-10 degrees)
- Targets closest to crosshair are prioritized

### 3. **Trigger Bot**
- Automatically shoots when crosshair is on an enemy
- Configurable delay before shooting
- Separate FOV setting for triggering (usually smaller than aim FOV)
- Can be enabled/disabled independently from aimbot

### 4. **Humanizer**
- **Reaction Time**: Adds realistic delay before starting to aim (50-200ms recommended)
- **Aim Shake**: Adds small random movements to simulate human imperfection
- **Random Variations**: Each action has slight timing variations
- Makes the bot harder to detect by anti-cheat systems

### 5. **Additional Features**
- **Team Check**: Option to ignore teammates
- **Visibility Check**: Only targets visible enemies
- **Recoil Control**: Compensates for weapon recoil
- **Hitbox Selection**: Choose which body part to aim at
- **Key Bind Support**: Activate on key press or always-on mode

## File Structure

```
legit_bot.hpp           - Header file with class definition
legit_bot.cpp           - Main implementation
legit_bot_config.hpp    - Configuration structure
```

## Integration Guide

### Step 1: Add to Your Project

1. Copy the three files to your project's appropriate directories
2. Include the header in your main hook file:

```cpp
#include "legit_bot.hpp"
```

### Step 2: Add Configuration

Add the configuration structure to your existing config system (merge with your `g_cfg` structure):

```cpp
struct config_t {
    // ... your existing config ...
    
    legit_bot_config_t legit_bot;
};
```

### Step 3: Call in CreateMove

Replace or add alongside your rage bot call:

```cpp
void __fastcall hooked_create_move(/* params */) {
    // ... your existing code ...
    
    // Call legitbot
    g_legit_bot->on_create_move(user_cmd);
    
    // ... rest of your code ...
}
```

## Configuration Examples

### Balanced Settings (Recommended)
```cpp
g_cfg->legit_bot.m_enabled = true;
g_cfg->legit_bot.m_fov = 5.0f;              // 5 degree FOV
g_cfg->legit_bot.m_smooth = 10.0f;          // Moderate smoothness
g_cfg->legit_bot.m_humanizer = true;
g_cfg->legit_bot.m_reaction_time = 120.0f;  // 120ms reaction delay
g_cfg->legit_bot.m_aim_shake = 0.05f;       // Slight shake
g_cfg->legit_bot.m_trigger_bot = true;
g_cfg->legit_bot.m_trigger_delay = 50.0f;   // 50ms trigger delay
```

### Rage-Like (High FOV, Low Smooth)
```cpp
g_cfg->legit_bot.m_fov = 15.0f;             // Large FOV
g_cfg->legit_bot.m_smooth = 3.0f;           // Fast aim
g_cfg->legit_bot.m_humanizer = false;       // No delays
g_cfg->legit_bot.m_trigger_bot = true;
g_cfg->legit_bot.m_trigger_delay = 0.0f;    // Instant trigger
```

### Very Legit (Low FOV, High Smooth)
```cpp
g_cfg->legit_bot.m_fov = 3.0f;              // Small FOV
g_cfg->legit_bot.m_smooth = 25.0f;          // Very smooth
g_cfg->legit_bot.m_humanizer = true;
g_cfg->legit_bot.m_reaction_time = 200.0f;  // Human-like delay
g_cfg->legit_bot.m_aim_shake = 0.1f;        // Noticeable shake
g_cfg->legit_bot.m_trigger_bot = false;     // Manual shooting
```

### Trigger Bot Only
```cpp
g_cfg->legit_bot.m_enabled = true;
g_cfg->legit_bot.m_smooth = 0.0f;           // No aim assist
g_cfg->legit_bot.m_trigger_bot = true;
g_cfg->legit_bot.m_trigger_delay = 30.0f;
g_cfg->legit_bot.m_trigger_fov = 0.5f;      // Very precise
```

## How Each Feature Works

### Smooth Aiming Algorithm

The smooth aiming uses an interpolation algorithm:

```cpp
vec3_t delta = target_angle - current_angle;
float smooth_amount = 1.0f / smooth_factor;
vec3_t smooth_angle = current_angle + delta * smooth_amount;
```

- Higher smooth factor = smaller smooth_amount = slower movement
- Lower smooth factor = larger smooth_amount = faster movement

### FOV Calculation

FOV is calculated using angular distance:

```cpp
float fov = sqrt(delta_pitch² + delta_yaw²)
```

- Only targets within configured FOV are considered
- Closest to crosshair (lowest FOV value) is selected

### Humanizer System

1. **Reaction Time**: Delays aim start by X milliseconds after target acquisition
2. **Aim Shake**: Adds periodic random offsets to aim angle
3. **Random Variations**: Each timing value has ±20ms variation

### Trigger Bot Logic

```
1. Check if crosshair is on target (within trigger FOV)
2. If yes, start delay timer
3. After delay expires, fire weapon
4. Reset on target loss
```

## Performance Considerations

- Target finding runs every frame but is optimized
- FOV calculations use fast sqrt approximation
- Humanizer updates only every 50ms
- No unnecessary memory allocations in hot path

## Customization Tips

### Adjusting for Different Weapons

```cpp
// In on_create_move, add weapon-specific settings:
if (weapon_data->m_weapon_type() == WEAPONTYPE_SNIPER_RIFLE) {
    // Slower, more precise for AWP
    smooth_factor = g_cfg->legit_bot.m_smooth * 1.5f;
    fov_limit = g_cfg->legit_bot.m_fov * 0.7f;
}
else if (weapon_data->m_weapon_type() == WEAPONTYPE_RIFLE) {
    // Standard settings
    smooth_factor = g_cfg->legit_bot.m_smooth;
    fov_limit = g_cfg->legit_bot.m_fov;
}
```

### Adding More Hitboxes

Modify `select_aim_point()` to support multiple hitboxes:

```cpp
std::vector<int> hitboxes = { HITBOX_HEAD, HITBOX_CHEST, HITBOX_STOMACH };
// Iterate and find best visible hitbox
```

### Advanced Humanizer Features

Add to the humanizer system:
- **Mouse Movement Patterns**: Simulate curved mouse paths
- **Fatigue Simulation**: Decrease accuracy over time
- **Over-correction**: Occasionally aim slightly past target
- **Distraction Delays**: Random pauses in aim tracking

## Troubleshooting

### Aim is Too Snappy
- Increase `m_smooth` value (try 15-30)
- Decrease `m_fov` value (try 3-5)
- Enable humanizer with higher reaction time

### Not Targeting Enemies
- Check `m_team_check` setting
- Verify `m_visibility_check` isn't too strict
- Increase `m_fov` value
- Ensure enemies are in valid range

### Trigger Bot Not Firing
- Verify `m_trigger_fov` isn't too small
- Check weapon ammo and fire rate
- Increase `m_trigger_delay` if firing too early
- Ensure visibility check is working correctly

### Aim Feels Robotic
- Enable humanizer
- Increase `m_aim_shake` (try 0.1-0.2)
- Add random reaction time (150-250ms)
- Use higher smooth values (20+)

## Security Considerations

**Important**: This is a game cheat and using it may result in:
- VAC/Game bans
- Account suspension
- Hardware bans

To minimize detection risk:
1. Use moderate settings (high smooth, low FOV)
2. Always enable humanizer features
3. Don't use trigger bot constantly
4. Vary your configuration
5. Don't make it obvious in game

## Advanced Integration

### Combining with Rage Bot

You can have both systems and switch between them:

```cpp
if (g_cfg->mode == MODE_RAGE) {
    g_rage_bot->on_create_move();
}
else if (g_cfg->mode == MODE_LEGIT) {
    g_legit_bot->on_create_move(user_cmd);
}
```

### Adding Bone Scan

For more legitimate aim point selection:

```cpp
// In select_aim_point()
std::vector<vec3_t> scan_points;
// Add multiple points around hitbox
// Select point with best visibility and closest to crosshair
```

### Backtracking Support

Add lag compensation similar to rage bot:

```cpp
// Store lag records
// Select best record based on latency
// Apply record before aiming
```

## Credits

Based on the rage bot implementation structure with significant modifications for legitbot behavior.

## License

Use at your own risk. For educational purposes only.
