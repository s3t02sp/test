#include "hit_log.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

// Global instance
c_hit_log g_hit_log;

void c_hit_log::add_log(const std::string& message, float displayTime, float fadeDuration, int r, int g, int b)
{
    if (!g_interfaces || !g_interfaces->m_global_vars || !g_interfaces->m_engine)
        return;
    
    if (!g_interfaces->m_engine->is_in_game() || !g_interfaces->m_engine->is_connected())
        return;
    
    float currentTime = g_interfaces->m_global_vars->m_current_time;
    float fadeStartTime = currentTime + displayTime;
    
    hit_logs.emplace_front(message, currentTime, fadeStartTime, fadeDuration, r, g, b);
    
    // Limit the number of logs
    if (hit_logs.size() > maxLogs)
        hit_logs.resize(maxLogs);
}

void c_hit_log::add_hit(const std::string& targetName, int damage, int health, const std::string& hitbox)
{
    std::stringstream ss;
    ss << "hit " << targetName << " in " << hitbox 
       << " for " << damage << " (" << health << " remaining)";
    
    // Green color for hits
    add_log(ss.str(), 5.0f, 1.0f, 144, 238, 144);
}

void c_hit_log::add_miss(const std::string& targetName, const std::string& reason)
{
    std::stringstream ss;
    ss << "missed " << targetName << " due to " << reason;
    
    // Red color for misses
    add_log(ss.str(), 5.0f, 1.0f, 255, 100, 100);
}

void c_hit_log::add_kill(const std::string& targetName, int damage, const std::string& hitbox)
{
    std::stringstream ss;
    ss << "killed " << targetName << " in " << hitbox 
       << " for " << damage << " damage";
    
    // Gold color for kills
    add_log(ss.str(), 6.0f, 1.5f, 255, 215, 0);
}

void c_hit_log::draw_logs()
{
    if (!g_interfaces || !g_interfaces->m_global_vars || !g_interfaces->m_engine)
        return;
    
    if (!g_interfaces->m_engine->is_in_game() || !g_interfaces->m_engine->is_connected())
    {
        if (!hit_logs.empty())
            hit_logs.clear();
        return;
    }
    
    if (!g_ctx || !g_ctx->m_local_controller || !g_ctx->m_local_controller->m_pawn_is_alive())
    {
        if (!hit_logs.empty())
            hit_logs.clear();
        return;
    }
    
    ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
    if (!pDrawList)
        return;
    
    float currentTime = g_interfaces->m_global_vars->m_current_time;
    
    if (!g_render || !g_render->fonts.onetap_pixel)
        return;
    
    float messageHeight = g_render->fonts.onetap_pixel->FontSize + 2.0f;
    float currentY = startY;
    
    // Iterate through logs and remove expired ones
    for (auto it = hit_logs.begin(); it != hit_logs.end();)
    {
        const hit_log_entry& entry = *it;
        
        // Check if log has expired
        float totalLifetime = entry.fadeStartTime - entry.startTime + entry.fadeDuration;
        if (currentTime - entry.startTime > totalLifetime)
        {
            it = hit_logs.erase(it);
            continue;
        }
        
        // Calculate alpha for fade effect
        float alpha = 1.0f;
        if (currentTime >= entry.fadeStartTime)
        {
            float fadeProgress = (currentTime - entry.fadeStartTime) / entry.fadeDuration;
            alpha = 1.0f - fadeProgress;
            alpha = std::max(0.0f, std::min(1.0f, alpha));
        }
        
        // Colors with alpha
        ImColor textColor(entry.r, entry.g, entry.b, static_cast<int>(255 * alpha));
        ImColor shadowColor(0, 0, 0, static_cast<int>(150 * alpha));
        
        // Draw shadow (slightly offset)
        pDrawList->AddText(
            g_render->fonts.onetap_pixel,
            g_render->fonts.onetap_pixel->FontSize,
            ImVec2(startX + 1.0f, currentY + 1.0f),
            shadowColor,
            entry.message.c_str()
        );
        
        // Draw main text
        pDrawList->AddText(
            g_render->fonts.onetap_pixel,
            g_render->fonts.onetap_pixel->FontSize,
            ImVec2(startX, currentY),
            textColor,
            entry.message.c_str()
        );
        
        currentY += messageHeight;
        ++it;
    }
}

void c_hit_log::clear_logs()
{
    hit_logs.clear();
}

void c_hit_log::set_position(float x, float y)
{
    startX = x;
    startY = y;
}

void c_hit_log::set_max_logs(size_t max)
{
    maxLogs = max;
    if (hit_logs.size() > maxLogs)
        hit_logs.resize(maxLogs);
}
