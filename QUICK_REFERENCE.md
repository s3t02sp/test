# 🚀 Быстрая шпаргалка - Spread Miss Detection

## Минимальный код для детекта spread miss

### Вариант 1: Самый простой (3 строки)

```cpp
float distance = (impact_pos - aim_pos).Length();
float weapon_spread = weapon->get_inaccuracy() + weapon->get_spread();
bool is_spread = (distance >= 30.0f && distance <= 100.0f && weapon_spread > 0.005f);
```

### Вариант 2: С учетом угла (более точный)

```cpp
Vector dir_aim = (aim_pos - shoot_pos).Normalized();
Vector dir_impact = (impact_pos - shoot_pos).Normalized();
float angle = acosf(dir_aim.Dot(dir_impact)) * (180.0f / M_PI);
float max_angle = weapon_spread * (180.0f / M_PI) * 2.5f;
bool is_spread = (angle > 0.1f && angle <= max_angle);
```

### Вариант 3: С проверкой условий (самый точный)

```cpp
// 1. Расстояние отклонения
float distance = (impact_pos - aim_pos).Length();
bool right_distance = (distance >= 30.0f && distance <= 100.0f);

// 2. Высокий спред оружия
float weapon_spread = weapon->get_inaccuracy() + weapon->get_spread();
bool high_spread = (weapon_spread > 0.005f);

// 3. Условия высокого спреда
bool moving = (local->get_velocity().Length2D() > 5.0f);
bool jumping = !(local->get_flags() & FL_ONGROUND);
bool spraying = (weapon->get_shots_fired() > 2);
bool has_spread_conditions = (moving || jumping || spraying);

// Финальная проверка
bool is_spread = (right_distance && high_spread && has_spread_conditions);
```

---

## Полная интеграция (копируй-вставляй)

```cpp
// Структура для хранения выстрела
struct shot_info {
    Vector aim_pos;
    Vector shoot_pos;
    int target_id;
    float time;
    float spread;
};
std::vector<shot_info> shots;

// 1. При выстреле (в CreateMove)
void on_weapon_fire() {
    shot_info shot;
    shot.aim_pos = target_hitbox_position;
    shot.shoot_pos = local->get_eye_position();
    shot.target_id = target->get_index();
    shot.time = g_interfaces->m_global_vars->m_current_time;
    shot.spread = weapon->get_inaccuracy() + weapon->get_spread();
    shots.push_back(shot);
}

// 2. При bullet_impact событии
void on_bullet_impact(Vector impact_pos) {
    if (shots.empty()) return;
    
    shot_info& shot = shots.back();
    
    // Проверка таймаута
    float now = g_interfaces->m_global_vars->m_current_time;
    if (now - shot.time > 0.5f) {
        shots.pop_back();
        return;
    }
    
    // ДЕТЕКТ SPREAD
    float distance = (impact_pos - shot.aim_pos).Length();
    
    if (distance >= 30.0f && distance <= 100.0f && shot.spread > 0.005f) {
        g_hit_log.add_miss(get_player_name(shot.target_id), "spread");
    }
    
    shots.pop_back();
}
```

---

## Настройка порогов под свой чит

### Стандартные пороги (работают в 90% случаев):

```cpp
#define SPREAD_MIN_DISTANCE  30.0f    // Минимум для spread miss
#define SPREAD_MAX_DISTANCE  100.0f   // Максимум для spread miss
#define SPREAD_MIN_VALUE     0.005f   // Минимальный спред оружия
#define SPREAD_ANGLE_MULT    2.5f     // Множитель угла
```

### Если много false positives (детектит не spread как spread):

```cpp
// Увеличь минимальный порог
#define SPREAD_MIN_DISTANCE  40.0f    // Было 30.0f
#define SPREAD_MIN_VALUE     0.008f   // Было 0.005f
```

### Если пропускает реальные spread miss:

```cpp
// Уменьши минимальный порог
#define SPREAD_MIN_DISTANCE  25.0f    // Было 30.0f
#define SPREAD_MAX_DISTANCE  120.0f   // Было 100.0f
#define SPREAD_MIN_VALUE     0.003f   // Было 0.005f
```

---

## Типичные ошибки и исправления

### ❌ Ошибка 1: Все miss детектятся как spread

**Проблема:** Не проверяешь прицел был ли на цели

**Исправление:**
```cpp
// Добавь проверку FOV до хитбокса
Vector hitbox_pos = get_hitbox_position(target_id, hitbox);
float fov = calculate_fov(view_angles, angle_to_pos(shoot_pos, hitbox_pos));

if (fov > 5.0f) {
    return false; // Прицел был не на цели - это не spread
}
```

### ❌ Ошибка 2: Spread miss не детектится в движении

**Проблема:** Не учитываешь GetSpread() - только GetInaccuracy()

**Исправление:**
```cpp
// Всегда используй оба значения
float total_spread = weapon->get_inaccuracy() + weapon->get_spread();
```

### ❌ Ошибка 3: Детект срабатывает на resolver miss

**Проблема:** Слишком большой max_distance порог

**Исправление:**
```cpp
// Уменьши максимальное расстояние
if (distance > 100.0f) {
    return "resolver"; // Слишком большое отклонение
}
```

### ❌ Ошибка 4: Не детектится spread в прыжке

**Проблема:** Не проверяешь FL_ONGROUND флаг

**Исправление:**
```cpp
bool jumping = !(local->get_flags() & FL_ONGROUND);
if (jumping) {
    // В прыжке всегда высокий спред
    return "spread (in air)";
}
```

---

## Дебаг и тестирование

### Вывод debug информации:

```cpp
void debug_spread_detection(const shot_info& shot, const Vector& impact_pos) {
    float distance = (impact_pos - shot.aim_pos).Length();
    
    printf("=== SPREAD DEBUG ===\n");
    printf("Distance: %.2f\n", distance);
    printf("Weapon spread: %.4f\n", shot.spread);
    printf("Velocity: %.2f\n", local->get_velocity().Length2D());
    printf("On ground: %d\n", (local->get_flags() & FL_ONGROUND) != 0);
    printf("Shots fired: %d\n", weapon->get_shots_fired());
    
    // Определяем причину
    if (distance < 30.0f)
        printf("Result: Too close (occlusion?)\n");
    else if (distance > 100.0f)
        printf("Result: Too far (resolver?)\n");
    else if (shot.spread < 0.005f)
        printf("Result: Low spread (aim error?)\n");
    else
        printf("Result: SPREAD MISS ✓\n");
}
```

### Статистика для калибровки:

```cpp
struct spread_stats {
    int total_checks = 0;
    int spread_detected = 0;
    float avg_distance = 0.0f;
    float avg_spread = 0.0f;
    
    void add(float distance, float spread, bool is_spread) {
        total_checks++;
        if (is_spread) spread_detected++;
        avg_distance = (avg_distance * (total_checks - 1) + distance) / total_checks;
        avg_spread = (avg_spread * (total_checks - 1) + spread) / total_checks;
    }
    
    void print() {
        printf("Total checks: %d\n", total_checks);
        printf("Spread detected: %d (%.1f%%)\n", 
               spread_detected, 
               (float)spread_detected / total_checks * 100.0f);
        printf("Avg distance: %.2f\n", avg_distance);
        printf("Avg spread: %.4f\n", avg_spread);
    }
};
```

---

## Чек-лист перед запуском

- [x] Получаю `weapon->GetInaccuracy()` правильно
- [x] Получаю `weapon->GetSpread()` правильно
- [x] Суммирую оба значения для total_spread
- [x] Проверяю что `impact_pos` в world coordinates
- [x] Проверяю что `aim_pos` это позиция хитбокса
- [x] Timeout выстрелов >= 0.3 секунды
- [x] Очищаю shots при death/round end
- [x] Проверяю что bullet_impact это мой выстрел

---

## Полезные константы

```cpp
// Флаги игрока
#define FL_ONGROUND    (1 << 0)
#define FL_DUCKING     (1 << 1)

// Пороги скорости
#define VELOCITY_STOPPED    5.0f    // Считается стоит
#define VELOCITY_WALKING    100.0f  // Идет
#define VELOCITY_RUNNING    250.0f  // Бежит

// Пороги расстояний
#define DISTANCE_OCCLUSION  30.0f   // Близко - возможно стена
#define DISTANCE_SPREAD     100.0f  // Средне - скорее всего spread
#define DISTANCE_RESOLVER   200.0f  // Далеко - скорее всего resolver

// Пороги спреда
#define SPREAD_STANDING     0.01f   // Стоя спокойно
#define SPREAD_MOVING       0.1f    // В движении
#define SPREAD_JUMPING      0.3f    // В прыжке
```

---

## Копируй-вставляй решения

### 1. Базовый детект (без зависимостей)

```cpp
bool is_spread_miss(float distance, float weapon_spread) {
    return (distance >= 30.0f && distance <= 100.0f && weapon_spread > 0.005f);
}
```

### 2. С проверкой движения

```cpp
bool is_spread_miss_with_velocity(float distance, float weapon_spread, float velocity) {
    bool right_distance = (distance >= 30.0f && distance <= 100.0f);
    bool high_spread = (weapon_spread > 0.005f);
    bool was_moving = (velocity > 5.0f);
    return (right_distance && high_spread && was_moving);
}
```

### 3. Полный детект

```cpp
std::string get_miss_reason(float distance, float weapon_spread, 
                            float velocity, bool on_ground) {
    // Очень близко - скорее всего стена
    if (distance < 30.0f)
        return "occlusion";
    
    // Очень далеко - скорее всего resolver
    if (distance > 100.0f)
        return "resolver";
    
    // Низкий спред оружия - не spread
    if (weapon_spread < 0.005f)
        return "prediction error";
    
    // Spread условия
    bool moving = (velocity > 5.0f);
    bool jumping = !on_ground;
    
    if (moving)
        return "spread (moving)";
    if (jumping)
        return "spread (in air)";
    
    return "spread";
}
```

---

## Быстрые тесты

### Тест 1: AK-47 стоя
```
distance = 50 units
weapon_spread = 0.007
Expected: НЕ spread (слишком низкий спред)
```

### Тест 2: AK-47 в движении
```
distance = 50 units
weapon_spread = 0.14
Expected: SPREAD MISS ✓
```

### Тест 3: AWP в прыжке
```
distance = 60 units  
weapon_spread = 0.25
Expected: SPREAD MISS (in air) ✓
```

### Тест 4: Deagle спам
```
distance = 80 units
weapon_spread = 0.032
Expected: SPREAD MISS (recoil) ✓
```

---

## FAQ

**Q: Какой spread threshold использовать?**  
A: 0.005f для большинства оружий, 0.001f для AWP

**Q: Какой timeout для выстрелов?**  
A: 0.3-0.5 секунд оптимально

**Q: Нужно ли учитывать ping?**  
A: Нет, bullet_impact уже учитывает задержку

**Q: Что если пуля вообще не зарегистрировалась?**  
A: Удаляй выстрелы по timeout (0.5s)

**Q: Spread miss или resolver miss?**  
A: Spread = 30-100 units, Resolver = 100+ units
