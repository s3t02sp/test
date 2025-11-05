// Example ImGui hit log widget that renders entries similar to the provided screenshot.
//
// Usage inside your ImGui frame:
//   static HitLogWindow hitLog;
//   hitLog.Draw();
//   // When a new hit happens:
//   hitLog.PushHitEvent("Kошелек", 122, "head", 0, true);
//
// The widget keeps the newest entries for a configurable amount of time,
// fades them out smoothly, and positions the window near the top-right corner.

#include "imgui.h"

#include <algorithm>
#include <deque>
#include <sstream>
#include <string>

struct HitLogEntry
{
    std::string text;
    ImVec4 color;
    double createdAt = 0.0;
};

class HitLogWindow
{
public:
    void PushHitEvent(const std::string& targetName,
                      int damage,
                      const std::string& hitPart,
                      int remainingHp,
                      bool isHeadshot)
    {
        std::ostringstream builder;
        builder << "Hit " << targetName
                << " for " << damage << " hp in " << hitPart
                << " (" << remainingHp << " remaining)";

        HitLogEntry entry;
        entry.text = builder.str();
        entry.color = isHeadshot ? ImVec4(1.0f, 0.60f, 0.86f, 1.0f)
                                 : ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        entry.createdAt = ImGui::GetTime();

        entries_.push_front(entry);

        while (entries_.size() > maxEntries_)
        {
            entries_.pop_back();
        }
    }

    void SetLifetime(float seconds) { lifetime_ = seconds; }
    void SetFadeDuration(float seconds) { fadeDuration_ = seconds; }
    void SetMaxEntries(std::size_t count) { maxEntries_ = count; }

    void Draw()
    {
        if (entries_.empty())
            return;

        const double now = ImGui::GetTime();

        // Remove expired entries and compute max width for auto-resize window.
        float widestText = 0.0f;
        for (auto it = entries_.begin(); it != entries_.end();) {
            const float age = static_cast<float>(now - it->createdAt);
            if (age > lifetime_) {
                it = entries_.erase(it);
                continue;
            }

            widestText = std::max(widestText, ImGui::CalcTextSize(it->text.c_str()).x);
            ++it;
        }

        if (entries_.empty())
            return;

        const ImGuiIO& io = ImGui::GetIO();
        const float padding = 12.0f;

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - padding, padding * 4.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.12f);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoInputs |
                                       ImGuiWindowFlags_NoNav |
                                       ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("Hit Log", nullptr, windowFlags))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));

            for (const HitLogEntry& entry : entries_)
            {
                const float age = static_cast<float>(now - entry.createdAt);
                float alpha = 1.0f;
                if (lifetime_ > fadeDuration_ && age > lifetime_ - fadeDuration_)
                {
                    const float t = (age - (lifetime_ - fadeDuration_)) / fadeDuration_;
                    alpha = std::clamp(1.0f - t, 0.0f, 1.0f);
                }

                ImVec4 color = entry.color;
                color.w *= alpha;

                ImGui::TextColored(color, "%s", entry.text.c_str());
            }

            ImGui::PopStyleVar();
        }

        ImGui::End();
    }

private:
    std::deque<HitLogEntry> entries_;
    std::size_t maxEntries_ = 6;
    float lifetime_ = 4.0f;      // Seconds an entry stays fully visible.
    float fadeDuration_ = 0.7f;  // Seconds spent fading out at the end of lifetime.
};

// Example of feeding the log. In your actual game logic hook, you might call:
//   static HitLogWindow hitLog;
//   if (ImGui::Begin("Overlay")) { hitLog.Draw(); }
//   ImGui::End();
//
//   // On hit event:
//   hitLog.PushHitEvent(playerName, damage, hitboxName, remainingHp, wasHeadshot);

