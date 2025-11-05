// Example usage of the GameSense-style hit log system
#include "hit_log.h"

// Example 1: Using the draw function in your menu/render loop
void c_menu::draw_logs()
{
    g_hit_log.draw_logs();
}

// Example 2: Adding hit logs when you hit an enemy
void on_player_hurt(const std::string& targetName, int damage, int healthRemaining, const std::string& hitbox)
{
    g_hit_log.add_hit(targetName, damage, healthRemaining, hitbox);
    
    // Examples:
    // "hit enemy in head for 95 (5 remaining)"
    // "hit enemy in chest for 32 (68 remaining)"
}

// Example 3: Adding miss logs when you miss a shot
void on_shot_miss(const std::string& targetName, const std::string& reason)
{
    g_hit_log.add_miss(targetName, reason);
    
    // Examples:
    // "missed enemy due to spread"
    // "missed enemy due to resolver"
    // "missed enemy due to prediction error"
}

// Example 4: Adding kill logs when you get a kill
void on_player_death(const std::string& targetName, int damage, const std::string& hitbox)
{
    g_hit_log.add_kill(targetName, damage, hitbox);
    
    // Examples:
    // "killed enemy in head for 95 damage"
    // "killed enemy in chest for 100 damage"
}

// Example 5: Custom log with custom color
void add_custom_log()
{
    // White text
    g_hit_log.add_log("Custom message here", 5.0f, 1.0f, 255, 255, 255);
    
    // Red text
    g_hit_log.add_log("Error message", 5.0f, 1.0f, 255, 0, 0);
    
    // Green text
    g_hit_log.add_log("Success message", 5.0f, 1.0f, 0, 255, 0);
    
    // Blue text
    g_hit_log.add_log("Info message", 5.0f, 1.0f, 100, 150, 255);
}

// Example 6: Configuration
void configure_hit_log()
{
    // Set position (x, y coordinates)
    g_hit_log.set_position(10.0f, 10.0f);
    
    // Set maximum number of visible logs
    g_hit_log.set_max_logs(8);
    
    // Clear all logs
    g_hit_log.clear_logs();
}

// Example 7: Integration with your cheat's event system
class ExampleEventListener
{
public:
    void on_bullet_impact(int userId, float x, float y, float z)
    {
        // Your bullet impact logic
        bool didHit = check_if_hit(userId, x, y, z);
        
        if (didHit)
        {
            auto targetName = get_player_name(userId);
            auto hitbox = get_hitbox_name(x, y, z);
            auto damage = calculate_damage();
            auto health = get_player_health(userId);
            
            g_hit_log.add_hit(targetName, damage, health, hitbox);
        }
        else
        {
            auto targetName = get_player_name(userId);
            auto reason = get_miss_reason();
            
            g_hit_log.add_miss(targetName, reason);
        }
    }
    
private:
    bool check_if_hit(int userId, float x, float y, float z) { return false; }
    std::string get_player_name(int userId) { return "enemy"; }
    std::string get_hitbox_name(float x, float y, float z) { return "head"; }
    int calculate_damage() { return 95; }
    int get_player_health(int userId) { return 5; }
    std::string get_miss_reason() { return "spread"; }
};

/*
 * FEATURES:
 * 
 * - Smooth fade-in/fade-out animations
 * - Color-coded messages (green for hits, red for misses, gold for kills)
 * - Customizable position and maximum number of logs
 * - Automatic cleanup of expired logs
 * - Shadow text effect for better readability
 * - Uses deque for efficient log management
 * - Thread-safe log additions
 * - Automatically clears logs when not in game
 * 
 * HITBOX NAMES EXAMPLES:
 * - "head"
 * - "chest"
 * - "stomach"
 * - "left arm"
 * - "right arm"
 * - "left leg"
 * - "right leg"
 * 
 * MISS REASONS EXAMPLES:
 * - "spread"
 * - "resolver"
 * - "prediction error"
 * - "bad angle"
 * - "occlusion"
 * - "lag compensation"
 */
