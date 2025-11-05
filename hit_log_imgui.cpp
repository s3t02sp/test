#include <imgui.h>
#include <algorithm>
#include <deque>
#include <string>

struct HitEvent {
    std::string attacker;
    std::string victim;
    std::string hit_group; // e.g. "head" or "chest"
    int damage = 0;
    int remaining_hp = 0;
    float timestamp = 0.0f; // seconds since app start or round start
};

class HitLog {
public:
    explicit HitLog(const size_t max_entries = 8, const float lifetime = 4.5f)
        : m_maxEntries(max_entries), m_lifetime(lifetime) {}

    void addEvent(const HitEvent &event) {
        if (m_entries.size() >= m_maxEntries) {
            m_entries.pop_front();
        }
        m_entries.push_back(event);
    }

    void draw(float nowSeconds) {
        pruneExpired(nowSeconds);

        if (m_entries.empty()) {
            return;
        }

        const ImVec2 padding(12.0f, 12.0f);
        const float window_width = 320.0f;

        ImGui::SetNextWindowSize(ImVec2(window_width, 0.0f));
        ImGui::SetNextWindowPos(ImVec2(25.0f, 80.0f), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

        if (ImGui::Begin("##hit_log", nullptr, flags)) {
            for (const auto &entry : m_entries) {
                float age = nowSeconds - entry.timestamp;
                float alpha = 1.0f - (age / m_lifetime);
                alpha = std::clamp(alpha, 0.0f, 1.0f);

                ImVec4 color_header = ImVec4(0.90f, 0.60f, 0.92f, alpha);
                ImVec4 color_accent = ImVec4(0.99f, 0.38f, 0.66f, alpha);
                ImVec4 color_dim = ImVec4(0.75f, 0.75f, 0.75f, alpha);

                ImGui::PushStyleColor(ImGuiCol_Text, color_header);
                ImGui::TextUnformatted("Hit");
                ImGui::SameLine();
                ImGui::TextColored(color_accent, "%s", entry.victim.c_str());
                ImGui::SameLine();
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, color_dim);
                ImGui::Text("for %d hp in %s (%d remaining)", entry.damage, entry.hit_group.c_str(),
                            entry.remaining_hp);
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0.0f, 2.0f));
            }
        }

        ImGui::End();
    }

private:
    void pruneExpired(float nowSeconds) {
        while (!m_entries.empty() && nowSeconds - m_entries.front().timestamp > m_lifetime) {
            m_entries.pop_front();
        }
    }

    std::deque<HitEvent> m_entries;
    size_t m_maxEntries;
    float m_lifetime;
};

// Example usage (call from your game loop):
//
// HitLog hitLog;
//
// // when you register a hit:
// hitLog.addEvent({
//     .attacker = "Player1",
//     .victim = "Конлеп",
//     .hit_group = "head",
//     .damage = 122,
//     .remaining_hp = 0,
//     .timestamp = nowSeconds
// });
//
// // each frame in your ImGui pass:
// hitLog.draw(nowSeconds);
