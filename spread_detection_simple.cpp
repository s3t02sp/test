// Упрощенный вариант детекта SPREAD MISS
// Легко интегрируется в любой чит

#include <vector>
#include <cmath>

// Минимальная структура для отслеживания выстрелов
struct simple_shot
{
    Vector aim_pos;        // Куда целились
    Vector shoot_pos;      // Откуда стреляли
    int target_id;         // ID цели
    float time;            // Время выстрела
    float weapon_spread;   // Спред оружия
};

std::vector<simple_shot> shots;

// ============================================================================
// ШАГ 1: При выстреле записываем данные
// ============================================================================

void on_weapon_fire()
{
    auto local = g_ctx->m_local_controller->m_pawn;
    auto weapon = local->get_active_weapon();
    
    if (!weapon || !target)
        return;
    
    simple_shot shot;
    shot.aim_pos = target_hitbox_position;      // Куда целились
    shot.shoot_pos = local->get_eye_position(); // Откуда стреляли
    shot.target_id = target->get_index();       // ID цели
    shot.time = g_interfaces->m_global_vars->m_current_time;
    
    // ВАЖНО: Получаем текущий спред оружия
    shot.weapon_spread = weapon->get_inaccuracy() + weapon->get_spread();
    
    shots.push_back(shot);
}

// ============================================================================
// ШАГ 2: При bullet_impact проверяем промах
// ============================================================================

void on_bullet_impact_event(const Vector& impact_pos)
{
    if (shots.empty())
        return;
    
    // Берем последний выстрел
    simple_shot& shot = shots.back();
    
    // Проверяем таймаут (0.5 секунд)
    float current_time = g_interfaces->m_global_vars->m_current_time;
    if (current_time - shot.time > 0.5f)
    {
        shots.pop_back();
        return;
    }
    
    // ========================================================================
    // ОСНОВНАЯ ЛОГИКА ДЕТЕКТА SPREAD MISS
    // ========================================================================
    
    // 1. Вычисляем расстояние от импакта до точки прицеливания
    float distance_from_aim = (impact_pos - shot.aim_pos).Length();
    
    // 2. Вычисляем расстояние от глаз до цели
    float distance_to_target = (shot.shoot_pos - shot.aim_pos).Length();
    
    // 3. Вычисляем угол отклонения пули
    // Формула: tan(angle) = distance_from_aim / distance_to_target
    float spread_angle = atan2f(distance_from_aim, distance_to_target) * (180.0f / M_PI);
    
    // 4. Получаем максимально возможный спред для оружия
    // Spread в радианах -> градусах
    float max_weapon_spread = shot.weapon_spread * (180.0f / M_PI);
    
    // ========================================================================
    // ОПРЕДЕЛЯЕМ ПРИЧИНУ ПРОМАХА
    // ========================================================================
    
    bool is_spread_miss = false;
    std::string miss_reason = "unknown";
    
    // Если пуля отклонилась в пределах спреда оружия - это SPREAD MISS
    if (spread_angle <= max_weapon_spread * 2.0f && spread_angle > 0.3f)
    {
        is_spread_miss = true;
        miss_reason = "spread";
    }
    // Если пуля отклонилась сильно больше - другая причина
    else if (spread_angle > max_weapon_spread * 3.0f)
    {
        miss_reason = "resolver"; // Скорее всего цель повернулась
    }
    // Если пуля практически не отклонилась
    else if (distance_from_aim < 10.0f)
    {
        miss_reason = "occlusion"; // Попали в стену перед целью
    }
    
    // Логируем
    if (is_spread_miss)
    {
        g_hit_log.add_miss(get_player_name(shot.target_id), miss_reason);
    }
    
    // Удаляем обработанный выстрел
    shots.pop_back();
}

// ============================================================================
// АЛЬТЕРНАТИВНЫЙ МЕТОД: Проверка через углы
// ============================================================================

bool is_spread_miss_angle_method(const simple_shot& shot, const Vector& impact_pos)
{
    // Вектор от стрелка к цели (куда хотели попасть)
    Vector dir_to_aim = (shot.aim_pos - shot.shoot_pos).Normalized();
    
    // Вектор от стрелка к импакту (куда реально попала пуля)
    Vector dir_to_impact = (impact_pos - shot.shoot_pos).Normalized();
    
    // Вычисляем угол между векторами через dot product
    float dot = dir_to_aim.Dot(dir_to_impact);
    float angle = acosf(dot) * (180.0f / M_PI); // В градусах
    
    // Максимальный угол спреда для оружия
    float max_spread_angle = shot.weapon_spread * (180.0f / M_PI);
    
    // Если угол отклонения в пределах спреда - это spread miss
    if (angle > 0.1f && angle <= max_spread_angle * 2.5f)
    {
        return true;
    }
    
    return false;
}

// ============================================================================
// ЕЩЕ БОЛЕЕ ПРОСТОЙ МЕТОД: По расстоянию
// ============================================================================

bool is_spread_miss_distance_method(const simple_shot& shot, const Vector& impact_pos)
{
    // Просто проверяем расстояние от импакта до цели
    float distance = (impact_pos - shot.aim_pos).Length();
    
    // Примерные пороги для разных причин:
    // 0-30 units    = попал или очень близко (возможно occlusion)
    // 30-100 units  = spread miss (пуля отклонилась от спреда)
    // 100-200 units = resolver miss (неправильный угол тела)
    // 200+ units    = prediction error или другая причина
    
    if (distance >= 30.0f && distance <= 100.0f && shot.weapon_spread > 0.005f)
    {
        return true; // Spread miss
    }
    
    return false;
}

// ============================================================================
// ПРОВЕРКА УСЛОВИЙ ДЛЯ ВЫСОКОГО СПРЕДА
// ============================================================================

bool check_high_spread_conditions()
{
    auto local = g_ctx->m_local_controller->m_pawn;
    auto weapon = local->get_active_weapon();
    
    if (!local || !weapon)
        return false;
    
    bool high_spread = false;
    
    // 1. Двигается ли игрок?
    float velocity = local->get_velocity().Length2D();
    if (velocity > 5.0f)
    {
        high_spread = true;
        // Можно даже добавить: return "spread (moving)";
    }
    
    // 2. В воздухе ли игрок?
    if (!(local->get_flags() & FL_ONGROUND))
    {
        high_spread = true;
        // Можно: return "spread (jumping)";
    }
    
    // 3. Много ли выстрелов подряд? (рекоил не восстановился)
    if (weapon->get_shots_fired() > 3)
    {
        high_spread = true;
        // Можно: return "spread (recoil)";
    }
    
    // 4. Высокая инаккураси оружия?
    float inaccuracy = weapon->get_inaccuracy();
    if (inaccuracy > 0.02f) // Порог для высокой инаккураси
    {
        high_spread = true;
    }
    
    return high_spread;
}

// ============================================================================
// ПОЛНАЯ ИНТЕГРАЦИЯ В РЕЙДЖБОТ
// ============================================================================

void full_integration_example()
{
    // В CreateMove после стрельбы:
    if (cmd->buttons & IN_ATTACK)
    {
        // Сохраняем данные выстрела
        on_weapon_fire();
    }
    
    // В обработчике события bullet_impact:
    // void on_game_event(IGameEvent* event)
    // {
    //     if (strcmp(event->GetName(), "bullet_impact") == 0)
    //     {
    //         int user_id = event->GetInt("userid");
    //         
    //         // Проверяем что это наш выстрел
    //         if (user_id == local_user_id)
    //         {
    //             Vector impact(
    //                 event->GetFloat("x"),
    //                 event->GetFloat("y"),
    //                 event->GetFloat("z")
    //             );
    //             
    //             on_bullet_impact_event(impact);
    //         }
    //     }
    // }
}

/*
 * ============================================================================
 * КРАТКАЯ ШПАРГАЛКА: КАК ОПРЕДЕЛИТЬ SPREAD MISS
 * ============================================================================
 * 
 * МЕТОД 1 - По расстоянию (самый простой):
 * ----------------------------------------
 * distance = |impact_pos - aim_pos|
 * if (30 < distance < 100) -> spread miss
 * 
 * 
 * МЕТОД 2 - По углу отклонения:
 * ----------------------------------------
 * angle = angle_between(dir_to_aim, dir_to_impact)
 * if (angle < weapon_max_spread * 2.5) -> spread miss
 * 
 * 
 * МЕТОД 3 - По условиям (самый точный):
 * ----------------------------------------
 * ✓ Прицел был на цели (aim правильный)
 * ✓ Пуля отклонилась (distance > threshold)
 * ✓ Weapon inaccuracy высокий (spread/inaccuracy > 0.01)
 * ✓ Игрок двигался ИЛИ в воздухе ИЛИ спамил
 * -> spread miss
 * 
 * 
 * ТИПИЧНЫЕ ЗНАЧЕНИЯ:
 * ----------------------------------------
 * AK-47 standing still:    spread ~0.005-0.01
 * AK-47 moving:            spread ~0.1-0.2
 * AK-47 in air:            spread ~0.3-0.4
 * AWP standing:            spread ~0.001-0.002
 * AWP moving:              spread ~0.05-0.1
 * 
 * 
 * ЧАСТЫЕ ОШИБКИ:
 * ----------------------------------------
 * ❌ Не учитывать weapon->GetInaccuracy()
 * ❌ Не проверять FL_ONGROUND флаг
 * ❌ Не учитывать velocity игрока
 * ❌ Слишком малый timeout (< 0.3s)
 * ❌ Не проверять расстояние до цели
 * 
 * ============================================================================
 */
