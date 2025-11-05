// Значения spread для популярных оружий CS2
// Используйте эти данные для калибровки детекта spread miss

#include <map>
#include <string>

// Структура с данными спреда для оружия
struct weapon_spread_data
{
    const char* name;
    float standing_inaccuracy;      // Инаккураси стоя
    float crouching_inaccuracy;     // Инаккураси сидя
    float moving_inaccuracy;        // Инаккураси в движении
    float jumping_inaccuracy;       // Инаккураси в прыжке
    float recoil_multiplier;        // Множитель рекоила
};

// Таблица spread значений для популярных оружий
const weapon_spread_data weapon_spreads[] = 
{
    // Rifles
    {"weapon_ak47",      0.0070f, 0.0035f, 0.1400f, 0.3500f, 1.0f},
    {"weapon_m4a1",      0.0060f, 0.0030f, 0.1200f, 0.3000f, 1.0f},
    {"weapon_m4a1_silencer", 0.0060f, 0.0030f, 0.1200f, 0.3000f, 1.0f},
    {"weapon_aug",       0.0055f, 0.0028f, 0.1100f, 0.2800f, 0.9f},
    {"weapon_sg556",     0.0055f, 0.0028f, 0.1100f, 0.2800f, 0.9f},
    {"weapon_famas",     0.0065f, 0.0033f, 0.1300f, 0.3200f, 1.0f},
    {"weapon_galilar",   0.0065f, 0.0033f, 0.1300f, 0.3200f, 1.0f},
    
    // SMGs
    {"weapon_mp9",       0.0080f, 0.0040f, 0.1000f, 0.2500f, 0.8f},
    {"weapon_mac10",     0.0080f, 0.0040f, 0.1000f, 0.2500f, 0.8f},
    {"weapon_mp7",       0.0075f, 0.0038f, 0.0950f, 0.2400f, 0.8f},
    {"weapon_ump45",     0.0070f, 0.0035f, 0.0900f, 0.2300f, 0.8f},
    {"weapon_p90",       0.0085f, 0.0043f, 0.1100f, 0.2700f, 0.8f},
    
    // Snipers
    {"weapon_awp",       0.0010f, 0.0005f, 0.0500f, 0.2500f, 1.0f},
    {"weapon_ssg08",     0.0020f, 0.0010f, 0.0600f, 0.2000f, 0.9f},
    {"weapon_g3sg1",     0.0030f, 0.0015f, 0.0700f, 0.2200f, 1.0f},
    {"weapon_scar20",    0.0030f, 0.0015f, 0.0700f, 0.2200f, 1.0f},
    
    // Pistols
    {"weapon_deagle",    0.0200f, 0.0100f, 0.2500f, 0.4000f, 1.2f},
    {"weapon_revolver",  0.0250f, 0.0125f, 0.2800f, 0.4500f, 1.3f},
    {"weapon_glock",     0.0100f, 0.0050f, 0.1500f, 0.2500f, 0.7f},
    {"weapon_usp_silencer", 0.0090f, 0.0045f, 0.1400f, 0.2400f, 0.7f},
    {"weapon_p250",      0.0110f, 0.0055f, 0.1600f, 0.2600f, 0.8f},
    {"weapon_fiveseven", 0.0105f, 0.0053f, 0.1550f, 0.2550f, 0.8f},
    {"weapon_tec9",      0.0120f, 0.0060f, 0.1700f, 0.2800f, 0.9f},
    {"weapon_cz75a",     0.0115f, 0.0058f, 0.1650f, 0.2700f, 0.85f},
    
    // Heavy
    {"weapon_nova",      0.0400f, 0.0200f, 0.3000f, 0.5000f, 1.0f},
    {"weapon_xm1014",    0.0450f, 0.0225f, 0.3200f, 0.5200f, 1.0f},
    {"weapon_mag7",      0.0420f, 0.0210f, 0.3100f, 0.5100f, 1.0f},
    {"weapon_sawedoff",  0.0500f, 0.0250f, 0.3500f, 0.5500f, 1.0f},
    {"weapon_m249",      0.0100f, 0.0050f, 0.1800f, 0.3500f, 1.1f},
    {"weapon_negev",     0.0110f, 0.0055f, 0.1900f, 0.3600f, 1.1f},
};

// Получить spread данные для оружия
const weapon_spread_data* get_weapon_spread_data(const char* weapon_name)
{
    for (const auto& data : weapon_spreads)
    {
        if (strcmp(data.name, weapon_name) == 0)
            return &data;
    }
    return nullptr;
}

// Вычислить текущий spread для оружия
float calculate_current_spread(const char* weapon_name)
{
    auto local = g_ctx->m_local_controller->m_pawn;
    auto weapon = local->get_active_weapon();
    
    if (!weapon)
        return 0.0f;
    
    const weapon_spread_data* data = get_weapon_spread_data(weapon_name);
    if (!data)
        return weapon->get_inaccuracy() + weapon->get_spread();
    
    float spread = 0.0f;
    
    // 1. Базовый спред зависит от позиции игрока
    if (!(local->get_flags() & FL_ONGROUND))
    {
        // В воздухе
        spread = data->jumping_inaccuracy;
    }
    else if (local->get_velocity().Length2D() > 5.0f)
    {
        // В движении
        spread = data->moving_inaccuracy;
    }
    else if (local->get_flags() & FL_DUCKING)
    {
        // Сидя
        spread = data->crouching_inaccuracy;
    }
    else
    {
        // Стоя
        spread = data->standing_inaccuracy;
    }
    
    // 2. Добавляем рекоил
    int shots_fired = weapon->get_shots_fired();
    if (shots_fired > 1)
    {
        float recoil_penalty = (shots_fired - 1) * 0.01f * data->recoil_multiplier;
        spread += recoil_penalty;
    }
    
    return spread;
}

// Оптимальные пороги детекта для разных оружий
struct spread_detection_thresholds
{
    float min_distance;     // Минимальное расстояние для spread miss
    float max_distance;     // Максимальное расстояние для spread miss
    float min_spread;       // Минимальный спред для детекта
    float angle_multiplier; // Множитель угла спреда
};

// Получить пороги детекта для типа оружия
spread_detection_thresholds get_detection_thresholds(const char* weapon_name)
{
    spread_detection_thresholds thresholds;
    
    // Снайперские винтовки
    if (strstr(weapon_name, "awp") || strstr(weapon_name, "ssg08") || 
        strstr(weapon_name, "g3sg1") || strstr(weapon_name, "scar20"))
    {
        thresholds.min_distance = 20.0f;
        thresholds.max_distance = 80.0f;
        thresholds.min_spread = 0.001f;
        thresholds.angle_multiplier = 2.0f;
    }
    // Винтовки
    else if (strstr(weapon_name, "ak47") || strstr(weapon_name, "m4a") || 
             strstr(weapon_name, "aug") || strstr(weapon_name, "sg556"))
    {
        thresholds.min_distance = 30.0f;
        thresholds.max_distance = 100.0f;
        thresholds.min_spread = 0.005f;
        thresholds.angle_multiplier = 2.5f;
    }
    // SMG
    else if (strstr(weapon_name, "mp") || strstr(weapon_name, "ump") || 
             strstr(weapon_name, "p90"))
    {
        thresholds.min_distance = 25.0f;
        thresholds.max_distance = 90.0f;
        thresholds.min_spread = 0.006f;
        thresholds.angle_multiplier = 2.3f;
    }
    // Пистолеты
    else if (strstr(weapon_name, "deagle") || strstr(weapon_name, "revolver"))
    {
        thresholds.min_distance = 35.0f;
        thresholds.max_distance = 120.0f;
        thresholds.min_spread = 0.01f;
        thresholds.angle_multiplier = 3.0f;
    }
    else // Другие пистолеты
    {
        thresholds.min_distance = 30.0f;
        thresholds.max_distance = 100.0f;
        thresholds.min_spread = 0.008f;
        thresholds.angle_multiplier = 2.5f;
    }
    
    return thresholds;
}

// Улучшенный детект spread miss с учетом типа оружия
bool is_spread_miss_improved(const shot_data& shot, const Vector& impact_pos, 
                              const char* weapon_name)
{
    // Получаем пороги для данного оружия
    auto thresholds = get_detection_thresholds(weapon_name);
    
    // Расстояние от импакта до точки прицеливания
    float distance = (impact_pos - shot.aim_pos).Length();
    
    // Проверка 1: Расстояние в допустимых пределах
    if (distance < thresholds.min_distance || distance > thresholds.max_distance)
        return false;
    
    // Проверка 2: Спред оружия достаточно высокий
    if (shot.weapon_spread < thresholds.min_spread)
        return false;
    
    // Проверка 3: Угол отклонения
    Vector dir_to_aim = (shot.aim_pos - shot.shoot_pos).Normalized();
    Vector dir_to_impact = (impact_pos - shot.shoot_pos).Normalized();
    
    float dot = dir_to_aim.Dot(dir_to_impact);
    float angle = acosf(dot) * (180.0f / M_PI);
    
    float max_spread_angle = shot.weapon_spread * (180.0f / M_PI) * thresholds.angle_multiplier;
    
    if (angle > max_spread_angle)
        return false;
    
    // ✅ Все проверки пройдены - это spread miss
    return true;
}

// Примеры использования
void examples()
{
    // Пример 1: Получить данные оружия
    {
        const weapon_spread_data* ak47_data = get_weapon_spread_data("weapon_ak47");
        if (ak47_data)
        {
            printf("AK-47 standing inaccuracy: %.4f\n", ak47_data->standing_inaccuracy);
            printf("AK-47 moving inaccuracy: %.4f\n", ak47_data->moving_inaccuracy);
            printf("AK-47 jumping inaccuracy: %.4f\n", ak47_data->jumping_inaccuracy);
        }
    }
    
    // Пример 2: Вычислить текущий spread
    {
        float current_spread = calculate_current_spread("weapon_ak47");
        printf("Current AK-47 spread: %.4f\n", current_spread);
    }
    
    // Пример 3: Детект spread miss с учетом оружия
    {
        shot_data shot;
        // ... заполнить данные ...
        Vector impact_pos;
        
        if (is_spread_miss_improved(shot, impact_pos, "weapon_ak47"))
        {
            g_hit_log.add_miss("enemy", "spread");
        }
    }
}

/*
 * ============================================================================
 * ТАБЛИЦА РЕКОМЕНДУЕМЫХ ПОРОГОВ ДЛЯ SPREAD MISS ДЕТЕКТА
 * ============================================================================
 * 
 * Тип оружия    | min_dist | max_dist | min_spread | angle_mult
 * --------------|----------|----------|------------|------------
 * AWP/Snipers   |   20     |    80    |   0.001    |    2.0
 * AK/M4/Rifles  |   30     |   100    |   0.005    |    2.5
 * MP9/SMGs      |   25     |    90    |   0.006    |    2.3
 * Deagle/Rev    |   35     |   120    |   0.010    |    3.0
 * Pistols       |   30     |   100    |   0.008    |    2.5
 * 
 * 
 * ============================================================================
 * ПРИМЕРЫ SPREAD ЗНАЧЕНИЙ В РАЗНЫХ СИТУАЦИЯХ
 * ============================================================================
 * 
 * AK-47:
 * ------
 * Стоя неподвижно:              0.007
 * Сидя неподвижно:              0.0035
 * Идет медленно:                0.14
 * Бежит полной скоростью:       0.14
 * В прыжке:                     0.35
 * После 1 выстрела:             0.007
 * После 3 выстрелов (спрей):    0.027
 * После 5 выстрелов (спрей):    0.047
 * 
 * AWP:
 * ----
 * Стоя неподвижно:              0.001
 * Сидя неподвижно:              0.0005
 * Идет медленно:                0.05
 * Бежит полной скоростью:       0.05
 * В прыжке:                     0.25
 * 
 * Deagle:
 * -------
 * Стоя неподвижно:              0.02
 * Сидя неподвижно:              0.01
 * Идет медленно:                0.25
 * В прыжке:                     0.40
 * После 1 выстрела:             0.02
 * После 2 выстрелов (спам):     0.032
 * 
 * 
 * ============================================================================
 * КАК ИСПОЛЬЗОВАТЬ ЭТИ ДАННЫЕ
 * ============================================================================
 * 
 * 1. При регистрации выстрела:
 *    shot.weapon_spread = calculate_current_spread(weapon_name);
 * 
 * 2. При детекте miss:
 *    if (is_spread_miss_improved(shot, impact, weapon_name))
 *        log("missed due to spread");
 * 
 * 3. Для калибровки порогов:
 *    - Запишите логи в файл
 *    - Посмотрите какие значения distance/spread у false positives
 *    - Подстройте пороги под ваш чит
 * 
 * 4. Для детальной статистики:
 *    - Сохраняйте weapon_name в shot_data
 *    - Ведите отдельную статистику по оружиям
 *    - Анализируйте какое оружие дает больше spread miss
 */
