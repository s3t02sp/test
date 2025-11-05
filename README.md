# ImGui Hit Log Demo

Демонстрация системы хит лога на C++ с использованием Dear ImGui.

## Особенности

- 🎯 Цветное отображение урона и частей тела
- ⏱️ Автоматическое затухание старых сообщений
- 🎨 Стильный темный интерфейс с подсветкой
- 📊 Отображает: имя цели, урон, часть тела, оставшееся HP

## Зависимости

- Dear ImGui (загружается автоматически)
- GLFW3
- OpenGL 3.0+

## Установка зависимостей

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
```

### Fedora/RHEL
```bash
sudo dnf install glfw-devel mesa-libGL-devel mesa-libGLU-devel
```

### macOS
```bash
brew install glfw
```

## Сборка

```bash
./build.sh
```

Или вручную:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Запуск

```bash
./build/hit_log
```

## Использование

- Хит лог автоматически генерирует тестовые сообщения каждые 2 секунды
- Используйте кнопку "Add Random Hit" для добавления случайного хита вручную
- Сообщения автоматически затухают через 5 секунд

## Интеграция в свой проект

Скопируйте класс `HitLog` из `hit_log.cpp` и используйте:

```cpp
HitLog hitLog;

// Добавить хит
hitLog.AddHit("Враг", 122, "head", 0);

// В главном цикле рендеринга
hitLog.Render();
```

## Структура файлов

- `hit_log.cpp` - Основной код с классом HitLog
- `CMakeLists.txt` - Конфигурация сборки
- `build.sh` - Скрипт автоматической сборки
