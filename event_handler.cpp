#pragma once
#include "rage_bot.hpp"

// Example implementation of event handlers for miss logging
// Hook these events in your game event listener

class c_event_handler {
public:
    void on_bullet_impact(c_game_event* event) {
        if (!g_cfg->rage_bot.m_enabled || !g_cfg->rage_bot.m_log_misses)
            return;
            
        if (!g_ctx->m_local_controller)
            return;
            
        // Get the user who fired
        int user_id = event->get_int("userid");
        
        // Convert user_id to handle (implementation depends on your game interface)
        int player_handle = get_player_handle_from_userid(user_id);
        
        if (player_handle != g_ctx->m_local_controller->get_handle().to_int())
            return;
            
        // Get impact position
        vec3_t impact_pos;
        impact_pos.x = event->get_float("x");
        impact_pos.y = event->get_float("y");
        impact_pos.z = event->get_float("z");
        
        // Pass to rage bot miss log system
        g_rage_bot->on_bullet_impact(impact_pos);
    }
    
    void on_player_hurt(c_game_event* event) {
        if (!g_cfg->rage_bot.m_enabled || !g_cfg->rage_bot.m_log_misses)
            return;
            
        if (!g_ctx->m_local_controller)
            return;
            
        // Get victim and attacker
        int victim_id = event->get_int("userid");
        int attacker_id = event->get_int("attacker");
        
        // Convert to handles
        int victim_handle = get_player_handle_from_userid(victim_id);
        int attacker_handle = get_player_handle_from_userid(attacker_id);
        
        int damage = event->get_int("dmg_health");
        int hitgroup = event->get_int("hitgroup");
        
        // Pass to rage bot miss log system
        g_rage_bot->on_player_hurt(victim_handle, attacker_handle, damage, hitgroup);
    }
    
    void on_round_start(c_game_event* event) {
        // Optional: Clear miss logs on round start
        if (g_cfg->rage_bot.m_clear_logs_on_round_start)
            g_rage_bot->clear_miss_logs();
    }
    
private:
    int get_player_handle_from_userid(int user_id) {
        // Implementation depends on your game interface
        // This is just an example
        auto player = g_interfaces->m_engine->get_player_for_user_id(user_id);
        if (player)
            return player->get_handle().to_int();
        return -1;
    }
};

inline c_event_handler* g_event_handler = new c_event_handler();

// Example of how to register these events in your game event listener
/*
void register_events() {
    g_interfaces->m_event_manager->add_listener("bullet_impact", 
        [](c_game_event* event) { g_event_handler->on_bullet_impact(event); });
    
    g_interfaces->m_event_manager->add_listener("player_hurt", 
        [](c_game_event* event) { g_event_handler->on_player_hurt(event); });
    
    g_interfaces->m_event_manager->add_listener("round_start", 
        [](c_game_event* event) { g_event_handler->on_round_start(event); });
}
*/
