#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <deque>
#include <chrono>

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
    const size_t maxHits = 10;
    const float fadeTime = 5.0f; // seconds
    
public:
    void AddHit(const std::string& targetName, int damage, const std::string& bodyPart, int remainingHP) {
        hits.emplace_front(targetName, damage, bodyPart, remainingHP);
        if (hits.size() > maxHits) {
            hits.pop_back();
        }
    }
    
    void Render() {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("Hit Log", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        
        auto now = std::chrono::steady_clock::now();
        
        // Update and render hits
        for (auto it = hits.begin(); it != hits.end();) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->timestamp).count() / 1000.0f;
            
            // Calculate fade
            if (elapsed > fadeTime) {
                it->alpha = std::max(0.0f, 1.0f - (elapsed - fadeTime) / 2.0f);
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
    
private:
    void RenderHitMessage(const HitInfo& hit) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
        
        // Background for hit message
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        ImVec2 textSize = ImGui::CalcTextSize("Hit XXXXXXXX for XXX hp in XXXXX (XXX remaining)");
        ImVec2 padding(8, 4);
        
        // Dark background with red tint
        ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f * hit.alpha, 0.1f * hit.alpha, 0.1f * hit.alpha, 0.9f * hit.alpha));
        drawList->AddRectFilled(
            ImVec2(cursorPos.x - padding.x, cursorPos.y - padding.y),
            ImVec2(cursorPos.x + textSize.x + padding.x, cursorPos.y + textSize.y + padding.y),
            bgColor,
            4.0f
        );
        
        // Render text with colors
        ImGui::Text("Hit ");
        ImGui::SameLine(0, 0);
        
        // Target name in white
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, hit.alpha), "%s", hit.targetName.c_str());
        ImGui::SameLine(0, 0);
        
        ImGui::Text(" for ");
        ImGui::SameLine(0, 0);
        
        // Damage in yellow/orange
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, hit.alpha), "%d hp", hit.damage);
        ImGui::SameLine(0, 0);
        
        ImGui::Text(" in ");
        ImGui::SameLine(0, 0);
        
        // Body part in orange/red
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, hit.alpha), "%s", hit.bodyPart.c_str());
        ImGui::SameLine(0, 0);
        
        // Remaining HP
        if (hit.remainingHP > 0) {
            ImGui::Text(" (");
            ImGui::SameLine(0, 0);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, hit.alpha), "%d remaining", hit.remainingHP);
            ImGui::SameLine(0, 0);
            ImGui::Text(")");
        } else {
            ImGui::Text(" (");
            ImGui::SameLine(0, 0);
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, hit.alpha), "0 remaining");
            ImGui::SameLine(0, 0);
            ImGui::Text(")");
        }
        
        ImGui::PopStyleVar();
        ImGui::Spacing();
    }
};

// Global hit log instance
HitLog g_HitLog;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Hit Log Demo", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Test data
    float hitTimer = 0.0f;
    const float hitInterval = 2.0f;
    
    std::vector<std::string> targets = {"Концепт", "Враг", "Боец", "Противник", "Цель"};
    std::vector<std::string> bodyParts = {"head", "chest", "stomach", "leg", "arm"};
    int targetIndex = 0;

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Auto-generate test hits
        hitTimer += io.DeltaTime;
        if (hitTimer >= hitInterval) {
            hitTimer = 0.0f;
            int damage = 50 + (rand() % 150);
            int remaining = std::max(0, 100 - damage);
            g_HitLog.AddHit(
                targets[targetIndex % targets.size()],
                damage,
                bodyParts[rand() % bodyParts.size()],
                remaining
            );
            targetIndex++;
        }

        // Render hit log
        g_HitLog.Render();

        // Control panel
        ImGui::SetNextWindowPos(ImVec2(10, 350), ImGuiCond_FirstUseEver);
        ImGui::Begin("Controls");
        ImGui::Text("Hit log demo");
        ImGui::Separator();
        ImGui::Text("New hit every %.1f seconds", hitInterval);
        
        if (ImGui::Button("Add Random Hit")) {
            int damage = 50 + (rand() % 150);
            int remaining = std::max(0, 100 - damage);
            g_HitLog.AddHit(
                targets[rand() % targets.size()],
                damage,
                bodyParts[rand() % bodyParts.size()],
                remaining
            );
        }
        
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, 
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
