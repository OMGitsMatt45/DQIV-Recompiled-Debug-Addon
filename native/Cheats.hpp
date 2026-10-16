#pragma once

namespace pc {
// Optional native cheats; the original game save does not store these switches.
struct CheatSettings {
    bool battleInvincible = false;
    bool noRandomEncounters = false;
    int expMultiplier = 1;
    int goldMultiplier = 1;
};

inline constexpr int cheatMultipliers[] = {1, 2, 4, 8, 16, 32, 64};
inline constexpr int cheatMultiplierCount = 7;

constexpr int normalizeCheatMultiplier(int value) {
    int result = 1;
    for (int multiplier : cheatMultipliers) {
        if (multiplier <= value) result = multiplier;
    }
    return result;
}

constexpr int changeCheatMultiplier(int value, int step) {
    const int normalized = normalizeCheatMultiplier(value);
    int index = 0;
    while (cheatMultipliers[index] != normalized) ++index;
    index += step < 0 ? -1 : step > 0 ? 1 : 0;
    if (index < 0) index = 0;
    if (index >= cheatMultiplierCount) index = cheatMultiplierCount - 1;
    return cheatMultipliers[index];
}

// Wide arithmetic avoids overflowing before the game's normal reward clamp.
// x1 preserves the original behavior, including the original debug modes.
constexpr int scaleCheatReward(int reward, int multiplier, int maximum) {
    const int factor = normalizeCheatMultiplier(multiplier);
    if (factor == 1) return reward;
    const long long scaled = static_cast<long long>(reward) * factor;
    return scaled < 0 ? 0 : scaled > maximum ? maximum : static_cast<int>(scaled);
}

enum CheatAction {
    cheatMaxGold, cheatMaxBank, cheatMaxCasino, cheatMaxMedals,
    cheatRecoverParty, cheatAddItem, cheatReset,
};
enum CheatResult {cheatDone = 0, cheatInvalidItem = -1, cheatBagFull = -2, cheatStackFull = -3};
// Actions run only from the in-game Settings panel. Adding an item returns the
// actual added quantity, or a negative CheatResult when nothing is added.
int runCheatAction(CheatAction action, int itemId = 1, int quantity = 1);
int lastCheatItemId();
void cheatItemName(char* text, int size, int itemId);
const char* cheatActionLabel(CheatAction action, int language);
const char* cheatQuantityLabel(int language);
const char* cheatResultLabel(int result, int language);
}
