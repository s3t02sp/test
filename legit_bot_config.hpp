#pragma once

// Example configuration structure for the legitbot
// Add this to your existing config system

struct legit_bot_config_t
{
	// Main settings
	bool m_enabled = false;
	bool m_always_on = false; // If false, requires key bind
	
	// Key binds (adjust to your key handler system)
	int m_aim_key_bind = 0; // Key to activate aim (e.g., mouse button)
	int m_aim_key_bind_style = 0; // Hold/Toggle/etc
	
	// FOV settings
	float m_fov = 5.0f; // Field of view for target selection (degrees)
	
	// Smooth aiming
	float m_smooth = 10.0f; // Smoothness factor (1-100, higher = slower/smoother)
	
	// Target selection
	int m_target_hitbox = 6; // HITBOX_HEAD = 6 (adjust based on your hitbox enum)
	bool m_team_check = true; // Don't target teammates
	bool m_visibility_check = true; // Only target visible enemies
	
	// Recoil control
	bool m_recoil_control = false;
	float m_recoil_control_amount = 50.0f; // 0-100%
	
	// Trigger bot
	bool m_trigger_bot = false;
	float m_trigger_delay = 50.0f; // Delay before shooting (ms)
	float m_trigger_fov = 0.5f; // FOV for trigger activation (smaller than aim FOV)
	
	// Humanizer
	bool m_humanizer = true;
	float m_reaction_time = 100.0f; // Reaction time delay (ms)
	float m_aim_shake = 0.05f; // Random aim shake amount (0-1)
	
	// Additional settings
	bool m_auto_scope = false; // Auto scope for sniper rifles
	bool m_auto_stop = false; // Stop movement for better accuracy
};

// Example of how to add to your global config
// inline legit_bot_config_t g_legit_bot_cfg;
