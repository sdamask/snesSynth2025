#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Debug categories
enum DebugCategory {
    CAT_GENERAL,
    CAT_AUDIO,
    CAT_MIDI,
    CAT_CONTROLLER,
    CAT_COMMAND,
    CAT_STATE,
    CAT_PLAYSTYLE,
    CAT_COUNT
};

// Debug levels
enum DebugLevel {
    LEVEL_OFF,
    LEVEL_ERROR,
    LEVEL_WARNING,
    LEVEL_INFO,
    LEVEL_DEBUG,
    LEVEL_VERBOSE,
    NUM_LEVELS
};

// Extern declaration for current levels
extern DebugLevel currentDebugLevel[CAT_COUNT];
extern const char* categoryNames[CAT_COUNT];

// Debug macros
#define DEBUG_ERROR(cat, fmt, ...) do { if (currentDebugLevel[cat] >= LEVEL_ERROR) debugPrint(LEVEL_ERROR, cat, fmt, ##__VA_ARGS__); } while(0)
#define DEBUG_WARNING(cat, fmt, ...) do { if (currentDebugLevel[cat] >= LEVEL_WARNING) debugPrint(LEVEL_WARNING, cat, fmt, ##__VA_ARGS__); } while(0)
#define DEBUG_INFO(cat, fmt, ...) do { if (currentDebugLevel[cat] >= LEVEL_INFO) debugPrint(LEVEL_INFO, cat, fmt, ##__VA_ARGS__); } while(0)
#define DEBUG_DEBUG(cat, fmt, ...) do { if (currentDebugLevel[cat] >= LEVEL_DEBUG) debugPrint(LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__); } while(0)
#define DEBUG_VERBOSE(cat, fmt, ...) do { if (currentDebugLevel[cat] >= LEVEL_VERBOSE) debugPrint(LEVEL_VERBOSE, cat, fmt, ##__VA_ARGS__); } while(0)

// Function declarations
void setupDebug(unsigned long baud);
void debugPrint(DebugLevel level, DebugCategory category, const char* format, ...);
void setGlobalDebugLevel(DebugLevel level);
void setDebugLevelForCategory(DebugCategory category, DebugLevel level);

#endif
