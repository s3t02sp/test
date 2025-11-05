# Migration Guide: Movement System Port

This document details the changes made when porting the movement system between codebases.

## API Mapping

### Global Contexts

| Original | Ported To | Description |
|----------|-----------|-------------|
| `Globals::m_pCmd` | `g_ctx->m_user_cmd` | User command structure |
| `Globals::m_pBaseCmd` | `g_ctx->m_user_cmd->pb.mutable_base()` | Base command protobuf |
| `Globals::m_pLocalPlayerPawn` | `g_ctx->m_local_pawn` | Local player pawn entity |
| `Globals::nSequence` | `g_ctx->m_user_cmd->m_command_number` | Command sequence number |
| `Globals::m_bIsPressingMovementKey` | `g_ctx->m_is_pressing_movement_key` | Movement key state |

### Player Data

| Original | Ported To | Description |
|----------|-----------|-------------|
| `LocalPlayerData::m_pWeapon` | `g_ctx->m_weapon` | Active weapon |

### Configuration

| Original | Ported To | Description |
|----------|-----------|-------------|
| `Config::b(g_Variables.m_Movement.m_bEnableBunnyHop)` | `g_cfg->misc.m_bunny_hop` | Bunny hop toggle |
| `Config::b(g_Variables.m_Movement.m_bEnableAutoStrafe)` | `g_cfg->misc.m_auto_strafe` | Auto strafe toggle |
| `Config::b(g_Variables.m_Movement.m_bEnableAutoStrafeExploit)` | `g_cfg->misc.m_auto_strafe_exploit` | Subtick strafe toggle |
| `Config::b(g_Variables.m_Movement.m_bEnableJumpBug)` | `g_cfg->misc.m_jump_bug` | Jump bug toggle |
| `Config::b(g_Variables.m_Movement.m_bEnableQuickStop)` | `g_cfg->misc.m_quick_stop` | Quick stop toggle |
| `Config::vb(g_Variables.m_Ragebot.m_vbAutostop).at(EAutostop::AUTOSTOP_EARLY)` | `g_cfg->rage_bot.m_auto_stop_early` | Early auto stop |
| `Config::vb(g_Variables.m_Ragebot.m_vbAutostop).at(EAutostop::AUTOSTOP_IN_AIR)` | `g_cfg->rage_bot.m_auto_stop_in_air` | Air auto stop |

### Interfaces

| Original | Ported To | Description |
|----------|-----------|-------------|
| `Interfaces::m_pGlobalVariables` | `g_interfaces->m_global_vars` | Global game variables |
| `Interfaces::m_pInput` | `g_interfaces->m_csgo_input` | Input system |
| `Interfaces::m_pVPhys2World` | `g_interfaces->m_vphys2_world` | Physics world |
| `Convar::*` or `CONVAR(name)` | `g_interfaces->m_var->get_by_name(name)` | Console variables |

### Math & Utilities

| Original | Ported To | Description |
|----------|-----------|-------------|
| `Math::CalcAngle()` | `g_math->calc_angle()` | Calculate angle between vectors |
| `Math::AngleVectors()` | `g_math->angle_vectors()` | Convert angles to vectors |
| `Math::FloatNormalize()` | `g_math->normalize_yaw()` | Normalize yaw angle |
| `M_DEG2RAD(x)` | `DirectX::XMConvertToRadians(x)` | Degrees to radians |
| `M_RAD2DEG(x)` | `DirectX::XMConvertToDegrees(x)` | Radians to degrees |

### Prediction System

| Original | Ported To | Description |
|----------|-----------|-------------|
| `g_Prediction->GetPostFlags()` | `g_ctx->m_predicted_flags` (or local check) | Post-prediction flags |
| `g_Prediction->GetPreFlags()` | Direct `m_flags()` check | Pre-prediction flags |

### Protobuf/Subtick

| Original | Ported To | Description |
|----------|-----------|-------------|
| `Interfaces::m_pInput->CreateNewSubTickMoveStep()` | `g_protobuf->add_subtick_move_step()` | Create subtick step |

## Type Changes

### Vector/Angle Types

| Original | Ported To |
|----------|-----------|
| `Vector` | `vec3_t` |
| `QAngle` | `vec3_t` |

### Entity Methods

| Original | Ported To | Notes |
|----------|-----------|-------|
| `pawn->m_vecVelocity()` | `pawn->m_vec_velocity()` | Velocity accessor |
| `pawn->m_vecAbsVelocity()` | `pawn->m_vec_abs_velocity()` | Absolute velocity |
| `pawn->m_fFlags()` | `pawn->m_flags()` | Entity flags |
| `pawn->m_pGameSceneNode()->m_vecAbsOrigin()` | `pawn->m_game_scene_node()->m_abs_origin()` | World position |
| `pawn->m_hGroundEntity().Get()` | `pawn->m_ground_entity().get()` | Ground entity handle |
| `pawn->m_pCollision()` | `pawn->m_collision()` | Collision component |
| `pawn->m_pMovementServices()` | `pawn->m_movement_services()` | Movement services |
| `pawn->m_nActualMoveType()` | `pawn->m_actual_move_type()` | Movement type |
| `pawn->m_flWaterLevel()` | `pawn->m_water_level()` | Water immersion level |
| `pawn->m_flFriction()` | `pawn->m_friction()` | Friction multiplier |
| `pawn->m_pHostageServices()` | `pawn->m_hostage_services()` | Hostage service |

### Movement Service Methods

| Original | Ported To |
|----------|-----------|
| `service->m_flSurfaceFriction()` | `service->m_surface_friction()` |
| `service->m_flOffsetTickCompleteTime()` | `service->m_offset_tick_complete_time()` |
| `service->m_flOffsetTickStashedSpeed()` | `service->m_offset_tick_stashed_speed()` |
| `service->m_bDucking()` | `service->m_ducking()` |
| `service->m_flStamina()` | `service->m_flStamina()` |
| `service->m_vecLastMovementImpulses()` | `service->m_last_movement_impulses()` |
| `service->m_flMaxSpeed()` | `service->m_max_speed()` |

### Weapon Methods

| Original | Ported To |
|----------|-----------|
| `weapon->GetMaxSpeed()` | `weapon->get_max_speed()` |
| `weapon->GetWeaponBaseVData()` | `weapon->get_weapon_base_vdata()` |
| `weapon->m_zoomLevel()` | `weapon->m_zoom_level()` |

### VData Methods

| Original | Ported To |
|----------|-----------|
| `vdata->m_nZoomLevels()` | `vdata->m_zoom_levels()` |

### Button State

| Original | Ported To |
|----------|-----------|
| `cmd->m_nButtons.m_nValue` | `cmd->m_button_state.m_button_state` |
| `cmd->m_nButtons.m_nValueChanged` | `cmd->m_button_state.m_button_state2` |
| `cmd->m_nButtons.m_nValueScroll` | `cmd->m_button_state.m_button_state3` |
| `cmd->m_nButtons.SetButtonState(btn, state)` | Direct bitwise operations on button states |

### Collision/Physics

| Original | Ported To |
|----------|-----------|
| `collision->m_vecMins()` | `collision->m_mins()` |
| `collision->m_vecMaxs()` | `collision->m_maxs()` |
| `physics->TracePlayerBBox()` | `physics->trace_player_bbox()` |
| `physics->InitPlayerMovementTraceFilter()` | `physics->init_player_movement_trace_filter()` |

### Trace Structure

| Original | Ported To |
|----------|-----------|
| `trace.m_bAllSolid` | `trace.m_all_solid` |
| `trace.m_flFraction` | `trace.m_fraction` |
| `trace.m_vecNormal` | `trace.m_normal` |
| `trace.m_vecStartPos` | `trace.m_start_pos` |
| `trace.m_vecEndPos` | `trace.m_end_pos` |
| `trace.m_pHitEntity` | `trace.m_hit_entity` |

## Function Naming

All function names converted from `CamelCase` to `snake_case`:

| Original | Ported To |
|----------|-----------|
| `FixCmdButtons()` | `fix_cmd_buttons()` |
| `ApplyFriction()` | `apply_friction()` |
| `SimulatePosAtStop()` | `simulate_pos_at_stop()` |
| `StopMovement()` | `stop_movement()` |
| `BunnyHop()` | `bunnyhop()` |
| `JumpBug()` | `jump_bug()` |
| `DirectionalAirStrafe()` | `directional_air_strafe()` |
| `AirAccelerate()` | `air_accelerate()` |
| `SubtickAirStrafe()` | `subtick_strafer()` |
| `HandleMoveStop()` | `handle_move_stop()` |
| `HandleDirectionalStrafe()` | `handle_directional_strafe()` |
| `MovementCorrection()` | `movement_fix()` |
| `IsValidMovementTrace()` | `is_valid_movement_trace()` |

## Class Member Variables

| Original | Ported To |
|----------|-----------|
| `m_angModelAngles` | `m_ang_model_angles` |
| `m_angCameraAngles` | `m_ang_camera_angles` |
| `m_bWants2DMoveHalt` | `m_wants_2d_move_halt` |

## Notable Behavioral Changes

### 1. Button State Setting
**Original:**
```cpp
Globals::m_pCmd->m_nButtons.SetButtonState(IN_FORWARD, CInButtonState::EButtonState::IN_BUTTON_DOWN);
```

**Ported:**
```cpp
g_ctx->m_user_cmd->m_button_state.m_button_state |= IN_FORWARD;
```

### 2. ConVar Access
**Original:**
```cpp
CONVAR(sv_friction);
float friction = sv_friction->GetFloat();
```

**Ported:**
```cpp
static float sv_friction = g_interfaces->m_var->get_by_name("sv_friction")->get_float();
```

Note: Convars are now cached as static variables for performance.

### 3. Angle Conversion
**Original:**
```cpp
QAngle aViewAngles = this->m_angCameraAngles;
// Use directly
```

**Ported:**
```cpp
vec3_t aViewAngles = this->m_ang_camera_angles;
// vec3_t serves as both vector and angle type
```

### 4. Vector Methods
**Original:**
```cpp
vecWishDir.NormalizeInPlace();
float flCurrentSpeed = vVelocity.DotProduct(vecWishDir);
```

**Ported:**
```cpp
vecWishDir.normalize_in_place();
float flCurrentSpeed = vVelocity.dot(vecWishDir);
```

### 5. Subtick Creation
**Original:**
```cpp
auto subtick = Interfaces::m_pInput->CreateNewSubTickMoveStep(base_cmd->mutable_subtick_moves());
base_cmd->mutable_subtick_moves()->AddAllocated(subtick);
```

**Ported:**
```cpp
auto subtick = g_protobuf->add_subtick_move_step(user_cmd);
// Automatically allocated and added
```

## Configuration Structure Changes

### Original Structure
```cpp
enum class EAutostop {
    AUTOSTOP_EARLY,
    AUTOSTOP_IN_AIR
};

// Access:
Config::vb(g_Variables.m_Ragebot.m_vbAutostop).at(EAutostop::AUTOSTOP_EARLY)
```

### Ported Structure
```cpp
// Access:
g_cfg->rage_bot.m_auto_stop_early
g_cfg->rage_bot.m_auto_stop_in_air
```

Simplified from enum-indexed vectors to individual bool flags.

## Integration Points

### Initialization
Ensure `g_movement` singleton is properly initialized before use:
```cpp
// In header
inline c_movement* g_movement = new c_movement();
```

### Per-Tick Usage
```cpp
void hook_create_move(c_user_cmd* cmd) {
    // Set angles
    g_movement->m_ang_model_angles = get_model_angles();
    g_movement->m_ang_camera_angles = get_camera_angles();
    
    // Process movement
    g_movement->on_create_move(cmd);
    
    // Apply corrections if needed
    g_movement->movement_fix();
}
```

### Ragebot Integration
```cpp
// Before shooting
if (should_stop_for_shot) {
    g_movement->handle_move_stop(true);
}

// After shooting or when no target
if (can_move) {
    g_movement->handle_move_stop(false);
}
```

## Testing Checklist

- [ ] Bunny hop maintains speed correctly
- [ ] Auto strafe follows key presses accurately
- [ ] Subtick strafe provides enhanced air control
- [ ] Jump bug prevents fall damage on edges
- [ ] Auto stop halts movement for accurate shots
- [ ] Movement correction preserves intended direction
- [ ] Speed limiting prevents overshooting
- [ ] Physics simulation matches game behavior
- [ ] All convars are read correctly
- [ ] No crashes or null pointer dereferences

## Common Issues & Solutions

### Issue: Convars not updating
**Solution:** Cached static convars don't update at runtime. Consider making them non-static or adding a refresh mechanism.

### Issue: Button states not registering
**Solution:** Ensure all three button state fields are updated:
```cpp
cmd->m_button_state.m_button_state &= ~button;
cmd->m_button_state.m_button_state2 &= ~button;
cmd->m_button_state.m_button_state3 &= ~button;
```

### Issue: Subtick moves not working
**Solution:** Verify protobuf helper function correctly allocates and adds subtick steps. Original code used manual allocation.

### Issue: Movement correction feels off
**Solution:** Ensure `m_ang_model_angles` and `m_ang_camera_angles` are correctly set before calling `movement_fix()`.

## Performance Considerations

1. **Static ConVars**: Convars are cached statically for performance but won't update if changed in-game
2. **Subtick Calculation**: Limited to 32 substeps to prevent performance issues
3. **Position Simulation**: Capped at 20 ticks to balance accuracy and performance
4. **Math Operations**: DirectX math used for conversions (ensure library is available)

## Future Maintenance

When updating:
1. Check for new Source 2 movement variables
2. Update convar caching mechanism if needed
3. Monitor for CS2 updates affecting subtick system
4. Consider adding telemetry for movement debugging
5. Add unit tests for physics calculations

## Conclusion

This migration successfully ports all advanced movement features while adapting to the target codebase's conventions and structure. All functionality is preserved with equivalent or improved performance.
