# 🎯 Полное руководство по детекту Spread Miss в CS2

## 📋 Содержание
1. [Что такое Spread Miss](#что-такое-spread-miss)
2. [Как работает Spread](#как-работает-spread)
3. [Методы детекта](#методы-детекта)
4. [Код примеры](#код-примеры)
5. [Визуализация](#визуализация)

---

## Что такое Spread Miss

**Spread Miss** - это промах, который произошел из-за случайного разброса пули (weapon inaccuracy/spread), а не из-за ошибки в прицеливании, resolver'а или предикта.

### Когда происходит:
- 🏃 Стрельба в движении
- 🪂 Стрельба в прыжке/воздухе
- 🎯 Стрельба на большой дистанции
- 🔫 Спам выстрелов (recoil recovery не завершен)

### Когда НЕ происходит:
- ❌ Прицел был не на цели (aim error)
- ❌ Resolver не смог определить угол тела (resolver miss)
- ❌ Предикт движения был неправильный (prediction miss)
- ❌ Пуля попала в стену перед целью (occlusion)

---

## Как работает Spread

В CS2 каждое оружие имеет параметры точности:

```cpp
// Базовая неточность оружия
float base_inaccuracy = weapon->GetInaccuracy();

// Дополнительный спред от движения/прыжков
float additional_spread = weapon->GetSpread();

// Общая неточность
float total_spread = base_inaccuracy + additional_spread;
```

### Формула отклонения пули:

```
                    🎯 Aim Point (куда целимся)
                   /|
                  / |
                 /  | spread_distance
                /   |
               /θ   |
    [Player] -------+
               d         💥 Impact (куда попала пуля)

θ = spread_angle (угол отклонения)
d = distance_to_target
spread_distance = d * tan(θ)
```

### Типичные значения spread:

| Оружие | Стоя | Движение | В воздухе |
|--------|------|----------|-----------|
| AK-47  | 0.007 | 0.14 | 0.35 |
| M4A4   | 0.006 | 0.12 | 0.30 |
| AWP    | 0.001 | 0.05 | 0.25 |
| Deagle | 0.02  | 0.25 | 0.40 |

---

## Методы детекта

### Метод 1: По расстоянию (простой) ⭐ Рекомендуется для начинающих

```cpp
bool is_spread_miss_simple(Vector aim_pos, Vector impact_pos, float weapon_spread)
{
    float distance = (impact_pos - aim_pos).Length();
    
    // Spread miss обычно в пределах 30-100 units
    if (distance >= 30.0f && distance <= 100.0f)
    {
        // Проверяем что у оружия был высокий спред
        if (weapon_spread > 0.005f)
        {
            return true; // Spread miss
        }
    }
    
    return false;
}
```

**Пороги расстояний:**
- `0-30 units` → Попал или occlusion
- `30-100 units` → **SPREAD MISS** ✅
- `100-200 units` → Resolver miss
- `200+ units` → Prediction error

---

### Метод 2: По углу отклонения (средний) ⭐⭐ Более точный

```cpp
bool is_spread_miss_angle(Vector shoot_pos, Vector aim_pos, 
                          Vector impact_pos, float weapon_spread)
{
    // Вектор к цели
    Vector dir_to_aim = (aim_pos - shoot_pos).Normalized();
    
    // Вектор к импакту
    Vector dir_to_impact = (impact_pos - shoot_pos).Normalized();
    
    // Угол между векторами
    float dot = dir_to_aim.Dot(dir_to_impact);
    float angle = acosf(dot) * (180.0f / M_PI); // В градусах
    
    // Максимальный угол спреда оружия
    float max_spread_angle = weapon_spread * (180.0f / M_PI);
    
    // Если отклонение в пределах спреда оружия
    if (angle > 0.1f && angle <= max_spread_angle * 2.5f)
    {
        return true; // Spread miss
    }
    
    return false;
}
```

**Множители для max_spread_angle:**
- `x1.0` - только гарантированные spread miss
- `x2.0` - стандартный порог (рекомендуется)
- `x2.5` - с запасом на погрешность
- `x3.0+` - слишком много false positives

---

### Метод 3: По условиям (продвинутый) ⭐⭐⭐ Самый точный

```cpp
bool is_spread_miss_advanced(shot_data& shot, Vector impact_pos)
{
    // 1. Проверяем что прицел был правильный
    Vector hitbox_pos = get_hitbox_position(shot.target_id, shot.hitbox);
    float angle_to_hitbox = calculate_fov(shot.view_angles, 
                                          angle_to_vector(shot.shoot_pos, hitbox_pos));
    
    bool aimed_correctly = (angle_to_hitbox <= 2.0f); // 2 градуса порог
    
    if (!aimed_correctly)
        return false; // Это не spread miss, а aim error
    
    // 2. Проверяем отклонение пули
    float distance_from_aim = (impact_pos - shot.aim_pos).Length();
    bool bullet_deviated = (distance_from_aim > 20.0f && distance_from_aim < 150.0f);
    
    if (!bullet_deviated)
        return false; // Слишком мало или слишком много
    
    // 3. Проверяем условия высокого спреда
    bool high_spread = check_spread_conditions(shot);
    
    if (!high_spread)
        return false; // Спред был низкий, значит другая причина
    
    // 4. Проверяем что это не occlusion
    bool hit_wall = check_occlusion(shot.shoot_pos, impact_pos);
    
    if (hit_wall)
        return false; // Это occlusion, не spread
    
    // ✅ Все условия выполнены - это spread miss
    return true;
}

bool check_spread_conditions(shot_data& shot)
{
    auto local = get_local_pawn();
    auto weapon = local->get_active_weapon();
    
    // 1. Высокая инаккураси оружия
    if (shot.inaccuracy + shot.spread > 0.01f)
        return true;
    
    // 2. Игрок двигался
    if (local->get_velocity().Length2D() > 5.0f)
        return true;
    
    // 3. Игрок в воздухе
    if (!(local->get_flags() & FL_ONGROUND))
        return true;
    
    // 4. Спам выстрелов (recoil)
    if (weapon->get_shots_fired() > 2)
        return true;
    
    return false;
}
```

---

## Код примеры

### Полная интеграция в рейджбот:

```cpp
class c_spread_detector
{
private:
    struct shot_record
    {
        Vector aim_pos;
        Vector shoot_pos;
        int target_id;
        float time;
        float spread;
    };
    
    std::vector<shot_record> shots;

public:
    // При выстреле
    void on_fire()
    {
        auto local = g_ctx->m_local_controller->m_pawn;
        auto weapon = local->get_active_weapon();
        
        shot_record record;
        record.aim_pos = current_aim_point;
        record.shoot_pos = local->get_eye_position();
        record.target_id = current_target_id;
        record.time = g_interfaces->m_global_vars->m_current_time;
        record.spread = weapon->get_inaccuracy() + weapon->get_spread();
        
        shots.push_back(record);
    }
    
    // При bullet_impact
    void on_impact(Vector impact_pos)
    {
        if (shots.empty())
            return;
        
        shot_record& shot = shots.back();
        
        // Проверяем таймаут
        float now = g_interfaces->m_global_vars->m_current_time;
        if (now - shot.time > 0.5f)
        {
            shots.pop_back();
            return;
        }
        
        // ДЕТЕКТ SPREAD
        float distance = (impact_pos - shot.aim_pos).Length();
        
        if (distance >= 30.0f && distance <= 100.0f && shot.spread > 0.005f)
        {
            // ✅ Spread miss detected!
            g_hit_log.add_miss(get_player_name(shot.target_id), "spread");
        }
        
        shots.pop_back();
    }
};
```

### Использование в event listener:

```cpp
void on_game_event(IGameEvent* event)
{
    const char* name = event->GetName();
    
    if (strcmp(name, "weapon_fire") == 0)
    {
        int user_id = event->GetInt("userid");
        
        if (user_id == get_local_user_id())
        {
            g_spread_detector.on_fire();
        }
    }
    
    if (strcmp(name, "bullet_impact") == 0)
    {
        int user_id = event->GetInt("userid");
        
        if (user_id == get_local_user_id())
        {
            Vector impact(
                event->GetFloat("x"),
                event->GetFloat("y"),
                event->GetFloat("z")
            );
            
            g_spread_detector.on_impact(impact);
        }
    }
}
```

---

## Визуализация

### Схема детекта spread miss:

```
Ситуация 1: SPREAD MISS ✅
===========================

[Player] -----> 🎯 (aim point)
         \
          \___> 💥 (impact)
          
Distance: 50 units
Weapon spread: 0.15 (moving)
Angle: 1.2°

✅ Прицел был на цели
✅ Пуля отклонилась (50 units)
✅ Высокий спред (0.15)
✅ В пределах разумного

РЕЗУЛЬТАТ: "missed enemy due to spread"
```

```
Ситуация 2: RESOLVER MISS ❌
============================

[Player] -----> 🎯 (aim point - где мы думали что тело)
                
                
                💥 (impact - далеко от цели)
                       
                🧍 (real enemy - реальное положение)

Distance: 180 units
Weapon spread: 0.007 (standing)
Angle: 15°

❌ Прицел был не на реальной цели
❌ Слишком большое отклонение
❌ Низкий спред оружия

РЕЗУЛЬТАТ: "missed enemy due to resolver"
```

```
Ситуация 3: OCCLUSION MISS ❌
=============================

[Player] -----> 🎯 (aim point)
         \
          🧱 (wall)
             \
              💥 (impact)

Distance: 15 units
Hit entity: worldspawn

❌ Попали в стену перед целью
❌ Расстояние слишком мало

РЕЗУЛЬТАТ: "missed enemy due to occlusion"
```

---

## Частые вопросы

### Q: Почему spread miss детектится неправильно?
**A:** Проверьте:
1. Правильно ли вы получаете `weapon->GetInaccuracy()`
2. Учитываете ли вы `weapon->GetSpread()`
3. Таймаут достаточно большой (>= 0.3s)
4. Проверяете ли вы что прицел был на цели

### Q: Какой метод выбрать?
**A:** Зависит от опыта:
- **Новичок** → Метод 1 (по расстоянию)
- **Средний** → Метод 2 (по углу)
- **Эксперт** → Метод 3 (по условиям)

### Q: Как отличить spread от resolver miss?
**A:**
- **Spread**: Малое отклонение (30-100 units), высокий weapon spread
- **Resolver**: Большое отклонение (100+ units), низкий weapon spread

### Q: Нужно ли учитывать recoil?
**A:** Да! При спаме выстрелов:
```cpp
if (weapon->get_shots_fired() > 3)
{
    return "spread (recoil)"; // Более детальная причина
}
```

### Q: Как улучшить точность детекта?
**A:**
1. Добавьте проверку velocity игрока
2. Проверяйте FL_ONGROUND флаг
3. Учитывайте расстояние до цели
4. Сохраняйте историю выстрелов
5. Добавьте проверку line of sight

---

## Примеры логов

### Хорошие логи:
```
✅ "missed enemy due to spread"
✅ "missed enemy due to spread (moving)"
✅ "missed enemy due to spread (in air)"
✅ "missed enemy due to spread (recoil)"
```

### Плохие логи:
```
❌ "missed enemy due to unknown"  // Слишком общее
❌ "miss"                          // Не информативно
❌ "spread miss lol"               // Непрофессионально
```

---

## Дополнительные улучшения

### 1. Детальные причины spread:
```cpp
std::string get_detailed_spread_reason()
{
    auto local = get_local_pawn();
    
    if (!(local->get_flags() & FL_ONGROUND))
        return "spread (in air)";
    
    if (local->get_velocity().Length2D() > 50.0f)
        return "spread (moving)";
    
    if (weapon->get_shots_fired() > 3)
        return "spread (recoil)";
    
    return "spread";
}
```

### 2. Визуализация на экране:
```cpp
void draw_spread_visualization()
{
    if (shots.empty())
        return;
    
    auto& last_shot = shots.back();
    
    // Рисуем круг возможного спреда
    float screen_spread = calculate_spread_radius(last_shot.spread);
    
    ImGui::GetBackgroundDrawList()->AddCircle(
        screen_center,
        screen_spread,
        ImColor(255, 255, 0, 100) // Желтый круг
    );
}
```

### 3. Статистика:
```cpp
struct spread_stats
{
    int total_shots = 0;
    int spread_misses = 0;
    int resolver_misses = 0;
    
    float spread_miss_rate()
    {
        return (float)spread_misses / total_shots * 100.0f;
    }
};
```

---

## Полезные ссылки

- `hit_log.h` - Основной хит лог
- `miss_detection.cpp` - Полный детект всех типов miss
- `spread_detection_simple.cpp` - Упрощенная версия детекта

---

**Автор:** GameSense-style Hit Log System  
**Версия:** 1.0  
**Дата:** 2025-11-05
