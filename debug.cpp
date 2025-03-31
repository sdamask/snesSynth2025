#include "debug.h"
#include <stdarg.h>

// Global variables
static bool debugEnabled[NUM_CATEGORIES] = {0};  // All categories disabled by default
static DebugLevel currentDebugLevel = LEVEL_INFO;  // Default to INFO level

// Category names for printing
static const char* categoryNames[NUM_CATEGORIES] = {
    "AUDIO",
    "BUTTON",
    "COMMAND",
    "MIDI",
    "SCALE",
    "STATE"
};

// Level names for printing
static const char* levelNames[NUM_LEVELS] = {
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

void setupDebug(unsigned long baud) {
    Serial.begin(baud);
    
    // Enable only essential categories by default
    setDebugCategory(CAT_AUDIO, true);
    setDebugCategory(CAT_BUTTON, true);
    setDebugCategory(CAT_STATE, true);
}

void debugPrint(DebugLevel level, DebugCategory category, const char* format, ...) {
    // Check if debug is enabled for this category and level
    if (!debugEnabled[category] || level > currentDebugLevel) {
        return;
    }
    
    // Print header with level and category
    Serial.printf("[%s][%s] ", levelNames[level], categoryNames[category]);
    
    // Print formatted message
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.println(buffer);
}

void setDebugLevel(DebugLevel level) {
    if (level < NUM_LEVELS) {
        currentDebugLevel = level;
    }
}

void setDebugCategory(DebugCategory category, bool enabled) {
    if (category < NUM_CATEGORIES) {
        debugEnabled[category] = enabled;
    }
}