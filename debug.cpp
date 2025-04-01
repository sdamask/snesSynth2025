#include "debug.h"
#include <stdarg.h>

// Global variables
static DebugLevel currentDebugLevel = LEVEL_INFO;  // Default to INFO level
static bool debugEnabled[CAT_COUNT] = {0};  // Use CAT_COUNT

// Category names for printing
static const char* categoryNames[CAT_COUNT] = {
    "General",
    "Audio",
    "MIDI",
    "Controller",
    "Command",
    "State",
    "Playstyle"
};

// Level names for printing
static const char* levelNames[NUM_LEVELS] = {
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG"
};

void setupDebug(unsigned long baud) {
    Serial.begin(baud);
    while (!Serial && millis() < 4000); // Wait for Serial connection (max 4s)
    
    // Enable specific categories by default (optional)
    setDebugCategory(CAT_GENERAL, true);
    setDebugCategory(CAT_AUDIO, true);
    setDebugCategory(CAT_MIDI, true);
    setDebugCategory(CAT_CONTROLLER, true); // Changed from CAT_BUTTON
    setDebugCategory(CAT_COMMAND, true);
    setDebugCategory(CAT_STATE, true);
    setDebugCategory(CAT_PLAYSTYLE, true); 

    DEBUG_INFO(CAT_GENERAL, "Debug system initialized. Baud: %lu", baud);
}

void debugPrint(DebugLevel level, DebugCategory category, const char* format, ...) {
    if (category >= CAT_COUNT || !debugEnabled[category] || level > currentDebugLevel) {
        return; // Category out of bounds, disabled, or level too low
    }
    
    char buffer[256]; // Buffer for formatted message
    va_list args;
    va_start(args, format);
    
    // Format the main message
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    // Print level, category, and message
    Serial.printf("[%s][%s] %s\n", levelNames[level], categoryNames[category], buffer);
    Serial.flush(); // Ensure message is sent immediately
}

void setDebugLevel(DebugLevel level) {
    if (level < NUM_LEVELS) {
        currentDebugLevel = level;
        DEBUG_INFO(CAT_GENERAL, "Debug level set to: %s", levelNames[level]);
    }
}

void setDebugCategory(DebugCategory category, bool enabled) {
    if (category < CAT_COUNT) { // Use CAT_COUNT
        debugEnabled[category] = enabled;
        // Optional: Print confirmation
        // DEBUG_INFO(CAT_GENERAL, "Debug category '%s' %s", categoryNames[category], enabled ? "enabled" : "disabled");
    }
}