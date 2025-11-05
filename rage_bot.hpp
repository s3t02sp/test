#pragma once
#include <deque>
#include <map>
#include <vector>
#include <memory>
#include <string>

// Forward declarations
class c_cs_player_pawn;
class c_cs_player_controller;
class c_base_player_weapon;
class c_cs_weapon_base_v_data;
struct c_user_cmd;

struct lag_record_t {
    c_cs_player_pawn* m_pawn;
    float m_simulation_time;
    bool m_throwing;
    std::vector<matrix3x4_t> m_bone_data;

    void store(c_cs_player_pawn* pawn);
    void apply(c_cs_player_pawn* pawn);
    void reset(c_cs_player_pawn* pawn);
    bool is_valid();
};

struct aim_point_t {
    vec3_t m_point;
    int m_hitbox;
    bool m_is_center;

    aim_point_t() : m_point(0.f, 0.f, 0.f), m_hitbox(-1), m_is_center(false) {}
    aim_point_t(vec3_t point, int hitbox, bool is_center = false)
        : m_point(point), m_hitbox(hitbox), m_is_center(is_center) {}
};

struct aim_target_t {
    lag_record_t* m_lag_record;
    c_cs_player_pawn* m_pawn;
    std::unique_ptr<aim_point_t> m_best_point;

    aim_target_t() : m_lag_record(nullptr), m_pawn(nullptr), m_best_point(nullptr) {}
    aim_target_t(lag_record_t* record) : m_lag_record(record), m_pawn(record->m_pawn), m_best_point(nullptr) {}

    void reset() {
        m_best_point.reset();
    }
};

struct hitbox_data_t {
    vec3_t m_mins;
    vec3_t m_maxs;
    float m_radius;
    int m_num_bone;
    bool m_invalid_data;
};

// Miss log structures
struct shot_info_t {
    int m_target_handle;
    std::string m_target_name;
    vec3_t m_shoot_position;
    vec3_t m_aim_point;
    vec3_t m_aim_angles;
    int m_hitbox;
    float m_damage;
    int m_hit_chance;
    float m_spread;
    float m_inaccuracy;
    int m_tick_count;
    float m_simulation_time;
    int m_seed;
    bool m_no_spread;
    
    shot_info_t()
        : m_target_handle(-1)
        , m_target_name("")
        , m_shoot_position(0.f, 0.f, 0.f)
        , m_aim_point(0.f, 0.f, 0.f)
        , m_aim_angles(0.f, 0.f, 0.f)
        , m_hitbox(-1)
        , m_damage(0.f)
        , m_hit_chance(0)
        , m_spread(0.f)
        , m_inaccuracy(0.f)
        , m_tick_count(0)
        , m_simulation_time(0.f)
        , m_seed(0)
        , m_no_spread(false)
    {}
};

enum miss_reason_e {
    MISS_REASON_SPREAD,
    MISS_REASON_PREDICTION_ERROR,
    MISS_REASON_RESOLVER,
    MISS_REASON_HITBOX,
    MISS_REASON_UNKNOWN
};

struct miss_log_t {
    shot_info_t m_shot_info;
    miss_reason_e m_reason;
    std::string m_reason_string;
    float m_time_logged;
    vec3_t m_hit_position;
    int m_hit_group;
    bool m_did_hit;
    
    miss_log_t()
        : m_reason(MISS_REASON_UNKNOWN)
        , m_reason_string("Unknown")
        , m_time_logged(0.f)
        , m_hit_position(0.f, 0.f, 0.f)
        , m_hit_group(-1)
        , m_did_hit(false)
    {}
};

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
    
    // Miss log helpers
    void process_miss(const shot_info_t& shot_info, const vec3_t& impact_pos);
    miss_reason_e determine_miss_reason(const shot_info_t& shot_info, const vec3_t& impact_pos);
    std::string get_hitbox_name(int hitbox);
    std::string get_reason_string(miss_reason_e reason);

private:
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
