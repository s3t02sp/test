#pragma once

// Минимальная версия rage_bot.hpp с inline функциями для избежания проблем с линковкой
// Используйте эту версию, если получаете ошибки линковки с get_hitbox_name/get_reason_string

#include <deque>
#include <map>
#include <vector>
#include <memory>
#include <string>

// ... (все ваши forward declarations и структуры остаются без изменений) ...

class c_rage_bot {
public:
    void on_create_move();
    void store_records();
    
    // Shot tracking
    void register_shot(const shot_info_t& shot_info);
    void on_bullet_impact(const vec3_t& impact_pos);
    void on_player_hurt(int victim_handle, int attacker_handle, int damage, int hitgroup);
    
    // Miss log
    std::vector<miss_log_t>& get_miss_logs() { return m_miss_logs; }
    void clear_miss_logs() { m_miss_logs.clear(); }
    
    // Inline реализация для избежания проблем с линковкой
    inline std::string get_hitbox_name(int hitbox) {
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
    
    inline std::string get_reason_string(miss_reason_e reason) {
        switch (reason) {
            case MISS_REASON_SPREAD: return "Spread/Inaccuracy";
            case MISS_REASON_PREDICTION_ERROR: return "Prediction Error";
            case MISS_REASON_RESOLVER: return "Resolver";
            case MISS_REASON_HITBOX: return "Hitbox";
            default: return "Unknown";
        }
    }

private:
    void store_hitboxes();
    lag_record_t* select_record(int handle);
    void find_targets();
    aim_target_t* get_nearest_target();
    
    float calc_point_scale(vec3_t& point, const float& hitbox_radius);
    vec3_t transform_point(matrix3x4_t matrix, vec3_t point);
    std::vector<vec3_t> calculate_sphere_points(float radius, int num_points);
    std::vector<vec3_t> calculate_points(int num_points, float radius, float height, matrix3x4_t matrix, hitbox_data_t hitbox);
    
    bool multi_points(lag_record_t* record, int hitbox, std::vector<aim_point_t>& points);
    aim_point_t select_points(lag_record_t* record, float& damage);
    void select_target();
    
    bool weapon_is_at_max_accuracy(c_cs_weapon_base_v_data* weapon_data, float inaccuracy);
    vec3_t calculate_spread_angles(vec3_t angle, int random_seed, float weapon_inaccuarcy, float weapon_spread);
    int calculate_hit_chance(c_cs_player_pawn* pawn, vec3_t angles, c_base_player_weapon* active_weapon, c_cs_weapon_base_v_data* weapon_data, bool no_spread);
    
    void process_backtrack(lag_record_t* record);
    bool can_shoot(c_cs_player_pawn* pawn, c_base_player_weapon* active_weapon);
    void process_attack(c_user_cmd* user_cmd, vec3_t angle);
    
    int get_hitbox_from_menu(int hitbox);
    hitbox_data_t get_hitbox_data(c_cs_player_pawn* pawn, int hitbox);
    
    void process_miss(const shot_info_t& shot_info, const vec3_t& impact_pos);
    miss_reason_e determine_miss_reason(const shot_info_t& shot_info, const vec3_t& impact_pos);
    
    // Member variables
    std::map<int, std::deque<lag_record_t>> m_lag_records;
    std::map<int, aim_target_t> m_aim_targets;
    std::vector<int> m_hitboxes;
    aim_target_t* m_best_target;
    
    // Miss log data
    std::vector<shot_info_t> m_shot_records;
    std::vector<miss_log_t> m_miss_logs;
    shot_info_t m_last_shot;
    bool m_waiting_for_impact;
    vec3_t m_last_impact_position;
    float m_last_shot_time;
};

inline c_rage_bot* g_rage_bot = new c_rage_bot();

// ВАЖНО: Если используете эту версию с inline функциями,
// УДАЛИТЕ реализации get_hitbox_name и get_reason_string из rage_bot.cpp (строки 794-829)
