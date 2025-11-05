#pragma once
#include "precompiled.h"

class c_movement {
public:
	void on_create_move(c_user_cmd* user_cmd);
	
	// Basic movement
	void bunnyhop(c_user_cmd* user_cmd);
	void auto_strafe(vec3_t vVelocity, float flCurrentSpeed, float flFrameTime, bool bSubtickCalling);
	void subtick_strafer();
	
	// Advanced movement
	void jump_bug(c_user_cmd* user_cmd);
	void edge_jump(c_user_cmd* user_cmd);
	
	// Speed control
	void limit_speed(c_user_cmd* user_cmd, c_cs_player_pawn* local_player, c_base_player_weapon* active_weapon, float max_speed);
	void auto_stop(c_user_cmd* user_cmd, c_cs_player_pawn* local_player, c_base_player_weapon* active_weapon, bool no_spread);
	void stop_movement();
	
	// Physics simulation
	float apply_friction(vec3_t& vVecVelocity);
	vec3_t simulate_pos_at_stop(vec3_t vPos);
	void air_accelerate(vec3_t& vVelocity, vec3_t vMoveImpulse, float& flStamina, float flFrameTime, float flYaw);
	void parachute_accelerate(vec3_t& out_velo, float& stamina, float& friction, vec3_t move, float frametime);
	
	// Movement correction
	void movement_fix();
	void fix_cmd_buttons(const bool calling_from_strafer);
	
	// Movement control
	void handle_move_stop(bool bHaltMovement);
	void handle_directional_strafe();
	
	// Validation
	bool is_valid_movement_trace(GameTrace_t& tr, BBox_t bounds, TraceFilter_t* filter);
	
	// Angle management
	vec3_t m_ang_model_angles;
	vec3_t m_ang_camera_angles;
	bool m_wants_2d_move_halt;
	
private:
	// Internal helpers
	void directional_air_strafe(vec3_t vVelocity, float flCurrentSpeed, float flFrameTime, bool bSubtickCalling);
};

inline c_movement* g_movement = new c_movement();
