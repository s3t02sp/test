// Simple Hit Log for ImGui - Header Only
// Usage: Just include this file and call HitLog::Render() in your ImGui loop

#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include <deque>
#include <chrono>
#include <algorithm>

struct HitInfo {
    std::string targetName;
    int damage;
    std::string bodyPart;
    int remainingHP;
    std::chrono::steady_clock::time_point timestamp;
    float alpha;
    
    HitInfo(const std::string& name, int dmg, const std::string& part, int remaining)
        : targetName(name), damage(dmg), bodyPart(part), remainingHP(remaining),
          timestamp(std::chrono::steady_clock::now()), alpha(1.0f) {}
};

class HitLog {
private:
    std::deque<HitInfo> hits;
    size_t maxHits = 10;
    float fadeTime = 5.0f; // seconds
    float fadeOutDuration = 2.0f; // fade out duration in seconds
    
public:
    // Add a new hit to the log
    void AddHit(const std::string& targetName, int damage, const std::string& bodyPart, int remainingHP) {
        hits.emplace_front(targetName, damage, bodyPart, remainingHP);
        if (hits.size() > maxHits) {
            hits.pop_back();
        }
    }
    
    // Render the hit log (call this every frame)
    void Render(bool* p_open = nullptr) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_FirstUseEver);
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
        
        if (!ImGui::Begin("Hit Log", p_open, flags)) {
            ImGui::End();
            return;
        }
        
        auto now = std::chrono::steady_clock::now();
        
        // Update and render hits
        for (auto it = hits.begin(); it != hits.end();) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->timestamp).count() / 1000.0f;
            
            // Calculate fade
            if (elapsed > fadeTime) {
                it->alpha = std::max(0.0f, 1.0f - (elapsed - fadeTime) / fadeOutDuration);
                if (it->alpha <= 0.0f) {
                    it = hits.erase(it);
                    continue;
                }
            } else {
                it->alpha = 1.0f;
            }
            
            RenderHitMessage(*it);
            ++it;
        }
        
        ImGui::End();
    }
    
    // Render with custom position and size
    void RenderAt(ImVec2 pos, ImVec2 size, bool* p_open = nullptr) {
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        Render(p_open);
    }
    
    // Configuration
    void SetMaxHits(size_t max) { maxHits = max; }
    void SetFadeTime(float seconds) { fadeTime = seconds; }
    void SetFadeOutDuration(float seconds) { fadeOutDuration = seconds; }
    
    // Clear all hits
    void Clear() { hits.clear(); }
    
    // Get number of active hits
    size_t GetHitCount() const { return hits.size(); }
    
private:
    void RenderHitMessage(const HitInfo& hit) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6));
        
        // Background for hit message
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Calculate text size for background
        std::string fullText = "Hit " + hit.targetName + " for " + std::to_string(hit.damage) + 
                              " hp in " + hit.bodyPart + " (" + std::to_string(hit.remainingHP) + " remaining)";
        ImVec2 textSize = ImGui::CalcTextSize(fullText.c_str());
        ImVec2 padding(10, 6);
        
        // Dark background with red tint and border
        ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.35f * hit.alpha, 0.12f * hit.alpha, 0.12f * hit.alpha, 0.95f * hit.alpha)
        );
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.6f * hit.alpha, 0.2f * hit.alpha, 0.2f * hit.alpha, hit.alpha)
        );
        
        ImVec2 rectMin(cursorPos.x - padding.x, cursorPos.y - padding.y);
        ImVec2 rectMax(cursorPos.x + textSize.x + padding.x, cursorPos.y + textSize.y + padding.y);
        
        drawList->AddRectFilled(rectMin, rectMax, bgColor, 5.0f);
        drawList->AddRect(rectMin, rectMax, borderColor, 5.0f, 0, 1.5f);
        
        // Render text with colors
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, hit.alpha));
        ImGui::Text("Hit ");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        // Target name in white/bright
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, hit.alpha));
        ImGui::Text("%s", hit.targetName.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, hit.alpha));
        ImGui::Text(" for ");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        // Damage in yellow/orange - prominent
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, hit.alpha));
        ImGui::Text("%d hp", hit.damage);
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, hit.alpha));
        ImGui::Text(" in ");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        // Body part in orange/red
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, hit.alpha));
        ImGui::Text("%s", hit.bodyPart.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        // Remaining HP
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, hit.alpha));
        ImGui::Text(" (");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        
        if (hit.remainingHP > 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, hit.alpha));
            ImGui::Text("%d remaining", hit.remainingHP);
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, hit.alpha));
            ImGui::Text("0 remaining");
            ImGui::PopStyleColor();
        }
        ImGui::SameLine(0, 0);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, hit.alpha));
        ImGui::Text(")");
        ImGui::PopStyleColor();
        
        ImGui::PopStyleVar();
    }
};
