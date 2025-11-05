#pragma once
#include "rage_bot.hpp"

// Example utility class for analyzing spread-related misses
// This demonstrates how to use the miss log system to improve aim

class c_spread_analyzer {
public:
    struct spread_stats_t {
        int total_shots = 0;
        int total_misses = 0;
        int spread_misses = 0;
        int resolver_misses = 0;
        int prediction_misses = 0;
        int wrong_hitbox_hits = 0;
        
        float avg_spread = 0.f;
        float avg_inaccuracy = 0.f;
        float avg_hit_chance = 0.f;
        
        float hit_rate() const {
            if (total_shots == 0) return 100.f;
            return (1.f - (float)total_misses / total_shots) * 100.f;
        }
        
        float spread_miss_rate() const {
            if (total_misses == 0) return 0.f;
            return ((float)spread_misses / total_misses) * 100.f;
        }
    };
    
    // Analyze all miss logs and generate statistics
    spread_stats_t analyze_misses() {
        spread_stats_t stats;
        
        auto& miss_logs = g_rage_bot->get_miss_logs();
        
        stats.total_misses = miss_logs.size();
        
        float total_spread = 0.f;
        float total_inaccuracy = 0.f;
        float total_hit_chance = 0.f;
        
        for (const auto& miss : miss_logs) {
            const auto& shot = miss.m_shot_info;
            
            // Count by reason
            switch (miss.m_reason) {
                case MISS_REASON_SPREAD:
                    stats.spread_misses++;
                    break;
                case MISS_REASON_RESOLVER:
                    stats.resolver_misses++;
                    break;
                case MISS_REASON_PREDICTION_ERROR:
                    stats.prediction_misses++;
                    break;
            }
            
            if (miss.m_did_hit)
                stats.wrong_hitbox_hits++;
            
            // Sum for averages
            total_spread += shot.m_spread;
            total_inaccuracy += shot.m_inaccuracy;
            total_hit_chance += shot.m_hit_chance;
        }
        
        // Calculate averages
        if (stats.total_misses > 0) {
            stats.avg_spread = total_spread / stats.total_misses;
            stats.avg_inaccuracy = total_inaccuracy / stats.total_misses;
            stats.avg_hit_chance = total_hit_chance / stats.total_misses;
        }
        
        return stats;
    }
    
    // Get recommended settings based on miss analysis
    void get_recommendations(const spread_stats_t& stats) {
        g_console->print("=== SPREAD ANALYSIS RECOMMENDATIONS ===");
        
        // Too many spread misses
        if (stats.spread_miss_rate() > 50.f) {
            g_console->print("[!] High spread miss rate: %.1f%%", stats.spread_miss_rate());
            g_console->print("    -> Increase hit chance requirement (current: %d%%)", 
                g_cfg->rage_bot.m_hit_chance);
            g_console->print("    -> Enable auto-stop for better accuracy");
            g_console->print("    -> Use static scale for hitbox multi-points");
            
            if (stats.avg_spread > 0.015f) {
                g_console->print("    -> Average spread is high (%.4f), consider:");
                g_console->print("       * Only shooting when stationary");
                g_console->print("       * Waiting for scope (if sniper)");
                g_console->print("       * Avoiding rapid fire");
            }
        }
        
        // Resolver issues
        if (stats.resolver_misses > stats.spread_misses) {
            g_console->print("[!] Resolver issues detected");
            g_console->print("    -> Consider targeting body instead of head");
            g_console->print("    -> Improve anti-aim resolver");
        }
        
        // Prediction issues
        if (stats.prediction_misses > 5) {
            g_console->print("[!] Prediction errors detected");
            g_console->print("    -> Check lag compensation settings");
            g_console->print("    -> Verify backtrack record selection");
        }
        
        // Wrong hitbox hits
        if (stats.wrong_hitbox_hits > 3) {
            g_console->print("[!] Multiple wrong hitbox hits");
            g_console->print("    -> Spread is affecting shot placement");
            g_console->print("    -> Adjust multi-point scale for affected hitboxes");
        }
        
        // Overall good performance
        if (stats.hit_rate() > 80.f) {
            g_console->print("[✓] Good hit rate: %.1f%%", stats.hit_rate());
        } else if (stats.hit_rate() > 60.f) {
            g_console->print("[~] Moderate hit rate: %.1f%%", stats.hit_rate());
        } else {
            g_console->print("[X] Low hit rate: %.1f%%", stats.hit_rate());
            g_console->print("    -> Review all settings and check logs");
        }
        
        g_console->print("======================================");
    }
    
    // Detailed spread analysis for specific hitbox
    void analyze_hitbox_spread(int hitbox) {
        auto& miss_logs = g_rage_bot->get_miss_logs();
        
        int hitbox_misses = 0;
        int hitbox_spread_misses = 0;
        float total_spread = 0.f;
        float max_spread = 0.f;
        float min_spread = FLT_MAX;
        
        for (const auto& miss : miss_logs) {
            if (miss.m_shot_info.m_hitbox == hitbox) {
                hitbox_misses++;
                
                if (miss.m_reason == MISS_REASON_SPREAD)
                    hitbox_spread_misses++;
                
                float spread = miss.m_shot_info.m_spread;
                total_spread += spread;
                
                if (spread > max_spread)
                    max_spread = spread;
                if (spread < min_spread)
                    min_spread = spread;
            }
        }
        
        if (hitbox_misses > 0) {
            float avg_spread = total_spread / hitbox_misses;
            float spread_miss_percent = ((float)hitbox_spread_misses / hitbox_misses) * 100.f;
            
            std::string hitbox_name = g_rage_bot->get_hitbox_name(hitbox);
            
            g_console->print("=== %s SPREAD ANALYSIS ===", hitbox_name.c_str());
            g_console->print("Total misses: %d", hitbox_misses);
            g_console->print("Spread misses: %d (%.1f%%)", hitbox_spread_misses, spread_miss_percent);
            g_console->print("Average spread: %.4f", avg_spread);
            g_console->print("Min spread: %.4f", min_spread);
            g_console->print("Max spread: %.4f", max_spread);
            
            if (spread_miss_percent > 60.f) {
                g_console->print("[!] High spread miss rate for this hitbox");
                g_console->print("    -> Consider disabling or adjusting multi-point scale");
            }
        }
    }
    
    // Print detailed report
    void print_report() {
        auto stats = analyze_misses();
        
        g_console->print("\n");
        g_console->print("╔════════════════════════════════════════╗");
        g_console->print("║    SPREAD MISS LOG ANALYSIS REPORT    ║");
        g_console->print("╚════════════════════════════════════════╝");
        g_console->print("");
        g_console->print("Total Misses: %d", stats.total_misses);
        g_console->print("Hit Rate: %.1f%%", stats.hit_rate());
        g_console->print("");
        g_console->print("Miss Breakdown:");
        g_console->print("  • Spread/Inaccuracy: %d (%.1f%%)", 
            stats.spread_misses, stats.spread_miss_rate());
        g_console->print("  • Resolver: %d (%.1f%%)", 
            stats.resolver_misses, 
            stats.total_misses > 0 ? (float)stats.resolver_misses / stats.total_misses * 100.f : 0.f);
        g_console->print("  • Prediction: %d (%.1f%%)", 
            stats.prediction_misses,
            stats.total_misses > 0 ? (float)stats.prediction_misses / stats.total_misses * 100.f : 0.f);
        g_console->print("  • Wrong Hitbox Hits: %d", stats.wrong_hitbox_hits);
        g_console->print("");
        g_console->print("Average Values:");
        g_console->print("  • Spread: %.4f", stats.avg_spread);
        g_console->print("  • Inaccuracy: %.4f", stats.avg_inaccuracy);
        g_console->print("  • Hit Chance: %.1f%%", stats.avg_hit_chance);
        g_console->print("");
        
        get_recommendations(stats);
        
        g_console->print("\n");
    }
    
    // Auto-adjust settings based on miss analysis
    void auto_optimize() {
        auto stats = analyze_misses();
        
        // Need at least 10 misses to make adjustments
        if (stats.total_misses < 10)
            return;
        
        bool made_changes = false;
        
        // Increase hit chance if too many spread misses
        if (stats.spread_miss_rate() > 60.f && g_cfg->rage_bot.m_hit_chance < 90) {
            g_cfg->rage_bot.m_hit_chance = min(g_cfg->rage_bot.m_hit_chance + 5, 90);
            g_console->print("[AUTO] Increased hit chance to %d%%", g_cfg->rage_bot.m_hit_chance);
            made_changes = true;
        }
        
        // Enable static scale if spread is high
        if (stats.avg_spread > 0.02f && !g_cfg->rage_bot.m_static_scale) {
            g_cfg->rage_bot.m_static_scale = true;
            g_console->print("[AUTO] Enabled static scale due to high spread");
            made_changes = true;
        }
        
        // Switch to body aim if many resolver misses
        if (stats.resolver_misses > stats.spread_misses * 2) {
            if (g_cfg->rage_bot.m_hitboxes[0]) { // Head enabled
                g_cfg->rage_bot.m_hitboxes[0] = false;
                g_cfg->rage_bot.m_hitboxes[1] = true; // Enable chest
                g_console->print("[AUTO] Disabled head aim, enabled chest due to resolver issues");
                made_changes = true;
            }
        }
        
        if (made_changes) {
            g_console->print("[AUTO] Settings optimized based on miss analysis");
            // Clear logs after optimization
            g_rage_bot->clear_miss_logs();
        }
    }
};

inline c_spread_analyzer* g_spread_analyzer = new c_spread_analyzer();

// Example usage in your code:
/*
// Print analysis report
if (key_pressed(VK_F8)) {
    g_spread_analyzer->print_report();
}

// Auto-optimize settings
if (g_cfg->rage_bot.m_auto_optimize) {
    static float last_optimize = 0.f;
    float current_time = g_interfaces->m_globals->m_curtime;
    
    // Optimize every 30 seconds
    if (current_time - last_optimize > 30.f) {
        g_spread_analyzer->auto_optimize();
        last_optimize = current_time;
    }
}

// Analyze specific hitbox
if (key_pressed(VK_F9)) {
    g_spread_analyzer->analyze_hitbox_spread(HITBOX_HEAD);
}
*/
