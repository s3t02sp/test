#pragma once
#include "rage_bot.hpp"
#include "../../render/render.hpp"

// Example implementation of miss log rendering
// Call this in your ESP/render loop

class c_miss_log_render {
public:
    void render() {
        if (!g_cfg->rage_bot.m_enabled || !g_cfg->rage_bot.m_show_miss_log)
            return;
            
        auto& miss_logs = g_rage_bot->get_miss_logs();
        
        if (miss_logs.empty())
            return;
            
        // Render position
        int x = g_cfg->rage_bot.m_miss_log_x;
        int y = g_cfg->rage_bot.m_miss_log_y;
        
        // Title
        g_render->text(x, y, color_t(255, 255, 255, 255), FONT_DEFAULT, "MISS LOG", false);
        y += 20;
        
        // Get current time
        float current_time = g_interfaces->m_globals->m_curtime;
        
        // Render each miss (newest first)
        int displayed = 0;
        for (auto it = miss_logs.rbegin(); it != miss_logs.rend() && displayed < 10; ++it, ++displayed) {
            const auto& miss = *it;
            
            // Fade out old entries
            float time_since_miss = current_time - miss.m_time_logged;
            if (time_since_miss > 10.f)
                continue;
                
            int alpha = static_cast<int>(255.f * (1.f - (time_since_miss / 10.f)));
            
            // Color based on reason
            color_t color;
            switch (miss.m_reason) {
                case MISS_REASON_SPREAD:
                    color = color_t(255, 100, 100, alpha); // Red - spread
                    break;
                case MISS_REASON_PREDICTION_ERROR:
                    color = color_t(255, 165, 0, alpha); // Orange - prediction
                    break;
                case MISS_REASON_RESOLVER:
                    color = color_t(255, 255, 0, alpha); // Yellow - resolver
                    break;
                default:
                    color = color_t(200, 200, 200, alpha); // Gray - other
                    break;
            }
            
            // Build log string
            std::string log_text;
            
            if (miss.m_did_hit) {
                // We hit but wrong hitbox
                log_text = miss.m_shot_info.m_target_name + " [" + 
                    get_hitbox_name(miss.m_shot_info.m_hitbox) + " -> " +
                    get_hitbox_name(miss.m_hit_group) + "] " +
                    std::to_string(miss.m_shot_info.m_hit_chance) + "%";
            } else {
                // Complete miss
                log_text = miss.m_shot_info.m_target_name + " [" + 
                    get_hitbox_name(miss.m_shot_info.m_hitbox) + "] " +
                    miss.m_reason_string + " " +
                    std::to_string(miss.m_shot_info.m_hit_chance) + "%";
            }
            
            // Add spread info if it was spread-related
            if (miss.m_reason == MISS_REASON_SPREAD) {
                int spread_value = static_cast<int>(miss.m_shot_info.m_spread * 1000.f);
                int inaccuracy_value = static_cast<int>(miss.m_shot_info.m_inaccuracy * 1000.f);
                log_text += " [S:" + std::to_string(spread_value) + " I:" + std::to_string(inaccuracy_value) + "]";
            }
            
            g_render->text(x, y, color, FONT_DEFAULT, log_text.c_str(), false);
            y += 15;
        }
    }
    
    void render_3d_visualizations() {
        if (!g_cfg->rage_bot.m_enabled || !g_cfg->rage_bot.m_show_3d_miss_log)
            return;
            
        auto& miss_logs = g_rage_bot->get_miss_logs();
        
        if (miss_logs.empty())
            return;
            
        float current_time = g_interfaces->m_globals->m_curtime;
        
        // Render 3D lines showing miss positions
        for (const auto& miss : miss_logs) {
            float time_since_miss = current_time - miss.m_time_logged;
            
            // Only show recent misses (last 3 seconds)
            if (time_since_miss > 3.f)
                continue;
                
            int alpha = static_cast<int>(255.f * (1.f - (time_since_miss / 3.f)));
            
            // Color based on reason
            color_t color;
            switch (miss.m_reason) {
                case MISS_REASON_SPREAD:
                    color = color_t(255, 0, 0, alpha); // Red
                    break;
                case MISS_REASON_PREDICTION_ERROR:
                    color = color_t(255, 165, 0, alpha); // Orange
                    break;
                case MISS_REASON_RESOLVER:
                    color = color_t(255, 255, 0, alpha); // Yellow
                    break;
                default:
                    color = color_t(200, 200, 200, alpha); // Gray
                    break;
            }
            
            // Draw line from shoot position to aim point (where we wanted to hit)
            g_render->draw_line_3d(miss.m_shot_info.m_shoot_position, 
                                   miss.m_shot_info.m_aim_point, 
                                   color_t(0, 255, 0, alpha));
            
            // Draw line from shoot position to impact point (where bullet actually went)
            if (!miss.m_did_hit) {
                g_render->draw_line_3d(miss.m_shot_info.m_shoot_position, 
                                       miss.m_hit_position, 
                                       color);
            }
            
            // Draw sphere at aim point
            g_render->draw_sphere_3d(miss.m_shot_info.m_aim_point, 2.f, 
                                     color_t(0, 255, 0, alpha));
            
            // Draw sphere at impact point
            if (!miss.m_did_hit) {
                g_render->draw_sphere_3d(miss.m_hit_position, 2.f, color);
            }
        }
    }
    
private:
    std::string get_hitbox_name(int hitbox) {
        return g_rage_bot->get_hitbox_name(hitbox);
    }
};

inline c_miss_log_render* g_miss_log_render = new c_miss_log_render();

// Call these in your render hooks:
// - g_miss_log_render->render() in your 2D overlay render
// - g_miss_log_render->render_3d_visualizations() in your 3D world render
