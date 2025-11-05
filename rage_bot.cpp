#include "rage_bot.hpp"
#include "../../entity_system/entity.hpp"
#include "../movement/movement.hpp"
#include "../../render/render.hpp"

void c_rage_bot::store_records()
{
	if (!g_interfaces->m_engine->is_in_game() || !g_interfaces->m_engine->is_connected())
		return;

	if (!g_ctx->m_local_pawn)
		return;

	static auto sv_maxunlag = g_interfaces->m_var->get_by_name("sv_maxunlag");
	const int max_ticks = TIME_TO_TICKS(sv_maxunlag->get_float());

	const auto& entity_list = g_entity_system->get("CCSPlayerController");
	for (int i = 0; i < entity_list.size(); ++i) 
	{
		auto entity = entity_list[i];

		auto player_controller = reinterpret_cast<c_cs_player_controller*>(entity);
		if (!player_controller || player_controller == g_ctx->m_local_controller)
			continue;

		auto handle = player_controller->get_handle().to_int();

		if (!player_controller->m_pawn_is_alive())
		{
			auto player_iterator = m_lag_records.find(handle);
			if (player_iterator != m_lag_records.end())
				m_lag_records.erase(player_iterator);

			continue;
		}

		auto player_pawn = reinterpret_cast<c_cs_player_pawn*>(g_interfaces->m_entity_system->get_base_entity(player_controller->m_pawn().get_entry_index()));
		if (!player_pawn)
		{
			auto player_iterator = m_lag_records.find(handle);
			if (player_iterator != m_lag_records.end())
				m_lag_records.erase(player_iterator);

			continue;
		}

		if (player_pawn->m_team_num() == g_ctx->m_local_pawn->m_team_num())
			continue;

		if (player_pawn == g_ctx->m_local_pawn)
			continue;

		if (m_lag_records.find(handle) == m_lag_records.end()) {
			m_lag_records.insert_or_assign(handle, std::deque<lag_record_t>{});

			continue;
		}

		auto& records = m_lag_records[handle];

		if (records.size() != max_ticks) 
		{
			records.clear();
			records.resize(max_ticks);
		}

		auto& new_record = records.emplace_front();
		new_record.store(player_pawn);

		if (records.size() > max_ticks - 1)
			records.erase(records.end() - 1);
	}
}

void c_rage_bot::store_hitboxes()
{
	if (g_cfg->rage_bot.m_hitboxes[0])
		m_hitboxes.emplace_back(HITBOX_HEAD);

	if (g_cfg->rage_bot.m_hitboxes[1]) 
	{
		m_hitboxes.emplace_back(HITBOX_CHEST);
		m_hitboxes.emplace_back(HITBOX_LOWER_CHEST);
		m_hitboxes.emplace_back(HITBOX_UPPER_CHEST);
	}

	if (g_cfg->rage_bot.m_hitboxes[2]) 
	{
		m_hitboxes.emplace_back(HITBOX_PELVIS);
		m_hitboxes.emplace_back(HITBOX_STOMACH);
	}

	if (g_cfg->rage_bot.m_hitboxes[3]) 
	{
		m_hitboxes.emplace_back(HITBOX_RIGHT_HAND);
		m_hitboxes.emplace_back(HITBOX_LEFT_HAND);
		m_hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
		m_hitboxes.emplace_back(HITBOX_RIGHT_FOREARM);
		m_hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);
		m_hitboxes.emplace_back(HITBOX_LEFT_FOREARM);
	}

	if (g_cfg->rage_bot.m_hitboxes[4]) 
	{
		m_hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
		m_hitboxes.emplace_back(HITBOX_LEFT_THIGH);

		m_hitboxes.emplace_back(HITBOX_RIGHT_CALF);
		m_hitboxes.emplace_back(HITBOX_LEFT_CALF);
	}

	if (g_cfg->rage_bot.m_hitboxes[5]) 
	{
		m_hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
		m_hitboxes.emplace_back(HITBOX_LEFT_FOOT);
	}
}

lag_record_t* c_rage_bot::select_record(int handle) 
{
	if (!g_ctx->m_local_pawn->is_alive())
		return nullptr;

	auto& records = m_lag_records[handle];

	if (records.empty())
		return nullptr;

	lag_record_t* best_record = nullptr;
	lag_record_t* last_record = &records.front();

	if (records.size() == 1u)
		return last_record;

	for (auto i = records.begin(); i != records.end(); i = std::next(i)) 
	{
		auto record = &*i;

		if (!record->m_pawn || !record->is_valid())
			continue;

		if (!record || !record->is_valid())
			continue;

		if (!best_record) 
		{
			best_record = record;

			continue;
		}

		if (record->m_throwing)
		{
			best_record = record;

			continue;
		}

		if ((record->m_pawn->m_flags() & FL_ONGROUND) != (record->m_pawn->m_flags() & FL_ONGROUND))
		{
			if ((record->m_pawn->m_flags() & FL_ONGROUND))
				best_record = record;
		}
	}

	return best_record;
}

void c_rage_bot::find_targets() 
{
	if (!g_ctx->m_local_pawn->is_alive())
		return;

	auto entities = g_entity_system->get("CCSPlayerController");

	for (auto entity : entities)
	{
		if (!entity)
			continue;

		auto player_controller = reinterpret_cast<c_cs_player_controller*>(entity);
		if (!player_controller || player_controller == g_ctx->m_local_controller)
			continue;

		int handle = player_controller->get_handle().to_int();

		lag_record_t* best_record = this->select_record(handle);

		if (!best_record)
			continue;

		auto target_ptr = m_aim_targets.find(handle);

		if (target_ptr == m_aim_targets.end()) 
		{
			m_aim_targets.insert_or_assign(handle, aim_target_t{});
			continue;
		}

		target_ptr->second = aim_target_t(best_record);
	}
}

aim_target_t* c_rage_bot::get_nearest_target() {
	if (!g_ctx->m_local_pawn->is_alive())
		return nullptr;

	aim_target_t* best_target{};

	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return nullptr;

	vec3_t shoot_pos = local_data->m_eye_pos;
	vec3_t view_angles = g_interfaces->m_csgo_input->get_view_angles();


	vec3_t aim_direction;
	g_math->angle_vectors(view_angles, aim_direction);

	float best_distance = FLT_MAX;

	for (auto it = m_aim_targets.begin(); it != m_aim_targets.end(); it = std::next(it)) {
		aim_target_t* target = &it->second;

		if (!target->m_lag_record
			|| !target->m_pawn
			|| !target->m_pawn->is_player_pawn()
			|| !target->m_pawn->is_alive()
			|| !target->m_pawn->m_scene_node()) {
			continue;
		}

		vec3_t target_pos = target->m_pawn->m_scene_node()->m_origin();
		vec3_t to_target = target_pos - shoot_pos;

		float distance = to_target.length();
		vec3_t normalized_to_target = to_target / distance;

		float dot_product = aim_direction.dot(normalized_to_target);
		float distance_score = distance * (1.0f - dot_product);

		if (distance_score < best_distance) {
			best_distance = distance_score;
			best_target = target;
		}
	}

	return best_target;
}

float c_rage_bot::calc_point_scale(vec3_t& point, const float& hitbox_radius)
{
	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return 0.f;

	float spread = local_data->m_spread + local_data->m_inaccuracy;
	float distance = point.dist(local_data->m_eye_pos);

	float new_distance = distance / std::sin(deg2rad(90.f - rad2deg(spread)));
	float scale = ((hitbox_radius - new_distance * spread) + 0.1f) * 100.f;

	return std::clamp(scale, 0.f, 95.f);
}

vec3_t c_rage_bot::transform_point(matrix3x4_t matrix, vec3_t point)
{
	vec3_t result{ };
	g_math->vector_transform(point, matrix, result);
	return result;
}

std::vector<vec3_t> c_rage_bot::calculate_sphere_points(float radius, int num_points)
{
	std::vector<vec3_t> points;
	points.reserve(num_points);

	const float phi = _pi * (3.0f - std::sqrt(5.0f));

	for (int i = 0; i < num_points; ++i)
	{
		float y = 1 - (i / float(num_points - 1)) * 2;
		float radius_at_y = std::sqrt(1 - y * y);

		float theta = phi * i;

		float x = std::cos(theta) * radius_at_y;
		float z = std::sin(theta) * radius_at_y;

		vec3_t vec_point{};
		points.push_back({ x * radius, y * radius, z * radius });
	}

	return points;
}

std::vector<vec3_t> c_rage_bot::calculate_points(int num_points, float radius, float height, matrix3x4_t matrix, hitbox_data_t hitbox)
{
	std::vector<vec3_t> points;
	points.reserve(num_points);

	for (int i = 0; i < num_points; ++i)
	{
		float theta = 2.f * _pi * i / num_points;
		float y = radius * std::cos(theta);
		float z = radius * std::sin(theta);

		vec3_t in_point = { hitbox.m_mins.x + hitbox.m_radius * height, y, z };

		vec3_t vec_point{};
		g_math->vector_transform(in_point, matrix, vec_point);
		points.push_back(vec_point);
	}

	return points;
}

bool c_rage_bot::multi_points(lag_record_t* record, int hitbox, std::vector<aim_point_t>& points)
{
	if (!record)
		return false;

	c_cs_player_pawn* pawn = record->m_pawn;

	if (!pawn || !pawn->is_player_pawn() || !pawn->is_alive())
		return false;

	hitbox_data_t hitbox_data = get_hitbox_data(pawn, hitbox);

	if (hitbox_data.m_invalid_data)
		return false;

	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return false;

	vec3_t center = (hitbox_data.m_mins + hitbox_data.m_maxs) * 0.5f;

	matrix3x4_t matrix = g_math->transform_to_matrix(record->m_bone_data[hitbox_data.m_num_bone]);

	vec3_t point = transform_point(matrix, center);

	int hitbox_from_menu = this->get_hitbox_from_menu(hitbox);

	if (hitbox_from_menu == -1)
		return false;

	points.emplace_back(point, hitbox, true);

	if (!g_cfg->rage_bot.m_multi_points[hitbox_from_menu])
		return true;

	float scale = g_cfg->rage_bot.m_hitbox_scale[hitbox_from_menu];

	if (!g_cfg->rage_bot.m_static_scale)
		scale = calc_point_scale(center, hitbox_data.m_radius);

	float radius = hitbox_data.m_radius * (scale / 100.f);

	if (radius <= 0.f)
		return false;

	if (hitbox == HITBOX_HEAD) 
	{
		auto sphere_points = calculate_sphere_points(radius, 15);

		if (sphere_points.empty())
			return false;

		for (const auto& point : sphere_points)
		{
			auto point_pos = transform_point(matrix, { hitbox_data.m_maxs.x + point.x, hitbox_data.m_maxs.y + point.y, hitbox_data.m_maxs.z + point.z });
			points.emplace_back(point_pos, hitbox);
		}

		return true;
	}

	auto capsule_points = calculate_points(4, radius, 0.5f, matrix, hitbox_data);

	if (capsule_points.empty())
		return false;

	for (const auto& point : capsule_points)
		points.emplace_back(point, hitbox);

	return true;
}

aim_point_t c_rage_bot::select_points(lag_record_t* record, float& damage)
{
	std::vector<aim_point_t> points{};
	aim_point_t best_point{ vec3_t(0.f, 0.f, 0.f), -1 };

	if (!g_ctx->m_local_pawn->is_alive())
		return best_point;

	if (!record->m_pawn || !record->m_pawn->is_alive())
		return best_point;

	c_base_player_weapon* weapon = g_ctx->m_local_pawn->get_active_weapon();

	c_cs_weapon_base_v_data* weapon_data = weapon->get_weapon_data();

	if (!weapon_data)
		return best_point;

	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return best_point;

	vec3_t shoot_pos = local_data->m_eye_pos;

	bool is_taser = weapon_data->m_weapon_type() == WEAPONTYPE_TASER;

	float best_damage = 0.f;
	int prev_hitbox = -1;

	for (auto& hitbox : m_hitboxes)
	{
		std::vector<aim_point_t> points{};
		points.reserve(68);

		if (!multi_points(record, hitbox, points))
			continue;

		for (auto& point : points) {
			penetration_data_t pen_data{};

			if (!g_auto_wall->fire_bullet(shoot_pos, point.m_point, record->m_pawn, weapon_data, pen_data, is_taser))
				continue;

			vec3_t point_angle;
			float point_damage = pen_data.m_damage;

			if (point_damage > best_damage && prev_hitbox != hitbox || point_damage > best_damage + 30.f)
			{
				best_point = point;
				best_damage = point_damage;
				prev_hitbox = hitbox;
			}
		}
	}

	damage = best_damage;
	return best_point;
}

void c_rage_bot::select_target()
{
	if (!g_ctx->m_local_pawn->is_alive())
		return;

	if (m_aim_targets.empty())
		return;

	aim_target_t* best_target = this->get_nearest_target();

	if (!best_target || !best_target->m_lag_record || !best_target->m_pawn->is_alive())
		return;

	bool is_taser = g_ctx->m_local_pawn->get_active_weapon()->get_weapon_data()->m_weapon_type() == WEAPONTYPE_TASER;

	int min_damage = min(g_cfg->rage_bot.m_minimum_damage, best_target->m_lag_record->m_pawn->m_health());

	if (g_key_handler->is_pressed(g_cfg->rage_bot.m_override_damage_key_bind, g_cfg->rage_bot.m_override_damage_key_bind_style))
		min_damage = min(g_cfg->rage_bot.m_minimum_damage_override, best_target->m_lag_record->m_pawn->m_health());

	if (is_taser)
		min_damage = best_target->m_lag_record->m_pawn->m_health();

	best_target->m_lag_record->apply(best_target->m_pawn);

	float best_damage = 0.f;
	aim_point_t best_point = this->select_points(best_target->m_lag_record, best_damage);

	best_target->m_lag_record->reset(best_target->m_pawn);

	if (best_point.m_hitbox == -1 || best_damage < min_damage)
		return;

	best_target->m_best_point = std::make_unique<aim_point_t>(best_point);
	m_best_target = best_target;
}

bool c_rage_bot::weapon_is_at_max_accuracy(c_cs_weapon_base_v_data* weapon_data, float inaccuracy)
{
	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return false;

	constexpr auto round_accuracy = [](float accuracy) { return floorf(accuracy * 170.f) / 170.f; };
	constexpr auto round_duck_accuracy = [](float accuracy) { return floorf(accuracy * 300.f) / 300.f; };

	float speed = g_ctx->m_local_pawn->m_vec_abs_velocity().length();

	bool is_scoped = ((weapon_data->m_weapon_type() == WEAPONTYPE_SNIPER_RIFLE)
		&& !(g_ctx->m_user_cmd->m_button_state.m_button_state & IN_ZOOM)
		&& !g_ctx->m_local_pawn->m_scoped());

	bool is_ducking = ((g_ctx->m_local_pawn->m_flags() & FL_DUCKING) || (g_ctx->m_user_cmd->m_button_state.m_button_state & IN_DUCK));

	float rounded_accuracy = round_accuracy(inaccuracy);
	float rounded_duck_accuracy = round_duck_accuracy(inaccuracy);

	if (is_ducking && is_scoped && (g_ctx->m_local_pawn->m_flags() & FL_ONGROUND) && (rounded_duck_accuracy < local_data->m_inaccuracy))
		return true;

	if (speed <= 0 && is_scoped && (g_ctx->m_local_pawn->m_flags() & FL_ONGROUND) && (rounded_accuracy < local_data->m_inaccuracy))
		return true;

	return false;
}

vec3_t c_rage_bot::calculate_spread_angles(vec3_t angle, int random_seed, float weapon_inaccuarcy, float weapon_spread)
{
	float r1, r2, r3, r4, s1, c1, s2, c2;

	g_interfaces->m_random_seed(random_seed + 1);

	r1 = g_interfaces->m_random_float(0.f, 1.f);
	r2 = g_interfaces->m_random_float(0.f, _pi2);
	r3 = g_interfaces->m_random_float(0.f, 1.f);
	r4 = g_interfaces->m_random_float(0.f, _pi2);

	c1 = std::cos(r2);
	c2 = std::cos(r4);
	s1 = std::sin(r2);
	s2 = std::sin(r4);

	vec3_t spread = {
		(c1 * (r1 * weapon_inaccuarcy)) + (c2 * (r3 * weapon_spread)),
		(s1 * (r1 * weapon_inaccuarcy)) + (s2 * (r3 * weapon_spread))
	};

	vec3_t vec_forward, vec_right, vec_up;
	g_math->angle_vectors(angle, vec_forward, vec_right, vec_up);

	return (vec_forward + (vec_right * spread.x) + (vec_up * spread.y)).normalize();
}

int c_rage_bot::calculate_hit_chance(c_cs_player_pawn* pawn, vec3_t angles, c_base_player_weapon* active_weapon, c_cs_weapon_base_v_data* weapon_data, bool no_spread)
{
	if (!g_ctx->m_local_pawn->is_alive())
		return 0;

	if (!pawn)
		return 0;

	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return 0;

	if (no_spread)
		return 100;

	vec3_t shoot_pos = local_data->m_eye_pos;

	if (shoot_pos.dist(angles) > weapon_data->m_range())
		return 0;

	active_weapon->update_accuracy_penality();

	if (weapon_is_at_max_accuracy(weapon_data, active_weapon->get_inaccuracy()))
		return 100;

	const float spread = local_data->m_spread;
	const float inaccuracy = local_data->m_inaccuracy;

	int hits = 0;

	for (int seed = 0; seed < 570; seed++)
	{
		vec3_t spread_angle = calculate_spread_angles(angles, seed, inaccuracy, spread);

		vec3_t result = spread_angle * weapon_data->m_range() + shoot_pos;

		ray_t ray{};
		game_trace_t trace{};
		trace_filter_t filter{};
		g_interfaces->m_trace->init_trace(filter, g_ctx->m_local_pawn, MASK_SHOT, 0x3, 0x7);

		g_interfaces->m_trace->trace_shape(&ray, shoot_pos, result, &filter, &trace);
		g_interfaces->m_trace->clip_ray_entity(&ray, shoot_pos, result, pawn, &filter, &trace);

		if (trace.m_hit_entity && trace.m_hit_entity->is_player_pawn() && trace.m_hit_entity->get_handle().get_entry_index() == pawn->get_handle().get_entry_index())
			hits++;
	}

	return static_cast<int>((static_cast<float>(hits / 570.f)) * 100.f);
}

void c_rage_bot::process_backtrack(lag_record_t* record)
{
	if (!record)
		return;

	const int tick_shoot = TIME_TO_TICKS(record->m_simulation_time) + 2;

	for (int i = 0; i < g_ctx->m_user_cmd->pb.input_history_size(); i++) {

		if (g_ctx->m_user_cmd->pb.attack1_start_history_index() == -1)
			continue;

		auto input_history = g_ctx->m_user_cmd->pb.mutable_input_history(i);
		if (!input_history)
			continue;

		if (input_history->has_cl_interp()) {
			auto interp_info = input_history->mutable_cl_interp();
			interp_info->set_frac(0.f);
		}

		if (input_history->has_sv_interp0()) {
			auto interp_info = input_history->mutable_sv_interp0();
			interp_info->set_src_tick(tick_shoot);
			interp_info->set_dst_tick(tick_shoot);
			interp_info->set_frac(0.f);
		}

		if (input_history->has_sv_interp1()) {
			auto interp_info = input_history->mutable_sv_interp1();
			interp_info->set_src_tick(tick_shoot);
			interp_info->set_dst_tick(tick_shoot);
			interp_info->set_frac(0.f);
		}

		input_history->set_render_tick_count(tick_shoot);

		g_ctx->m_user_cmd->pb.mutable_base()->set_client_tick(tick_shoot);

		input_history->set_player_tick_count(g_ctx->m_local_controller->m_tick_base());
		input_history->set_player_tick_fraction(g_prediction->get_local_data()->m_player_tick_fraction);
	}
}

bool c_rage_bot::can_shoot(c_cs_player_pawn* pawn, c_base_player_weapon* active_weapon)
{
	return (active_weapon->m_clip1() > 0) && (active_weapon->m_next_primary_attack() <= g_ctx->m_local_controller->m_tick_base() + 2);
}

void c_rage_bot::process_attack(c_user_cmd* user_cmd, vec3_t angle)
{
	user_cmd->pb.mutable_base()->mutable_viewangles()->set_x(179.f);

	for (int i = 0; i < g_ctx->m_user_cmd->pb.input_history_size(); i++)
	{
		auto container = g_ctx->m_user_cmd->pb.mutable_input_history(i);
		if (container)
			container->set_player_tick_count(g_prediction->get_local_data()->m_shoot_tick);
	}

	user_cmd->pb.set_attack1_start_history_index(0);
	process_backtrack(m_best_target->m_lag_record);

	if (g_cfg->rage_bot.m_auto_fire)
		user_cmd->m_button_state.m_button_state |= IN_ATTACK;
}

// ============== MISS LOG IMPLEMENTATION ==============

void c_rage_bot::register_shot(const shot_info_t& shot_info)
{
	m_last_shot = shot_info;
	m_waiting_for_impact = true;
	m_last_shot_time = g_interfaces->m_globals->m_curtime;
	
	// Keep last 64 shots in history
	m_shot_records.push_back(shot_info);
	if (m_shot_records.size() > 64)
		m_shot_records.erase(m_shot_records.begin());
}

void c_rage_bot::on_bullet_impact(const vec3_t& impact_pos)
{
	if (!m_waiting_for_impact)
		return;
		
	m_last_impact_position = impact_pos;
	m_waiting_for_impact = false;
	
	// Check if we missed
	float distance_to_aim_point = m_last_impact_position.dist(m_last_shot.m_aim_point);
	
	// If impact is more than 64 units away from aim point, consider it a miss
	if (distance_to_aim_point > 64.f) {
		process_miss(m_last_shot, impact_pos);
	}
}

void c_rage_bot::on_player_hurt(int victim_handle, int attacker_handle, int damage, int hitgroup)
{
	if (!g_ctx->m_local_controller)
		return;
		
	int local_handle = g_ctx->m_local_controller->get_handle().to_int();
	
	if (attacker_handle != local_handle)
		return;
		
	// We hit someone
	if (m_last_shot.m_target_handle == victim_handle) {
		m_waiting_for_impact = false;
		
		// Check if we hit the intended hitbox
		if (m_last_shot.m_hitbox != hitgroup) {
			// We hit, but wrong hitbox - still log as miss
			miss_log_t miss;
			miss.m_shot_info = m_last_shot;
			miss.m_reason = MISS_REASON_SPREAD;
			miss.m_reason_string = "Hit wrong hitbox due to spread";
			miss.m_time_logged = g_interfaces->m_globals->m_curtime;
			miss.m_hit_group = hitgroup;
			miss.m_did_hit = true;
			
			m_miss_logs.push_back(miss);
			
			// Print to console
			if (g_cfg->rage_bot.m_log_misses) {
				std::string log = "[SPREAD MISS] Target: " + m_last_shot.m_target_name +
					" | Aimed: " + get_hitbox_name(m_last_shot.m_hitbox) +
					" | Hit: " + get_hitbox_name(hitgroup) +
					" | HC: " + std::to_string(m_last_shot.m_hit_chance) + "%" +
					" | Spread: " + std::to_string(static_cast<int>(m_last_shot.m_spread * 1000.f)) +
					" | Inaccuracy: " + std::to_string(static_cast<int>(m_last_shot.m_inaccuracy * 1000.f));
				
				g_console->print(log.c_str());
			}
		}
	}
}

void c_rage_bot::process_miss(const shot_info_t& shot_info, const vec3_t& impact_pos)
{
	miss_log_t miss;
	miss.m_shot_info = shot_info;
	miss.m_hit_position = impact_pos;
	miss.m_time_logged = g_interfaces->m_globals->m_curtime;
	miss.m_did_hit = false;
	
	miss.m_reason = determine_miss_reason(shot_info, impact_pos);
	miss.m_reason_string = get_reason_string(miss.m_reason);
	
	m_miss_logs.push_back(miss);
	
	// Keep only last 32 misses
	if (m_miss_logs.size() > 32)
		m_miss_logs.erase(m_miss_logs.begin());
	
	// Print to console
	if (g_cfg->rage_bot.m_log_misses) {
		float distance = impact_pos.dist(shot_info.m_aim_point);
		
		std::string log = "[MISS] Target: " + shot_info.m_target_name +
			" | Hitbox: " + get_hitbox_name(shot_info.m_hitbox) +
			" | Reason: " + miss.m_reason_string +
			" | HC: " + std::to_string(shot_info.m_hit_chance) + "%" +
			" | Spread: " + std::to_string(static_cast<int>(shot_info.m_spread * 1000.f)) +
			" | Inaccuracy: " + std::to_string(static_cast<int>(shot_info.m_inaccuracy * 1000.f)) +
			" | Distance from point: " + std::to_string(static_cast<int>(distance)) + " units";
		
		g_console->print(log.c_str());
	}
}

miss_reason_e c_rage_bot::determine_miss_reason(const shot_info_t& shot_info, const vec3_t& impact_pos)
{
	float distance = impact_pos.dist(shot_info.m_aim_point);
	
	// If spread was high and we missed by a lot, it's spread
	if (shot_info.m_spread > 0.01f || shot_info.m_inaccuracy > 0.01f) {
		if (distance > 32.f)
			return MISS_REASON_SPREAD;
	}
	
	// If the impact was close but still missed, could be prediction
	if (distance < 64.f)
		return MISS_REASON_PREDICTION_ERROR;
	
	// If hitbox was head and we missed, could be resolver
	if (shot_info.m_hitbox == HITBOX_HEAD)
		return MISS_REASON_RESOLVER;
	
	return MISS_REASON_SPREAD;
}

std::string c_rage_bot::get_hitbox_name(int hitbox)
{
	switch (hitbox) {
		case HITBOX_HEAD: return "Head";
		case HITBOX_NECK: return "Neck";
		case HITBOX_CHEST: return "Chest";
		case HITBOX_STOMACH: return "Stomach";
		case HITBOX_PELVIS: return "Pelvis";
		case HITBOX_UPPER_CHEST: return "Upper Chest";
		case HITBOX_LOWER_CHEST: return "Lower Chest";
		case HITBOX_LEFT_THIGH: return "Left Thigh";
		case HITBOX_RIGHT_THIGH: return "Right Thigh";
		case HITBOX_LEFT_CALF: return "Left Calf";
		case HITBOX_RIGHT_CALF: return "Right Calf";
		case HITBOX_LEFT_FOOT: return "Left Foot";
		case HITBOX_RIGHT_FOOT: return "Right Foot";
		case HITBOX_LEFT_HAND: return "Left Hand";
		case HITBOX_RIGHT_HAND: return "Right Hand";
		case HITBOX_LEFT_UPPER_ARM: return "Left Upper Arm";
		case HITBOX_RIGHT_UPPER_ARM: return "Right Upper Arm";
		case HITBOX_LEFT_FOREARM: return "Left Forearm";
		case HITBOX_RIGHT_FOREARM: return "Right Forearm";
		default: return "Unknown";
	}
}

std::string c_rage_bot::get_reason_string(miss_reason_e reason)
{
	switch (reason) {
		case MISS_REASON_SPREAD: return "Spread/Inaccuracy";
		case MISS_REASON_PREDICTION_ERROR: return "Prediction Error";
		case MISS_REASON_RESOLVER: return "Resolver";
		case MISS_REASON_HITBOX: return "Hitbox";
		default: return "Unknown";
	}
}

// ============== END MISS LOG IMPLEMENTATION ==============

void c_rage_bot::on_create_move()
{
	if (!g_cfg->rage_bot.m_enabled)
		return;

	if (!g_interfaces->m_engine->is_in_game())
		return;

	if (!m_hitboxes.empty())
		m_hitboxes.clear();

	if (m_best_target)
		m_best_target->reset();

	c_user_cmd* user_cmd = g_ctx->m_user_cmd;

	if (!user_cmd)
		return;

	c_cs_player_pawn* local_player = g_ctx->m_local_pawn;

	if (!local_player || !local_player->is_alive())
		return;

	c_base_player_weapon* active_weapon = local_player->get_active_weapon();

	if (!active_weapon)
		return;

	c_cs_weapon_base_v_data* weapon_data = active_weapon->get_weapon_data();

	if (!weapon_data
		|| weapon_data->m_weapon_type() == WEAPONTYPE_KNIFE
		|| weapon_data->m_weapon_type() == WEAPONTYPE_GRENADE)
		return;

	store_hitboxes();

	if (m_hitboxes.empty())
		return;

	find_targets();

	select_target();

	if (!m_best_target || !m_best_target->m_best_point)
		return;

	auto local_data = g_prediction->get_local_data();

	if (!local_data)
		return;

	if (!can_shoot(local_player, active_weapon))
		return;

	bool no_spread = g_interfaces->m_var->get_by_name(xorstr_("weapon_accuracy_nospread"))->get_bool();

	if ((weapon_data->m_weapon_type() == WEAPONTYPE_SNIPER_RIFLE)
		&& !(user_cmd->m_button_state.m_button_state & IN_ZOOM)
		&& !local_player->m_scoped()
		&& (local_player->m_flags() & FL_ONGROUND)
		&& !no_spread) {
		user_cmd->m_button_state.m_button_state |= IN_ATTACK2;
	}

	g_movement->auto_stop(user_cmd, local_player, active_weapon, no_spread);

	bool is_taser = weapon_data->m_weapon_type() == WEAPONTYPE_TASER;

	m_best_target->m_lag_record->apply(m_best_target->m_pawn);

	vec3_t angle = g_math->aim_direction(local_data->m_eye_pos, m_best_target->m_best_point->m_point);

	vec3_t best_angle = angle - get_removed_aim_punch_angle(local_player);

	int hit_chance = calculate_hit_chance(m_best_target->m_pawn, best_angle, active_weapon, weapon_data, no_spread);

	if (hit_chance >= (is_taser ? 70 : g_cfg->rage_bot.m_hit_chance)) {
		for (int i = 0; i < user_cmd->pb.input_history_size(); i++) {
			auto tick = user_cmd->pb.mutable_input_history(i);
			if (tick) {
				tick->mutable_view_angles()->set_x(best_angle.x);
				tick->mutable_view_angles()->set_y(best_angle.y);
				tick->mutable_view_angles()->set_z(best_angle.z);
			}
		}

		if (!g_cfg->rage_bot.m_silent)
			g_interfaces->m_csgo_input->set_view_angles(best_angle);

		// Register shot for miss logging
		if (g_cfg->rage_bot.m_log_misses) {
			shot_info_t shot;
			shot.m_target_handle = m_best_target->m_pawn->get_handle().to_int();
			shot.m_target_name = m_best_target->m_pawn->get_player_name();
			shot.m_shoot_position = local_data->m_eye_pos;
			shot.m_aim_point = m_best_target->m_best_point->m_point;
			shot.m_aim_angles = best_angle;
			shot.m_hitbox = m_best_target->m_best_point->m_hitbox;
			shot.m_damage = 0; // Will be filled if we get damage info
			shot.m_hit_chance = hit_chance;
			shot.m_spread = local_data->m_spread;
			shot.m_inaccuracy = local_data->m_inaccuracy;
			shot.m_tick_count = g_ctx->m_local_controller->m_tick_base();
			shot.m_simulation_time = m_best_target->m_lag_record->m_simulation_time;
			shot.m_no_spread = no_spread;
			
			register_shot(shot);
		}

		process_attack(user_cmd, best_angle);
	}

	m_best_target->m_lag_record->reset(m_best_target->m_pawn);
}
