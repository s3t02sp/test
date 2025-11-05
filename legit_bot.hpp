#pragma once
#include <vector>
#include <memory>
#include <chrono>

// Forward declarations (adjust based on your project structure)
class c_cs_player_pawn;
class c_cs_player_controller;
class c_base_player_weapon;
class c_cs_weapon_base_v_data;
class c_user_cmd;
struct vec3_t;

// Legitbot target structure
struct legit_target_t 
{
	c_cs_player_pawn* m_pawn = nullptr;
	c_cs_player_controller* m_controller = nullptr;
	vec3_t m_aim_point = vec3_t(0.f, 0.f, 0.f);
	int m_hitbox = -1;
	float m_fov = 0.f;
	float m_distance = 0.f;

	legit_target_t() = default;
	legit_target_t(c_cs_player_pawn* pawn, c_cs_player_controller* controller) 
		: m_pawn(pawn), m_controller(controller) {}

	void reset() 
	{
		m_pawn = nullptr;
		m_controller = nullptr;
		m_aim_point = vec3_t(0.f, 0.f, 0.f);
		m_hitbox = -1;
		m_fov = 0.f;
		m_distance = 0.f;
	}
};

class c_legit_bot 
{
private:
	// Current best target
	legit_target_t* m_current_target = nullptr;

	// Smooth aim data
	vec3_t m_smooth_start_angle = vec3_t(0.f, 0.f, 0.f);
	vec3_t m_smooth_target_angle = vec3_t(0.f, 0.f, 0.f);
	float m_smooth_progress = 0.f;
	bool m_is_smoothing = false;

	// Humanizer data
	std::chrono::steady_clock::time_point m_last_target_acquisition_time;
	std::chrono::steady_clock::time_point m_last_shot_time;
	bool m_reaction_delay_active = false;
	float m_random_offset_x = 0.f;
	float m_random_offset_y = 0.f;
	int m_humanizer_seed = 0;

	// Trigger bot data
	bool m_trigger_ready = false;
	std::chrono::steady_clock::time_point m_trigger_delay_start;

	// Target tracking
	std::vector<legit_target_t> m_targets;

private:
	// Core functions
	void find_targets();
	legit_target_t* select_best_target();
	
	// FOV and visibility
	float calculate_fov(const vec3_t& view_angle, const vec3_t& aim_angle);
	bool is_visible(c_cs_player_pawn* local_pawn, c_cs_player_pawn* target_pawn, const vec3_t& target_pos);
	
	// Aim point selection
	vec3_t get_hitbox_position(c_cs_player_pawn* pawn, int hitbox);
	vec3_t select_aim_point(legit_target_t* target);
	
	// Smooth aim
	void start_smooth_aim(const vec3_t& current_angle, const vec3_t& target_angle);
	vec3_t calculate_smooth_angle(float delta_time);
	vec3_t apply_smoothing(const vec3_t& current_angle, const vec3_t& target_angle, float smooth_factor);
	
	// Humanizer
	void apply_humanizer();
	float get_reaction_time();
	vec3_t add_human_error(const vec3_t& angle);
	void update_random_offsets();
	
	// Trigger bot
	bool is_on_target(const vec3_t& view_angle, legit_target_t* target);
	bool can_shoot(c_base_player_weapon* weapon);
	void process_trigger(c_user_cmd* user_cmd);
	
	// Recoil control
	vec3_t get_recoil_control_angle(c_cs_player_pawn* local_pawn);

public:
	c_legit_bot() = default;
	~c_legit_bot() = default;

	// Main entry point
	void on_create_move(c_user_cmd* user_cmd);
	
	// Reset state
	void reset();
};

// Global instance
inline c_legit_bot* g_legit_bot = new c_legit_bot();
