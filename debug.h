#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Debug categories
enum DebugCategory {
    CAT_AUDIO,
    CAT_BUTTON,
    CAT_COMMAND,
    CAT_MIDI,
    CAT_SCALE,
    CAT_STATE,
    NUM_CATEGORIES
};

// Debug levels
enum DebugLevel {
    LEVEL_ERROR,
    LEVEL_WARNING,
    LEVEL_INFO,
    LEVEL_DEBUG,
    NUM_LEVELS
};

// Debug macros
#define DEBUG_ERROR(cat, fmt, ...) debugPrint(LEVEL_ERROR, cat, fmt, ##__VA_ARGS__)
#define DEBUG_WARNING(cat, fmt, ...) debugPrint(LEVEL_WARNING, cat, fmt, ##__VA_ARGS__)
#define DEBUG_INFO(cat, fmt, ...) debugPrint(LEVEL_INFO, cat, fmt, ##__VA_ARGS__)
#define DEBUG_DEBUG(cat, fmt, ...) debugPrint(LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__)

// Function declarations
void setupDebug(unsigned long baud);
void debugPrint(DebugLevel level, DebugCategory category, const char* format, ...);
void setDebugLevel(DebugLevel level);
void setDebugCategory(DebugCategory category, bool enabled);

#endif
