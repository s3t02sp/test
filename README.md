# ImGui Hit Log Demo

This repository contains a compact C++ widget that reproduces a hit log like the one shown in the screenshot. The widget is designed for integration into an existing Dear ImGui overlay or HUD.

## Getting Started

1. Make sure Dear ImGui is initialised inside your engine or debug overlay.
2. Add `src/imgui_hit_log.cpp` to your build or include it in one of your source files.
3. Instantiate the `HitLogWindow` once (for example as a static variable inside your UI drawing function).

```cpp
static HitLogWindow hitLog;
hitLog.Draw();

// Whenever you register a hit event:
hitLog.PushHitEvent(targetName, damage, hitPart, remainingHp, wasHeadshot);
```

## Customisation

- Call `SetLifetime`, `SetFadeDuration`, and `SetMaxEntries` to tweak how long entries stay on screen.
- Change the colors used inside `PushHitEvent` if you want different styling for headshots versus regular hits.
- Adjust the position by editing the call to `ImGui::SetNextWindowPos` in `HitLogWindow::Draw()`.

## Notes

The widget assumes you are already within an ImGui frame (between `ImGui::NewFrame()` and `ImGui::Render()`). It keeps entries automatically trimmed and fades them out smoothly during the final portion of their lifetime.
