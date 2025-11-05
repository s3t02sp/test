// Example integration of the legitbot into your existing CS2 cheat

#include "legit_bot.hpp"
#include "rage_bot.hpp"

// ============================================================================
// Example 1: Basic Integration in CreateMove Hook
// ============================================================================

void __fastcall hooked_create_move(void* rcx, int sequence_number, 
                                    bool is_first_command, bool is_predicted)
{
    // Call original
    original_create_move(rcx, sequence_number, is_first_command, is_predicted);
    
    // Get user command
    c_user_cmd* user_cmd = g_interfaces->m_csgo_input->get_user_cmd();
    if (!user_cmd)
        return;
    
    // Store in global context
    g_ctx->m_user_cmd = user_cmd;
    
    // Your existing prediction/movement code
    g_prediction->start();
    
    // Choose which bot mode to use
    if (g_cfg->mode == MODE_LEGIT)
    {
        g_legit_bot->on_create_move(user_cmd);
    }
    else if (g_cfg->mode == MODE_RAGE)
    {
        g_rage_bot->on_create_move();
    }
    
    // Your existing movement corrections
    g_movement->correct_movement(user_cmd);
    
    g_prediction->end();
}

// ============================================================================
// Example 2: UI/Menu Configuration
// ============================================================================

void render_legitbot_menu()
{
    ImGui::Begin("Legitbot");
    
    // Main
    ImGui::Checkbox("Enabled", &g_cfg->legit_bot.m_enabled);
    ImGui::Checkbox("Always On", &g_cfg->legit_bot.m_always_on);
    
    if (!g_cfg->legit_bot.m_always_on)
    {
        ImGui::Text("Aim Key:");
        // Your key bind UI here
    }
    
    ImGui::Separator();
    
    // Aim Settings
    ImGui::SliderFloat("FOV", &g_cfg->legit_bot.m_fov, 0.5f, 30.0f, "%.1f°");
    ImGui::SliderFloat("Smooth", &g_cfg->legit_bot.m_smooth, 1.0f, 50.0f, "%.1f");
    
    ImGui::Separator();
    
    // Target
    const char* hitboxes[] = { "Head", "Chest", "Stomach", "Pelvis" };
    ImGui::Combo("Target Hitbox", &g_cfg->legit_bot.m_target_hitbox, hitboxes, 4);
    
    ImGui::Checkbox("Team Check", &g_cfg->legit_bot.m_team_check);
    ImGui::Checkbox("Visibility Check", &g_cfg->legit_bot.m_visibility_check);
    
    ImGui::Separator();
    
    // Recoil Control
    ImGui::Checkbox("Recoil Control", &g_cfg->legit_bot.m_recoil_control);
    if (g_cfg->legit_bot.m_recoil_control)
    {
        ImGui::SliderFloat("RCS Amount", &g_cfg->legit_bot.m_recoil_control_amount, 
                          0.0f, 100.0f, "%.0f%%");
    }
    
    ImGui::Separator();
    
    // Trigger Bot
    ImGui::Checkbox("Trigger Bot", &g_cfg->legit_bot.m_trigger_bot);
    if (g_cfg->legit_bot.m_trigger_bot)
    {
        ImGui::SliderFloat("Trigger Delay", &g_cfg->legit_bot.m_trigger_delay, 
                          0.0f, 500.0f, "%.0f ms");
        ImGui::SliderFloat("Trigger FOV", &g_cfg->legit_bot.m_trigger_fov, 
                          0.1f, 5.0f, "%.1f°");
    }
    
    ImGui::Separator();
    
    // Humanizer
    ImGui::Checkbox("Humanizer", &g_cfg->legit_bot.m_humanizer);
    if (g_cfg->legit_bot.m_humanizer)
    {
        ImGui::SliderFloat("Reaction Time", &g_cfg->legit_bot.m_reaction_time, 
                          0.0f, 500.0f, "%.0f ms");
        ImGui::SliderFloat("Aim Shake", &g_cfg->legit_bot.m_aim_shake, 
                          0.0f, 0.5f, "%.2f");
    }
    
    ImGui::End();
}

// ============================================================================
// Example 3: Weapon-Specific Configurations
// ============================================================================

struct weapon_config_t
{
    float fov;
    float smooth;
    int target_hitbox;
    bool recoil_control;
    float recoil_amount;
};

std::map<int, weapon_config_t> weapon_configs = {
    // AWP - Precise, slow
    { WEAPON_AWP, { 3.0f, 15.0f, HITBOX_HEAD, false, 0.0f } },
    
    // AK47 - Moderate, with recoil control
    { WEAPON_AK47, { 5.0f, 10.0f, HITBOX_HEAD, true, 60.0f } },
    
    // M4A4 - Similar to AK
    { WEAPON_M4A4, { 5.0f, 10.0f, HITBOX_HEAD, true, 65.0f } },
    
    // Pistols - Faster, larger FOV
    { WEAPON_DEAGLE, { 7.0f, 8.0f, HITBOX_CHEST, false, 0.0f } },
    { WEAPON_USP, { 6.0f, 7.0f, HITBOX_HEAD, false, 0.0f } },
    
    // SMGs - Fast, spray
    { WEAPON_MP9, { 8.0f, 6.0f, HITBOX_CHEST, true, 50.0f } },
};

void apply_weapon_config(c_base_player_weapon* weapon)
{
    if (!weapon)
        return;
    
    int weapon_id = weapon->get_weapon_data()->m_weapon_id();
    
    auto it = weapon_configs.find(weapon_id);
    if (it != weapon_configs.end())
    {
        auto& cfg = it->second;
        g_cfg->legit_bot.m_fov = cfg.fov;
        g_cfg->legit_bot.m_smooth = cfg.smooth;
        g_cfg->legit_bot.m_target_hitbox = cfg.target_hitbox;
        g_cfg->legit_bot.m_recoil_control = cfg.recoil_control;
        g_cfg->legit_bot.m_recoil_control_amount = cfg.recoil_amount;
    }
}

// Call this in CreateMove before legitbot
void setup_weapon_specific_settings()
{
    if (!g_ctx->m_local_pawn)
        return;
    
    auto weapon = g_ctx->m_local_pawn->get_active_weapon();
    if (weapon && g_cfg->legit_bot.m_weapon_specific_config)
    {
        apply_weapon_config(weapon);
    }
}

// ============================================================================
// Example 4: Advanced Features - Backtracking Support
// ============================================================================

class c_legit_bot_extended : public c_legit_bot
{
private:
    // Store simple lag data for legitbot
    struct simple_lag_record_t
    {
        float simulation_time;
        vec3_t origin;
        vec3_t head_position;
        bool valid;
    };
    
    std::map<int, std::deque<simple_lag_record_t>> m_player_records;
    
public:
    void store_player_positions()
    {
        if (!g_interfaces->m_engine->is_in_game())
            return;
        
        auto entities = g_entity_system->get("CCSPlayerController");
        
        for (auto entity : entities)
        {
            auto controller = reinterpret_cast<c_cs_player_controller*>(entity);
            if (!controller || controller == g_ctx->m_local_controller)
                continue;
            
            if (!controller->m_pawn_is_alive())
                continue;
            
            auto pawn = reinterpret_cast<c_cs_player_pawn*>(
                g_interfaces->m_entity_system->get_base_entity(
                    controller->m_pawn().get_entry_index()));
            
            if (!pawn)
                continue;
            
            int handle = controller->get_handle().to_int();
            
            // Store record
            simple_lag_record_t record;
            record.simulation_time = pawn->m_simulation_time();
            record.origin = pawn->m_scene_node()->m_origin();
            record.head_position = get_hitbox_position(pawn, HITBOX_HEAD);
            record.valid = true;
            
            if (m_player_records[handle].size() >= 12)
                m_player_records[handle].pop_back();
            
            m_player_records[handle].push_front(record);
        }
    }
    
    // Override select_aim_point to use backtrack records if better
    vec3_t select_aim_point_with_backtrack(legit_target_t* target)
    {
        if (!target || !target->m_controller)
            return vec3_t(0.f, 0.f, 0.f);
        
        int handle = target->m_controller->get_handle().to_int();
        auto& records = m_player_records[handle];
        
        if (records.empty())
            return select_aim_point(target);
        
        // Use most recent valid record
        return records.front().head_position;
    }
};

// ============================================================================
// Example 5: Complete Setup Function
// ============================================================================

void initialize_legitbot()
{
    // Set default config
    g_cfg->legit_bot.m_enabled = false;
    g_cfg->legit_bot.m_always_on = false;
    g_cfg->legit_bot.m_fov = 5.0f;
    g_cfg->legit_bot.m_smooth = 10.0f;
    g_cfg->legit_bot.m_target_hitbox = HITBOX_HEAD;
    g_cfg->legit_bot.m_team_check = true;
    g_cfg->legit_bot.m_visibility_check = true;
    g_cfg->legit_bot.m_recoil_control = false;
    g_cfg->legit_bot.m_recoil_control_amount = 50.0f;
    g_cfg->legit_bot.m_trigger_bot = false;
    g_cfg->legit_bot.m_trigger_delay = 50.0f;
    g_cfg->legit_bot.m_trigger_fov = 0.5f;
    g_cfg->legit_bot.m_humanizer = true;
    g_cfg->legit_bot.m_reaction_time = 120.0f;
    g_cfg->legit_bot.m_aim_shake = 0.05f;
    
    // Initialize the global instance
    if (!g_legit_bot)
        g_legit_bot = new c_legit_bot();
}

// ============================================================================
// Example 6: Debug Visualization
// ============================================================================

void draw_legitbot_debug()
{
    if (!g_cfg->legit_bot.m_enabled || !g_cfg->debug.m_show_legitbot_info)
        return;
    
    auto local_data = g_prediction->get_local_data();
    if (!local_data)
        return;
    
    // Draw FOV circle
    vec3_t screen_center = g_render->get_screen_size() * 0.5f;
    float fov_radius = (g_cfg->legit_bot.m_fov / 90.0f) * screen_center.x;
    
    g_render->draw_circle(screen_center.x, screen_center.y, fov_radius, 
                         Color(255, 255, 0, 100), 64);
    
    // Draw trigger FOV circle
    if (g_cfg->legit_bot.m_trigger_bot)
    {
        float trigger_radius = (g_cfg->legit_bot.m_trigger_fov / 90.0f) * screen_center.x;
        g_render->draw_circle(screen_center.x, screen_center.y, trigger_radius, 
                             Color(255, 0, 0, 150), 32);
    }
    
    // Draw info text
    std::string info = "Legitbot\n";
    info += "FOV: " + std::to_string(g_cfg->legit_bot.m_fov) + "\n";
    info += "Smooth: " + std::to_string(g_cfg->legit_bot.m_smooth) + "\n";
    info += "Humanizer: " + std::string(g_cfg->legit_bot.m_humanizer ? "ON" : "OFF");
    
    g_render->draw_text(10, 100, Color(255, 255, 255), info.c_str());
}

// ============================================================================
// Example 7: Config Presets
// ============================================================================

enum LegitbotPreset
{
    PRESET_CUSTOM = 0,
    PRESET_SAFE,
    PRESET_BALANCED,
    PRESET_AGGRESSIVE,
    PRESET_TRIGGER_ONLY
};

void apply_legitbot_preset(LegitbotPreset preset)
{
    switch (preset)
    {
        case PRESET_SAFE:
            g_cfg->legit_bot.m_fov = 3.0f;
            g_cfg->legit_bot.m_smooth = 25.0f;
            g_cfg->legit_bot.m_humanizer = true;
            g_cfg->legit_bot.m_reaction_time = 200.0f;
            g_cfg->legit_bot.m_aim_shake = 0.15f;
            g_cfg->legit_bot.m_trigger_bot = false;
            break;
            
        case PRESET_BALANCED:
            g_cfg->legit_bot.m_fov = 5.0f;
            g_cfg->legit_bot.m_smooth = 12.0f;
            g_cfg->legit_bot.m_humanizer = true;
            g_cfg->legit_bot.m_reaction_time = 120.0f;
            g_cfg->legit_bot.m_aim_shake = 0.08f;
            g_cfg->legit_bot.m_trigger_bot = true;
            g_cfg->legit_bot.m_trigger_delay = 50.0f;
            break;
            
        case PRESET_AGGRESSIVE:
            g_cfg->legit_bot.m_fov = 10.0f;
            g_cfg->legit_bot.m_smooth = 5.0f;
            g_cfg->legit_bot.m_humanizer = true;
            g_cfg->legit_bot.m_reaction_time = 50.0f;
            g_cfg->legit_bot.m_aim_shake = 0.03f;
            g_cfg->legit_bot.m_trigger_bot = true;
            g_cfg->legit_bot.m_trigger_delay = 10.0f;
            break;
            
        case PRESET_TRIGGER_ONLY:
            g_cfg->legit_bot.m_fov = 0.5f; // Minimal aim assist
            g_cfg->legit_bot.m_smooth = 50.0f; // Very slow
            g_cfg->legit_bot.m_trigger_bot = true;
            g_cfg->legit_bot.m_trigger_delay = 30.0f;
            g_cfg->legit_bot.m_trigger_fov = 0.3f;
            break;
    }
}
