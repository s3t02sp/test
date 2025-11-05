# CS2 Legitbot Implementation

A complete legitbot implementation for Counter-Strike 2 based on existing rage bot architecture.

## 📁 Files Included

- **`legit_bot.hpp`** - Main header file with class definitions
- **`legit_bot.cpp`** - Core implementation with all features
- **`legit_bot_config.hpp`** - Configuration structure 
- **`usage_example.cpp`** - Integration examples and advanced features
- **`LEGITBOT_README.md`** - Complete documentation

## ✨ Features

### 🎯 Core Features
- **Smooth Aiming** - Natural, gradual aim movement
- **FOV Limiting** - Only targets within crosshair proximity
- **Trigger Bot** - Auto-fire when on target
- **Humanizer** - Reaction delays and aim imperfections

### 🔧 Additional Features
- Team check
- Visibility check
- Recoil control system
- Configurable hitbox selection
- Key bind support
- Weapon-specific configurations

## 🚀 Quick Start

### 1. Add Files to Project
Copy the legitbot files to your project structure:
```
your_project/
├── features/
│   └── aimbot/
│       ├── legit_bot.hpp
│       └── legit_bot.cpp
```

### 2. Include Header
```cpp
#include "legit_bot.hpp"
```

### 3. Call in CreateMove
```cpp
void hooked_create_move(c_user_cmd* user_cmd) {
    g_legit_bot->on_create_move(user_cmd);
}
```

### 4. Configure Settings
```cpp
g_cfg->legit_bot.m_enabled = true;
g_cfg->legit_bot.m_fov = 5.0f;
g_cfg->legit_bot.m_smooth = 10.0f;
g_cfg->legit_bot.m_humanizer = true;
```

## ⚙️ Configuration Guide

### Recommended Settings (Legit)
```cpp
FOV: 3-5 degrees
Smooth: 15-25
Humanizer: Enabled
Reaction Time: 150-250ms
Aim Shake: 0.08-0.15
```

### Aggressive Settings
```cpp
FOV: 8-12 degrees
Smooth: 5-8
Humanizer: Enabled
Reaction Time: 50-100ms
Aim Shake: 0.03-0.05
```

## 📖 Documentation

See **`LEGITBOT_README.md`** for:
- Detailed feature explanations
- Configuration examples
- Troubleshooting guide
- Security considerations
- Advanced customization

See **`usage_example.cpp`** for:
- Integration examples
- UI/Menu implementation
- Weapon-specific configs
- Debug visualization
- Configuration presets

## 🔑 Key Differences from Rage Bot

| Feature | Rage Bot | Legit Bot |
|---------|----------|-----------|
| Aim Speed | Instant snap | Smooth interpolation |
| Target Selection | Best damage | Closest to crosshair (FOV) |
| Shooting | Always auto-fire | Trigger bot with delay |
| Visibility | Ignores (walls) | Respects visibility |
| Detection Risk | Very High | Low (with proper config) |

## 🎮 Usage Tips

1. **Start Conservative** - Use safe presets and adjust gradually
2. **Enable Humanizer** - Always use reaction time and aim shake
3. **Use Key Binds** - Don't leave always-on in competitive
4. **Vary Settings** - Change config between games
5. **Weapon-Specific** - Different weapons need different settings

## ⚠️ Important Notes

- This is a game cheat and will result in bans if detected
- Use at your own risk
- For educational purposes only
- Test in safe environments first
- Never use in VAC-secured servers if you care about your account

## 🔨 Integration Checklist

- [ ] Add files to project
- [ ] Include headers
- [ ] Add config structure
- [ ] Call in CreateMove hook
- [ ] Test with conservative settings
- [ ] Add UI/menu controls
- [ ] Implement key binds
- [ ] Test all features
- [ ] Add weapon-specific configs (optional)
- [ ] Add debug visualization (optional)

## 🐛 Common Issues

**Aim not working?**
- Check if enabled in config
- Verify key bind is pressed
- Ensure FOV is large enough
- Check team/visibility settings

**Trigger not firing?**
- Verify trigger_bot is enabled
- Check trigger_fov setting
- Ensure weapon has ammo
- Check trigger_delay value

**Aim looks robotic?**
- Enable humanizer
- Increase smooth value
- Add aim shake
- Increase reaction time

## 📝 License

Use at your own risk. For educational purposes only.

---

**Based on the rage bot architecture - Modified for legitimate gameplay appearance**
