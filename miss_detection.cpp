// Детекция причин промахов (miss detection) для CS2
#include "hit_log.h"
#include <cmath>

// Структура для хранения информации о выстреле
struct shot_data
{
    Vector aim_point;           // Точка прицеливания (где был прицел)
    Vector shoot_position;      // Позиция стрельбы
    QAngle view_angles;         // Углы обзора при выстреле
    int target_index;           // Индекс цели
    int hitbox;                 // Целевой хитбокс
    float time;                 // Время выстрела
    float inaccuracy;           // Инаккураси оружия
    float spread;               // Спред оружия
    bool should_hit;            // Должна ли была попасть пуля
    int target_health;          // Здоровье цели до выстрела
    std::string target_name;    // Имя цели
};

class c_miss_detector
{
public:
    // Регистрация выстрела
    void register_shot(const shot_data& shot);
    
    // Обработка попадания
    void on_player_hurt(int victim, int attacker, int damage, int hitbox);
    
    // Обработка bullet impact (куда реально прилетела пуля)
    void on_bullet_impact(int user_id, const Vector& impact_point);
    
    // Вызывается каждый фрейм для проверки таймаута выстрелов
    void update();

private:
    std::vector<shot_data> pending_shots;
    
    // Вычисление причины промаха
    std::string determine_miss_reason(const shot_data& shot, const Vector& impact_point);
    
    // Проверка spread miss
    bool is_spread_miss(const shot_data& shot, const Vector& impact_point);
    
    // Проверка resolver miss
    bool is_resolver_miss(const shot_data& shot);
    
    // Проверка occlusion (попадание в стену/объект)
    bool is_occlusion_miss(const shot_data& shot, const Vector& impact_point);
    
    // Вычисление угла между двумя точками
    float calculate_angle(const Vector& src, const Vector& dst1, const Vector& dst2);
    
    // Получение позиции хитбокса
    Vector get_hitbox_position(int entity_index, int hitbox);
    
    // Константы для детекции
    static constexpr float SPREAD_ANGLE_THRESHOLD = 2.0f;      // Градусы
    static constexpr float OCCLUSION_DISTANCE_THRESHOLD = 50.0f; // Юниты
    static constexpr float SHOT_TIMEOUT = 0.5f;                // Секунды
};

void c_miss_detector::register_shot(const shot_data& shot)
{
    pending_shots.push_back(shot);
}

void c_miss_detector::on_player_hurt(int victim, int attacker, int damage, int hitbox)
{
    // Проверяем, был ли это наш выстрел
    if (attacker != g_ctx->m_local_controller->get_index())
        return;
    
    // Находим соответствующий зарегистрированный выстрел
    for (auto it = pending_shots.begin(); it != pending_shots.end(); ++it)
    {
        if (it->target_index == victim)
        {
            // Попадание зарегистрировано - добавляем хит лог
            int health_remaining = it->target_health - damage;
            std::string hitbox_name = get_hitbox_name(hitbox);
            
            g_hit_log.add_hit(it->target_name, damage, health_remaining, hitbox_name);
            
            // Удаляем выстрел из pending
            pending_shots.erase(it);
            return;
        }
    }
}

void c_miss_detector::on_bullet_impact(int user_id, const Vector& impact_point)
{
    // Проверяем, был ли это наш выстрел
    if (user_id != g_ctx->m_local_controller->get_index())
        return;
    
    float current_time = g_interfaces->m_global_vars->m_current_time;
    
    // Ищем ближайший pending выстрел
    for (auto it = pending_shots.begin(); it != pending_shots.end(); ++it)
    {
        // Проверяем таймаут
        if (current_time - it->time > SHOT_TIMEOUT)
            continue;
        
        // Определяем причину промаха
        std::string miss_reason = determine_miss_reason(*it, impact_point);
        
        // Добавляем лог промаха
        g_hit_log.add_miss(it->target_name, miss_reason);
        
        // Удаляем обработанный выстрел
        pending_shots.erase(it);
        return;
    }
}

std::string c_miss_detector::determine_miss_reason(const shot_data& shot, const Vector& impact_point)
{
    // 1. Проверка SPREAD - самая частая причина
    if (is_spread_miss(shot, impact_point))
    {
        return "spread";
    }
    
    // 2. Проверка OCCLUSION - попадание в стену/объект
    if (is_occlusion_miss(shot, impact_point))
    {
        return "occlusion";
    }
    
    // 3. Проверка RESOLVER - неправильное предсказание поворота
    if (is_resolver_miss(shot))
    {
        return "resolver";
    }
    
    // 4. Проверка других причин
    Vector hitbox_pos = get_hitbox_position(shot.target_index, shot.hitbox);
    float distance = (impact_point - hitbox_pos).Length();
    
    if (distance > 100.0f)
    {
        return "prediction error"; // Большая ошибка - скорее всего предикт
    }
    
    // Если ничего не подошло
    return "unknown";
}

bool c_miss_detector::is_spread_miss(const shot_data& shot, const Vector& impact_point)
{
    // Получаем позицию целевого хитбокса
    Vector hitbox_pos = get_hitbox_position(shot.target_index, shot.hitbox);
    
    // Вычисляем углы
    // 1. Угол между нашей позицией и точкой прицеливания
    QAngle aim_angle = calculate_angle_vectors(shot.shoot_position, shot.aim_point);
    
    // 2. Угол между нашей позицией и реальным импактом
    QAngle impact_angle = calculate_angle_vectors(shot.shoot_position, impact_point);
    
    // 3. Разница углов (FOV)
    float angle_diff = calculate_fov(aim_angle, impact_angle);
    
    // Проверяем условия для spread miss:
    // - Прицел был на цели (малый угол до хитбокса)
    float angle_to_hitbox = calculate_fov(shot.view_angles, 
                                          calculate_angle_vectors(shot.shoot_position, hitbox_pos));
    
    // - Но пуля улетела из-за спреда
    bool aimed_correctly = angle_to_hitbox <= SPREAD_ANGLE_THRESHOLD;
    
    // - Инаккураси/спред оружия был высоким
    float weapon_inaccuracy = shot.inaccuracy + shot.spread;
    bool high_inaccuracy = weapon_inaccuracy > 0.01f; // Порог для определения высокого спреда
    
    // - Расстояние импакта от хитбокса в пределах разумного
    float impact_to_hitbox_dist = (impact_point - hitbox_pos).Length();
    bool reasonable_distance = impact_to_hitbox_dist < 200.0f; // Не улетела слишком далеко
    
    return aimed_correctly && high_inaccuracy && reasonable_distance && angle_diff > 0.5f;
}

bool c_miss_detector::is_resolver_miss(const shot_data& shot)
{
    // Получаем текущую позицию хитбокса
    Vector current_hitbox_pos = get_hitbox_position(shot.target_index, shot.hitbox);
    
    // Сравниваем с тем, куда мы целились
    float distance = (shot.aim_point - current_hitbox_pos).Length();
    
    // Если разница большая - скорее всего resolver не смог правильно предсказать
    // поворот тела (body yaw)
    if (distance > 30.0f) // ~30 units разницы обычно означает неправильный resolve
    {
        // Дополнительно проверяем, что цель двигалась/поворачивалась
        // (можно добавить проверку velocity или angle changes)
        return true;
    }
    
    return false;
}

bool c_miss_detector::is_occlusion_miss(const shot_data& shot, const Vector& impact_point)
{
    // Проверяем, попала ли пуля в стену/объект перед целью
    
    // 1. Трейс от нас до импакта
    trace_t tr;
    Ray_t ray;
    ray.Init(shot.shoot_position, impact_point);
    
    CTraceFilter filter;
    filter.pSkip = g_ctx->m_local_controller->m_pawn;
    
    g_interfaces->m_engine_trace->TraceRay(ray, MASK_SHOT, &filter, &tr);
    
    // 2. Проверяем, попали ли мы в что-то кроме игрока
    if (tr.m_pEnt && tr.m_pEnt != get_entity(shot.target_index))
    {
        // Проверяем расстояние до хитбокса
        Vector hitbox_pos = get_hitbox_position(shot.target_index, shot.hitbox);
        float distance_to_hitbox = (tr.endpos - hitbox_pos).Length();
        
        // Если хитили объект близко к цели - это occlusion
        if (distance_to_hitbox < OCCLUSION_DISTANCE_THRESHOLD)
        {
            return true;
        }
    }
    
    return false;
}

void c_miss_detector::update()
{
    if (pending_shots.empty())
        return;
    
    float current_time = g_interfaces->m_global_vars->m_current_time;
    
    // Удаляем старые выстрелы (timeout)
    for (auto it = pending_shots.begin(); it != pending_shots.end();)
    {
        if (current_time - it->time > SHOT_TIMEOUT)
        {
            // Таймаут - добавляем лог с неизвестной причиной
            g_hit_log.add_miss(it->target_name, "timeout");
            it = pending_shots.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// Глобальный экземпляр
c_miss_detector g_miss_detector;

// ============================================================================
// ПРИМЕР ИНТЕГРАЦИИ В РЕЙДЖБОТ
// ============================================================================

class c_ragebot_example
{
public:
    void on_create_move()
    {
        // 1. Ищем цель и выбираем точку для стрельбы
        int target_index = find_best_target();
        if (target_index == -1)
            return;
        
        int best_hitbox = select_best_hitbox(target_index);
        Vector aim_point = calculate_aim_point(target_index, best_hitbox);
        
        // 2. Если стреляем - регистрируем выстрел
        if (should_shoot())
        {
            shot_data shot;
            shot.aim_point = aim_point;
            shot.shoot_position = g_ctx->m_local_controller->m_pawn->get_eye_position();
            shot.view_angles = cmd->viewangles;
            shot.target_index = target_index;
            shot.hitbox = best_hitbox;
            shot.time = g_interfaces->m_global_vars->m_current_time;
            shot.target_name = get_player_name(target_index);
            shot.target_health = get_player_health(target_index);
            
            // Получаем данные об инаккураси оружия
            auto weapon = g_ctx->m_local_controller->m_pawn->get_active_weapon();
            if (weapon)
            {
                shot.inaccuracy = weapon->get_inaccuracy();
                shot.spread = weapon->get_spread();
            }
            
            // Регистрируем выстрел
            g_miss_detector.register_shot(shot);
        }
    }
    
private:
    int find_best_target() { return -1; }
    int select_best_hitbox(int index) { return 0; }
    Vector calculate_aim_point(int index, int hitbox) { return Vector(); }
    bool should_shoot() { return false; }
    std::string get_player_name(int index) { return "enemy"; }
    int get_player_health(int index) { return 100; }
};

// ============================================================================
// ПРИМЕР ИНТЕГРАЦИИ В EVENT LISTENER
// ============================================================================

class c_game_event_listener_example
{
public:
    void on_bullet_impact(IGameEvent* event)
    {
        int user_id = event->GetInt("userid");
        Vector impact(
            event->GetFloat("x"),
            event->GetFloat("y"),
            event->GetFloat("z")
        );
        
        g_miss_detector.on_bullet_impact(user_id, impact);
    }
    
    void on_player_hurt(IGameEvent* event)
    {
        int victim = event->GetInt("userid");
        int attacker = event->GetInt("attacker");
        int damage = event->GetInt("dmg_health");
        int hitgroup = event->GetInt("hitgroup");
        
        g_miss_detector.on_player_hurt(victim, attacker, damage, hitgroup);
    }
};

/*
 * ДЕТАЛЬНОЕ ОБЪЯСНЕНИЕ SPREAD MISS ДЕТЕКТА:
 * 
 * Spread (разброс) - это случайное отклонение пули от центра прицела.
 * В CS2 каждое оружие имеет параметры:
 * - Inaccuracy (базовая неточность)
 * - Spread (дополнительный разброс при движении/стрельбе)
 * 
 * КАК ДЕТЕКТИТЬ SPREAD MISS:
 * 
 * 1. При выстреле записываем:
 *    - Куда мы целились (aim_point)
 *    - Наши углы обзора (view_angles)
 *    - Инаккураси оружия (weapon->GetInaccuracy())
 *    - Спред оружия (weapon->GetSpread())
 * 
 * 2. При получении bullet_impact события:
 *    - Получаем куда реально прилетела пуля (impact_point)
 *    - Вычисляем угол между aim_point и impact_point
 *    - Вычисляем расстояние от impact_point до хитбокса
 * 
 * 3. Определяем spread miss если:
 *    ✓ Прицел был на цели (angle_to_hitbox < threshold)
 *    ✓ Но пуля отклонилась (angle_diff > threshold)
 *    ✓ Инаккураси оружия был высоким (inaccuracy + spread > threshold)
 *    ✓ Пуля не улетела слишком далеко (distance < threshold)
 * 
 * КОГДА БЫВАЕТ SPREAD MISS:
 * - Стрельба в прыжке
 * - Стрельба во время движения
 * - Стрельба без остановки (running accuracy)
 * - Стрельба на дальние дистанции
 * - Спам выстрелов (recoil recovery не завершен)
 * 
 * СОВЕТЫ ПО УЛУЧШЕНИЮ:
 * 
 * 1. Добавить проверку movement:
 *    if (local->get_velocity().Length2D() > 5.0f)
 *        return "spread (moving)";
 * 
 * 2. Добавить проверку в воздухе:
 *    if (!(local->get_flags() & FL_ONGROUND))
 *        return "spread (in air)";
 * 
 * 3. Добавить историю выстрелов:
 *    if (shots_fired > 3)
 *        return "spread (recoil)";
 * 
 * 4. Добавить проверку дистанции:
 *    if (distance_to_target > 2000.0f)
 *        return "spread (distance)";
 */
