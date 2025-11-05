#include "legit_bot.hpp"
#include "../../entity_system/entity.hpp"
#include "../../render/render.hpp"
#include <algorithm>
#include <random>

// ============================================================================
// Main Entry Point
// ============================================================================

void c_legit_bot::on_create_move(c_user_cmd* user_cmd)
{
	if (!g_cfg->legit_bot.m_enabled)
		return;

	if (!g_interfaces->m_engine->is_in_game())
		return;

	if (!user_cmd)
		return;

	c_cs_player_pawn* local_pawn = g_ctx->m_local_pawn;
	if (!local_pawn || !local_pawn->is_alive())
	{
		reset();
		return;
	}

	c_base_player_weapon* active_weapon = local_pawn->get_active_weapon();
	if (!active_weapon)
		return;

	c_cs_weapon_base_v_data* weapon_data = active_weapon->get_weapon_data();
	if (!weapon_data 
		|| weapon_data->m_weapon_type() == WEAPONTYPE_KNIFE 
		|| weapon_data->m_weapon_type() == WEAPONTYPE_GRENADE)
		return;

	// Update target list
	find_targets();

	// Select best target based on FOV
	legit_target_t* best_target = select_best_target();
	
	if (!best_target)
	{
		reset();
		return;
	}

	m_current_target = best_target;

	// Get current view angles
	vec3_t current_angle = g_interfaces->m_csgo_input->get_view_angles();

	// Calculate target angle
	auto local_data = g_prediction->get_local_data();
	if (!local_data)
		return;

	vec3_t aim_point = select_aim_point(best_target);
	vec3_t target_angle = g_math->aim_direction(local_data->m_eye_pos, aim_point);

	// Apply recoil control if enabled
	if (g_cfg->legit_bot.m_recoil_control && g_cfg->legit_bot.m_recoil_control_amount > 0.f)
	{
		vec3_t recoil_offset = get_recoil_control_angle(local_pawn);
		target_angle -= recoil_offset * (g_cfg->legit_bot.m_recoil_control_amount / 100.f);
	}

	// Check if we need to apply reaction delay
	if (g_cfg->legit_bot.m_humanizer && g_cfg->legit_bot.m_reaction_time > 0)
	{
		auto current_time = std::chrono::steady_clock::now();
		if (!m_reaction_delay_active)
		{
			m_last_target_acquisition_time = current_time;
			m_reaction_delay_active = true;
			return; // Don't aim yet, apply reaction delay
		}

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			current_time - m_last_target_acquisition_time).count();
		
		if (elapsed < get_reaction_time())
			return; // Still in reaction delay
	}

	// Apply humanizer modifications
	if (g_cfg->legit_bot.m_humanizer)
	{
		apply_humanizer();
		target_angle = add_human_error(target_angle);
	}

	// Calculate smooth angle
	vec3_t final_angle = target_angle;
	
	if (g_cfg->legit_bot.m_smooth > 0.f)
	{
		final_angle = apply_smoothing(current_angle, target_angle, g_cfg->legit_bot.m_smooth);
	}

	// Normalize angles
	g_math->normalize_angles(final_angle);
	g_math->clamp_angles(final_angle);

	// Apply aim if aimbot key is held or always on
	bool should_aim = g_cfg->legit_bot.m_always_on || 
		g_key_handler->is_pressed(g_cfg->legit_bot.m_aim_key_bind, g_cfg->legit_bot.m_aim_key_bind_style);

	if (should_aim)
	{
		g_interfaces->m_csgo_input->set_view_angles(final_angle);
		
		for (int i = 0; i < user_cmd->pb.input_history_size(); i++)
		{
			auto tick = user_cmd->pb.mutable_input_history(i);
			if (tick)
			{
				tick->mutable_view_angles()->set_x(final_angle.x);
				tick->mutable_view_angles()->set_y(final_angle.y);
				tick->mutable_view_angles()->set_z(final_angle.z);
			}
		}
	}

	// Process trigger bot
	if (g_cfg->legit_bot.m_trigger_bot && can_shoot(active_weapon))
	{
		process_trigger(user_cmd);
	}
}

// ============================================================================
// Target Selection
// ============================================================================

void c_legit_bot::find_targets()
{
	m_targets.clear();

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive())
		return;

	auto entities = g_entity_system->get("CCSPlayerController");

	for (auto entity : entities)
	{
		if (!entity)
			continue;

		auto player_controller = reinterpret_cast<c_cs_player_controller*>(entity);
		if (!player_controller || player_controller == g_ctx->m_local_controller)
			continue;

		if (!player_controller->m_pawn_is_alive())
			continue;

		auto player_pawn = reinterpret_cast<c_cs_player_pawn*>(
			g_interfaces->m_entity_system->get_base_entity(
				player_controller->m_pawn().get_entry_index()));
		
		if (!player_pawn)
			continue;

		// Team check
		if (g_cfg->legit_bot.m_team_check && 
			player_pawn->m_team_num() == g_ctx->m_local_pawn->m_team_num())
			continue;

		if (!player_pawn->is_alive())
			continue;

		// Add to targets list
		legit_target_t target(player_pawn, player_controller);
		m_targets.push_back(target);
	}
}

legit_target_t* c_legit_bot::select_best_target()
{
	if (m_targets.empty())
		return nullptr;

	auto local_data = g_prediction->get_local_data();
	if (!local_data)
		return nullptr;

	vec3_t view_angle = g_interfaces->m_csgo_input->get_view_angles();
	vec3_t eye_pos = local_data->m_eye_pos;

	legit_target_t* best_target = nullptr;
	float best_fov = g_cfg->legit_bot.m_fov;

	for (auto& target : m_targets)
	{
		if (!target.m_pawn || !target.m_pawn->is_alive())
			continue;

		// Get aim point for target
		vec3_t aim_point = get_hitbox_position(target.m_pawn, g_cfg->legit_bot.m_target_hitbox);
		
		// Visibility check
		if (g_cfg->legit_bot.m_visibility_check && 
			!is_visible(g_ctx->m_local_pawn, target.m_pawn, aim_point))
			continue;

		// Calculate angle to target
		vec3_t target_angle = g_math->aim_direction(eye_pos, aim_point);
		
		// Calculate FOV
		float fov = calculate_fov(view_angle, target_angle);

		// Check if within FOV limit
		if (fov > best_fov)
			continue;

		// Distance calculation
		float distance = eye_pos.dist(aim_point);

		// Store data
		target.m_aim_point = aim_point;
		target.m_fov = fov;
		target.m_distance = distance;

		// Select closest to crosshair (lowest FOV)
		if (!best_target || fov < best_fov)
		{
			best_fov = fov;
			best_target = &target;
		}
	}

	return best_target;
}

// ============================================================================
// FOV Calculations
// ============================================================================

float c_legit_bot::calculate_fov(const vec3_t& view_angle, const vec3_t& aim_angle)
{
	vec3_t delta;
	delta.x = aim_angle.x - view_angle.x;
	delta.y = aim_angle.y - view_angle.y;

	g_math->normalize_angles(delta);

	return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}

bool c_legit_bot::is_visible(c_cs_player_pawn* local_pawn, c_cs_player_pawn* target_pawn, const vec3_t& target_pos)
{
	if (!local_pawn || !target_pawn)
		return false;

	auto local_data = g_prediction->get_local_data();
	if (!local_data)
		return false;

	vec3_t start = local_data->m_eye_pos;
	vec3_t end = target_pos;

	ray_t ray{};
	game_trace_t trace{};
	trace_filter_t filter{};
	
	g_interfaces->m_trace->init_trace(filter, local_pawn, MASK_SHOT, 0x3, 0x7);
	g_interfaces->m_trace->trace_shape(&ray, start, end, &filter, &trace);

	if (!trace.m_hit_entity)
		return false;

	return trace.m_hit_entity == target_pawn;
}

// ============================================================================
// Aim Point Selection
// ============================================================================

vec3_t c_legit_bot::get_hitbox_position(c_cs_player_pawn* pawn, int hitbox)
{
	if (!pawn || !pawn->m_scene_node())
		return vec3_t(0.f, 0.f, 0.f);

	// Get bone position based on hitbox
	auto game_scene = pawn->m_scene_node()->get_skeleton_instance();
	if (!game_scene)
		return pawn->m_scene_node()->m_origin();

	// Map hitbox to bone
	int bone_index = hitbox; // Simplified - you may need proper hitbox to bone mapping

	vec3_t bone_pos;
	if (!game_scene->calc_world_space_bones(1 << bone_index))
		return pawn->m_scene_node()->m_origin();

	auto bone_array = game_scene->model_state().bone_state_ptr();
	if (!bone_array)
		return pawn->m_scene_node()->m_origin();

	bone_pos = bone_array[bone_index].m_position;

	return bone_pos;
}

vec3_t c_legit_bot::select_aim_point(legit_target_t* target)
{
	if (!target || !target->m_pawn)
		return vec3_t(0.f, 0.f, 0.f);

	// Use the hitbox from config
	int hitbox = g_cfg->legit_bot.m_target_hitbox;

	return get_hitbox_position(target->m_pawn, hitbox);
}

// ============================================================================
// Smooth Aiming
// ============================================================================

vec3_t c_legit_bot::apply_smoothing(const vec3_t& current_angle, const vec3_t& target_angle, float smooth_factor)
{
	vec3_t delta = target_angle - current_angle;
	g_math->normalize_angles(delta);

	// Convert smooth factor to a usable value
	// Higher smooth value = slower aim
	float smooth_amount = 1.0f / std::max(smooth_factor, 1.0f);

	vec3_t smooth_angle;
	smooth_angle.x = current_angle.x + delta.x * smooth_amount;
	smooth_angle.y = current_angle.y + delta.y * smooth_amount;
	smooth_angle.z = current_angle.z;

	return smooth_angle;
}

// ============================================================================
// Humanizer
// ============================================================================

void c_legit_bot::apply_humanizer()
{
	static auto last_update = std::chrono::steady_clock::now();
	auto current_time = std::chrono::steady_clock::now();
	
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		current_time - last_update).count();

	// Update random offsets periodically
	if (elapsed > 50) // Update every 50ms
	{
		update_random_offsets();
		last_update = current_time;
	}
}

float c_legit_bot::get_reaction_time()
{
	// Return reaction time in milliseconds
	float base_time = g_cfg->legit_bot.m_reaction_time;
	
	// Add some randomness for more human-like behavior
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(-20.0f, 20.0f);
	
	float random_offset = dis(gen);
	
	return std::max(0.0f, base_time + random_offset);
}

vec3_t c_legit_bot::add_human_error(const vec3_t& angle)
{
	if (!g_cfg->legit_bot.m_humanizer || g_cfg->legit_bot.m_aim_shake == 0.f)
		return angle;

	vec3_t error_angle = angle;
	
	// Apply random offsets
	float shake_amount = g_cfg->legit_bot.m_aim_shake;
	error_angle.x += m_random_offset_x * shake_amount;
	error_angle.y += m_random_offset_y * shake_amount;

	return error_angle;
}

void c_legit_bot::update_random_offsets()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(-0.1f, 0.1f);

	m_random_offset_x = dis(gen);
	m_random_offset_y = dis(gen);
}

// ============================================================================
// Recoil Control
// ============================================================================

vec3_t c_legit_bot::get_recoil_control_angle(c_cs_player_pawn* local_pawn)
{
	if (!local_pawn)
		return vec3_t(0.f, 0.f, 0.f);

	// Get aim punch angle
	vec3_t aim_punch = local_pawn->m_aim_punch_angle();
	
	// CS2 uses a different system, adjust as needed
	vec3_t recoil_angle;
	recoil_angle.x = aim_punch.x * 2.0f; // Pitch compensation
	recoil_angle.y = aim_punch.y * 2.0f; // Yaw compensation
	recoil_angle.z = 0.0f;

	return recoil_angle;
}

// ============================================================================
// Trigger Bot
// ============================================================================

bool c_legit_bot::is_on_target(const vec3_t& view_angle, legit_target_t* target)
{
	if (!target || !target->m_pawn)
		return false;

	auto local_data = g_prediction->get_local_data();
	if (!local_data)
		return false;

	vec3_t target_angle = g_math->aim_direction(local_data->m_eye_pos, target->m_aim_point);
	
	float fov = calculate_fov(view_angle, target_angle);

	// Trigger FOV is usually smaller than aim FOV
	float trigger_fov = g_cfg->legit_bot.m_trigger_fov > 0.f ? 
		g_cfg->legit_bot.m_trigger_fov : 1.0f;

	return fov <= trigger_fov;
}

bool c_legit_bot::can_shoot(c_base_player_weapon* weapon)
{
	if (!weapon)
		return false;

	if (weapon->m_clip1() <= 0)
		return false;

	auto local_controller = g_ctx->m_local_controller;
	if (!local_controller)
		return false;

	// Check if weapon is ready to fire
	if (weapon->m_next_primary_attack() > local_controller->m_tick_base())
		return false;

	return true;
}

void c_legit_bot::process_trigger(c_user_cmd* user_cmd)
{
	if (!m_current_target || !m_current_target->m_pawn)
		return;

	vec3_t current_angle = g_interfaces->m_csgo_input->get_view_angles();

	// Check if crosshair is on target
	if (!is_on_target(current_angle, m_current_target))
	{
		m_trigger_ready = false;
		return;
	}

	// Apply trigger delay
	auto current_time = std::chrono::steady_clock::now();
	
	if (!m_trigger_ready)
	{
		m_trigger_delay_start = current_time;
		m_trigger_ready = true;
		return;
	}

	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		current_time - m_trigger_delay_start).count();

	if (elapsed < g_cfg->legit_bot.m_trigger_delay)
		return;

	// Shoot!
	user_cmd->m_button_state.m_button_state |= IN_ATTACK;
	
	m_trigger_ready = false;
	m_last_shot_time = current_time;
}

// ============================================================================
// Utility
// ============================================================================

void c_legit_bot::reset()
{
	m_current_target = nullptr;
	m_is_smoothing = false;
	m_smooth_progress = 0.f;
	m_reaction_delay_active = false;
	m_trigger_ready = false;
	m_random_offset_x = 0.f;
	m_random_offset_y = 0.f;
}
