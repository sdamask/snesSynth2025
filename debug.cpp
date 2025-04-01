#include "debug.h"
#include <stdarg.h>

// Global variables
// static DebugLevel currentDebugLevel = LEVEL_INFO;  // Removed global level
// static bool debugEnabled[CAT_COUNT] = {0}; // Removed enabled flag array

// Define and initialize the per-category debug levels
DebugLevel currentDebugLevel[CAT_COUNT];

// Category names for printing
const char* categoryNames[CAT_COUNT] = {
    "General",
    "Audio",
    "MIDI",
    "Controller",
    "Command",
    "State",
    "Playstyle"
};

// Level names for printing (add OFF and VERBOSE)
static const char* levelNames[NUM_LEVELS] = {
    "OFF",
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG",
    "VERBOSE"
};

void setupDebug(unsigned long baud) {
    Serial.begin(baud);
    while (!Serial && millis() < 4000); // Wait for Serial connection (max 4s)

    // Initialize all categories to a default level (e.g., LEVEL_INFO)
    for (int i = 0; i < CAT_COUNT; i++) {
        currentDebugLevel[i] = LEVEL_INFO; // Default level
    }

    // Optionally set specific categories to different default levels
    // currentDebugLevel[CAT_AUDIO] = LEVEL_DEBUG;
    currentDebugLevel[CAT_MIDI] = LEVEL_VERBOSE; // Set MIDI to verbose by default for now

    Serial.println("Debug system initialized."); // Use Serial.println directly as levels aren't fully set
    // DEBUG_INFO(CAT_GENERAL, "Debug system initialized. Baud: %lu", baud); // Can't use macro yet
}

void debugPrint(DebugLevel level, DebugCategory category, const char* format, ...) {
    // Check level against the specific category's current level
    if (category >= CAT_COUNT || level == LEVEL_OFF || level > currentDebugLevel[category]) {
        return; // Category out of bounds, level is OFF, or category level too low
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

// --- Updated functions to handle per-category levels ---

// Sets the debug level for a SPECIFIC category
void setDebugLevelForCategory(DebugCategory category, DebugLevel level) {
    if (category < CAT_COUNT && level < NUM_LEVELS) {
        currentDebugLevel[category] = level;
        // Use direct print here as the category we are setting might be off
        Serial.printf("[DEBUG] Level for %s set to: %s\n", categoryNames[category], levelNames[level]);
    }
}

// Sets the debug level for ALL categories
void setGlobalDebugLevel(DebugLevel level) {
    if (level < NUM_LEVELS) {
        for (int i = 0; i < CAT_COUNT; i++) {
            currentDebugLevel[i] = level;
        }
        Serial.printf("[DEBUG] Global level set to: %s\n", levelNames[level]);
    }
}

// Remove the old setDebugLevel and setDebugCategory functions
// void setDebugLevel(DebugLevel level) { ... }
// void setDebugCategory(DebugCategory category, bool enabled) { ... }