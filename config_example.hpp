#pragma once

// Example configuration structure for the miss log system
// Add these to your existing config structure

struct rage_bot_config_t {
    // Existing rage bot settings...
    bool m_enabled = false;
    bool m_auto_fire = false;
    bool m_silent = true;
    int m_hit_chance = 50;
    int m_minimum_damage = 10;
    int m_minimum_damage_override = 5;
    
    // Hitbox selection
    bool m_hitboxes[6] = { true, true, true, false, false, false };
    bool m_multi_points[6] = { true, true, true, true, true, true };
    int m_hitbox_scale[6] = { 50, 50, 50, 50, 50, 50 };
    bool m_static_scale = false;
    
    // Miss log settings
    bool m_log_misses = true;                    // Enable miss logging
    bool m_show_miss_log = true;                 // Show miss log on screen
    bool m_show_3d_miss_log = true;              // Show 3D visualization of misses
    bool m_clear_logs_on_round_start = true;     // Clear logs when round starts
    
    // Miss log display settings
    int m_miss_log_x = 10;                       // X position on screen
    int m_miss_log_y = 300;                      // Y position on screen
    int m_max_displayed_misses = 10;             // Max number of misses to show
    float m_miss_log_fade_time = 10.f;           // Time before miss fades out (seconds)
    
    // Miss log filtering
    bool m_log_spread_misses = true;             // Log misses due to spread
    bool m_log_prediction_misses = true;         // Log misses due to prediction errors
    bool m_log_resolver_misses = true;           // Log misses due to resolver
    bool m_log_wrong_hitbox = true;              // Log hits on wrong hitbox
    
    // Miss log console output
    bool m_print_misses_to_console = true;       // Print misses to console
    bool m_verbose_miss_logging = false;         // Include extra details in console
    
    // Override damage key bind
    int m_override_damage_key_bind = 0;
    int m_override_damage_key_bind_style = 0;
};

// Example usage in your config:
// inline rage_bot_config_t g_rage_bot_config;
