#pragma once
#include <string>
#include <vector>
#include <deque>

// Hit log entry structure
struct hit_log_entry
{
    std::string message;
    float startTime;
    float fadeStartTime;
    float fadeDuration;
    int r, g, b; // Custom color support
    
    hit_log_entry(const std::string& msg, float start, float fadeStart, float fadeDur, int red = 255, int green = 255, int blue = 255)
        : message(msg), startTime(start), fadeStartTime(fadeStart), fadeDuration(fadeDur), r(red), g(green), b(blue)
    {}
};

class c_hit_log
{
public:
    // Add a new hit log entry
    void add_log(const std::string& message, float displayTime = 5.0f, float fadeDuration = 1.0f, int r = 255, int g = 255, int b = 255);
    
    // Add hit log with formatting
    void add_hit(const std::string& targetName, int damage, int health, const std::string& hitbox);
    
    // Add miss log
    void add_miss(const std::string& targetName, const std::string& reason);
    
    // Add kill log
    void add_kill(const std::string& targetName, int damage, const std::string& hitbox);
    
    // Draw all logs (call this every frame)
    void draw_logs();
    
    // Clear all logs
    void clear_logs();
    
    // Set position offset
    void set_position(float x, float y);
    
    // Set maximum number of visible logs
    void set_max_logs(size_t max);

private:
    std::deque<hit_log_entry> hit_logs;
    float startX = 5.0f;
    float startY = 5.0f;
    size_t maxLogs = 10;
};

// Global instance
extern c_hit_log g_hit_log;
