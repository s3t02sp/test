# Исправление ошибки линковки: get_hitbox_name

## Проблема
```
ссылка на неразрешенный внешний символ "private: ... c_rage_bot::get_hitbox_name(int)"
```

## Причина
Компилятор видит функцию `get_hitbox_name()` как **private**, хотя она должна быть **public**.

## Решение

### Шаг 1: Полная пересборка проекта

1. **Очистите кэш компилятора:**
   - Visual Studio: `Build → Clean Solution`
   - Затем: `Build → Rebuild Solution`
   
2. **Удалите временные файлы:**
   - Удалите папки: `Debug/`, `Release/`, `x64/`, `.vs/`
   - Удалите все `.obj` и `.pch` файлы

### Шаг 2: Проверьте заголовочный файл

В вашем `rage_bot.hpp` убедитесь, что класс выглядит **ТОЧНО** так:

```cpp
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
    
    // ⬇️ ЭТИ ФУНКЦИИ ДОЛЖНЫ БЫТЬ В PUBLIC СЕКЦИИ ⬇️
    std::string get_hitbox_name(int hitbox);
    std::string get_reason_string(miss_reason_e reason);

private:
    // все остальные приватные функции...
};
```

### Шаг 3: Убедитесь, что нет дубликатов

Проверьте, что в вашем `rage_bot.hpp` **НЕТ** повторных объявлений `get_hitbox_name` в другом месте файла.

Используйте поиск (Ctrl+F) и найдите все вхождения:
- `get_hitbox_name` - должно быть только 1 объявление в public секции

### Шаг 4: Проверьте реализацию

В `rage_bot.cpp` должны быть реализации:

```cpp
std::string c_rage_bot::get_hitbox_name(int hitbox)
{
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

std::string c_rage_bot::get_reason_string(miss_reason_e reason)
{
    switch (reason) {
        case MISS_REASON_SPREAD: return "Spread/Inaccuracy";
        case MISS_REASON_PREDICTION_ERROR: return "Prediction Error";
        case MISS_REASON_RESOLVER: return "Resolver";
        case MISS_REASON_HITBOX: return "Hitbox";
        default: return "Unknown";
    }
}
```

### Шаг 5: Альтернативное решение

Если проблема не решается, замените вызовы в `on_player_hurt`:

**Было:**
```cpp
std::string log = "[SPREAD MISS] Target: " + m_last_shot.m_target_name +
    " | Aimed: " + get_hitbox_name(m_last_shot.m_hitbox) +
    " | Hit: " + get_hitbox_name(hitgroup) + ...
```

**Замените на:**
```cpp
std::string log = "[SPREAD MISS] Target: " + m_last_shot.m_target_name +
    " | Aimed: " + this->get_hitbox_name(m_last_shot.m_hitbox) +
    " | Hit: " + this->get_hitbox_name(hitgroup) + ...
```

### Шаг 6: Проверьте компиляцию только rage_bot.cpp

Попробуйте скомпилировать только `rage_bot.cpp`:
- Если компилируется без ошибок → проблема в порядке линковки
- Если выдаёт ошибку → проблема в заголовочном файле

## Быстрое исправление (если всё остальное не помогло)

Сделайте функции `inline` прямо в классе:

```cpp
class c_rage_bot {
public:
    // ... другие функции ...
    
    // Inline реализация
    inline std::string get_hitbox_name(int hitbox) {
        switch (hitbox) {
            case HITBOX_HEAD: return "Head";
            case HITBOX_CHEST: return "Chest";
            case HITBOX_STOMACH: return "Stomach";
            case HITBOX_PELVIS: return "Pelvis";
            case HITBOX_UPPER_CHEST: return "Upper Chest";
            case HITBOX_LOWER_CHEST: return "Lower Chest";
            // ... остальные case
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
    // ... приватные функции ...
};
```

И **удалите** реализации из `rage_bot.cpp` (строки 794-829).

## Checklist

- [ ] Очищена сборка (Clean Solution)
- [ ] Удалены временные файлы
- [ ] Функции объявлены как public в rage_bot.hpp
- [ ] Функции реализованы в rage_bot.cpp
- [ ] Нет дубликатов объявлений
- [ ] Выполнен Rebuild Solution
- [ ] Если всё ещё ошибка - используйте inline версию

## Если ничего не помогло

Пришлите вывод компилятора полностью, чтобы я мог точнее определить проблему.
