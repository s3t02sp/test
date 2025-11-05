// Example of how to use HitLog in your existing ImGui project
// Just include hit_log_simple.h and you're ready to go!

#include "hit_log_simple.h"
// Your other includes...

// Create a global or member instance
HitLog g_hitLog;

void YourGameLoop() {
    // ... your ImGui initialization ...
    
    while (running) {
        // ImGui frame start
        ImGui::NewFrame();
        
        // Render your UI...
        
        // Render hit log
        g_hitLog.Render();
        
        // Or render at specific position
        // g_hitLog.RenderAt(ImVec2(100, 100), ImVec2(400, 200));
        
        // Add hits when damage occurs in your game
        // g_hitLog.AddHit("Enemy", 75, "chest", 25);
        
        // ImGui frame end
        ImGui::Render();
        // ... rendering code ...
    }
}

// Example: When player hits enemy
void OnPlayerAttack(const std::string& enemyName, int damage, const std::string& hitLocation) {
    int enemyCurrentHP = 100; // Get from your game logic
    int remainingHP = enemyCurrentHP - damage;
    
    g_hitLog.AddHit(enemyName, damage, hitLocation, remainingHP);
}

// Example usage in main game code:
void GameExample() {
    // Configure the hit log (optional)
    g_hitLog.SetMaxHits(15);           // Show max 15 hits
    g_hitLog.SetFadeTime(4.0f);        // Start fading after 4 seconds
    g_hitLog.SetFadeOutDuration(1.5f); // Take 1.5 seconds to fade out
    
    // Examples of adding hits:
    g_hitLog.AddHit("Концепт", 122, "head", 0);        // Critical hit - killed
    g_hitLog.AddHit("Враг", 45, "chest", 55);          // Body shot
    g_hitLog.AddHit("Боец", 30, "leg", 70);            // Leg shot
    g_hitLog.AddHit("Противник", 89, "stomach", 11);   // Heavy hit
    
    // Clear all hits if needed
    // g_hitLog.Clear();
}
