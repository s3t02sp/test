# CS2 Advanced Movement System

A complete port of advanced movement mechanics from one CS2 cheat codebase to another, featuring sophisticated movement algorithms, subtick exploitation, and physics simulation.

## Features Implemented

### Core Movement Mechanics

#### 1. **Bunny Hop** (`bunnyhop`)
- Automatic jump timing to maintain speed
- Respects `sv_autobunnyhopping` convar
- Only triggers when holding jump button
- Ground detection using prediction flags

#### 2. **Auto Strafe** (`auto_strafe`, `directional_air_strafe`)
- Directional air strafing with intelligent key detection
- Maintains movement direction based on last pressed keys (W/A/S/D)
- Calculates optimal strafe angle based on velocity
- Automatic movement rotation relative to view angles
- Side switching for optimal acceleration
- Respects ladder and noclip movement types
- Configurable via `m_auto_strafe` config option

#### 3. **Subtick Air Strafe Exploit** (`subtick_strafer`)
- Exploits CS2's subtick system for enhanced air control
- Generates up to 32 subtick movement steps per tick
- Applies air acceleration physics per subtick
- Significantly improves air control and speed potential
- Configurable via `m_auto_strafe_exploit` config option

### Advanced Techniques

#### 4. **Jump Bug** (`jump_bug`)
- Detects edges and falling states
- Executes precise duck + jump timing via subticks
- Prevents fall damage through frame-perfect inputs
- Uses 4 subtick steps:
  - 0.0: Press duck
  - 0.25: Press jump
  - 0.999: Release duck
  - 0.999: Release jump
- Configurable via `m_jump_bug` config option

#### 5. **Auto Stop** (`auto_stop`, `stop_movement`)
- Intelligent movement halt for accurate shooting
- Two modes: quick stop and autostop for ragebot
- Adjusts movement to counter velocity
- Respects `weapon_accuracy_nospread` convar
- Works both on ground and in air (configurable)
- Movement speed limiting to weapon max speed * 0.25

### Physics Simulation

#### 6. **Friction Application** (`apply_friction`)
- Accurate Source engine friction simulation
- Considers surface friction and player friction
- Handles stamina and speed scaling
- Respects tick timing and stashed speed
- Uses `sv_friction` and `sv_stopspeed` convars

#### 7. **Position Prediction** (`simulate_pos_at_stop`)
- Simulates player position when stopping
- Up to 20 tick simulation
- Full acceleration physics including:
  - Weapon speed modifiers
  - Water level effects
  - Duck and sprint states
  - Hostage carry detection
  - Zoom slowdown mechanics
- Used for accurate shot prediction

#### 8. **Air Acceleration** (`air_accelerate`, `parachute_accelerate`)
- Two implementations for different use cases
- Handles stamina recovery and speed scaling
- Respects `sv_airaccelerate` and `sv_air_max_wishspeed`
- Calculates wish direction from view angles
- Used in subtick exploitation

### Movement Correction

#### 9. **Movement Fix** (`movement_fix`)
- Corrects movement relative to view angles
- Handles angle changes while maintaining intended direction
- Fixes forward/side/up movement vectors
- Accounts for pitch angle sign (looking up/down)
- Automatically updates button states via `fix_cmd_buttons`

#### 10. **Button State Fixing** (`fix_cmd_buttons`)
- Synchronizes button states with actual movement
- Sets IN_FORWARD/IN_BACK based on forwardmove
- Sets IN_MOVELEFT/IN_MOVERIGHT based on leftmove
- Only applies when on ground (unless called from strafer)

### Utility Functions

#### 11. **Speed Limiting** (`limit_speed`)
- Limits movement speed to specified maximum
- Calculates acceleration-aware speed adjustments
- Prevents overshooting target speed
- Uses surface friction and acceleration convars

#### 12. **Movement Trace Validation** (`is_valid_movement_trace`)
- Validates movement traces for physics queries
- Checks for stuck positions
- Verifies plane normal validity
- Performs forward and backward sweep tests
- Prevents invalid collision responses

#### 13. **Movement Control** (`handle_move_stop`, `handle_directional_strafe`)
- High-level movement control interface
- Manages halt states
- Delegates to appropriate strafe implementation
- Integrates with ragebot auto-stop

## Technical Details

### Naming Convention Migration

The port adapts from one codebase's naming to another:

**From:**
- `Globals::m_pCmd` → `g_ctx->m_user_cmd`
- `Globals::m_pBaseCmd` → `g_ctx->m_user_cmd->pb.mutable_base()`
- `Globals::m_pLocalPlayerPawn` → `g_ctx->m_local_pawn`
- `Config::b()` → `g_cfg->misc.m_*`
- `Interfaces::` → `g_interfaces->`
- `Vector` → `vec3_t`
- `QAngle` → `vec3_t`
- `M_DEG2RAD`/`M_RAD2DEG` → `DirectX::XMConvertToRadians`/`DirectX::XMConvertToDegrees`
- CamelCase functions → snake_case functions

### Key Constants

```cpp
constexpr int kMovementKeyBits =
    IN_TURNLEFT | IN_FORWARD | IN_BACK | 
    IN_TURNRIGHT | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP;
```

### Physics Constants
- `INTERVAL_PER_TICK`: Time per game tick
- Max subticks: 32
- Default simulation ticks: 20
- Strafe ideal angle range: 0-45 degrees

## Configuration Options

Required config variables:
- `m_bunny_hop`: Enable bunny hopping
- `m_auto_strafe`: Enable directional air strafing
- `m_auto_strafe_exploit`: Enable subtick air strafe exploit
- `m_jump_bug`: Enable jump bug
- `m_quick_stop`: Enable quick stop
- `m_auto_stop`: Enable auto stop for ragebot
- `m_auto_stop_early`: Use early stop prediction
- `m_auto_stop_in_air`: Allow auto stop in air

## Integration

### Basic Usage

```cpp
void on_create_move(c_user_cmd* cmd) {
    g_movement->on_create_move(cmd);
}
```

### Advanced Usage

```cpp
// Set movement angles before processing
g_movement->m_ang_model_angles = model_angles;
g_movement->m_ang_camera_angles = camera_angles;

// Process movement
g_movement->on_create_move(cmd);

// For ragebot integration
if (should_stop_for_shot) {
    g_movement->handle_move_stop(true);
}

// Apply movement correction
g_movement->movement_fix();
```

## Dependencies

Required interfaces and systems:
- `g_ctx`: Global context (player, cmd, weapon)
- `g_cfg`: Configuration system
- `g_interfaces`: Game interfaces (input, cvars, physics)
- `g_math`: Math utilities (angle_vectors, normalize_yaw, calc_angle)
- `g_protobuf`: Protobuf helpers for subtick moves

Required game interfaces:
- `m_csgo_input`: For view angles
- `m_var`: ConVar interface
- `m_vphys2_world`: Physics queries
- `m_global_vars`: Timing information

## Algorithm Highlights

### Strafe Angle Calculation
```
ideal_angle = clamp(
    atan(max(15, 30 - (speed * accel * friction * frametime)) / velocity) * RAD2DEG,
    0, 45
)
```

### Subtick Generation
- Divides tick into equal substeps (up to 32)
- Each substep applies:
  1. Air acceleration physics
  2. Strafe direction calculation
  3. Movement impulse delta calculation

### Movement Correction
- Projects movement onto view angle basis
- Handles pitch sign for inverted looking
- Normalizes all input vectors
- Clamps final values to [-1, 1]

## Known Limitations

1. Jump bug detection relies on prediction flags
2. Subtick exploit limited to 32 substeps
3. Position simulation capped at 20 ticks
4. Some convars are cached statically (update on change needed)

## Future Enhancements

- Edge jump implementation (placeholder exists)
- Dynamic convar updating
- Movement analytics/statistics
- Visual debug overlay
- Movement recording/playback

## Credits

Ported from advanced CS2 movement system with full physics simulation and subtick exploitation capabilities.
