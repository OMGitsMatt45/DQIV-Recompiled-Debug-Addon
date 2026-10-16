#pragma once
// Small game/renderer boundaries for running native cheat code without ROM
// resources. Production translation units are also checked with their real
// headers; these fixtures do not validate the Windows ABI or graphics.
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <string>
#define ARM
#define THUMB
using Language = int;
inline constexpr int BATTLE_PART = 13;
inline constexpr unsigned PAD_BUTTON_L = 1u << 9, PAD_BUTTON_R = 1u << 8;
inline constexpr unsigned PAD_KEY_LEFT = 1u << 5, PAD_KEY_RIGHT = 1u << 4;
namespace status {
struct HaveStatus {
    bool isPlayer_ = true, isBattleNpc_ = false;
    int hp = 100, mp = 50, exp = 0;
    int hpMax = 100, mpMax = 50;
    int getHp() {return hp;}
    int getHpMax() {return hpMax;}
    int getMpMax() {return mpMax;}
    void setHp(unsigned short value) {hp = value;}
    void setMp(unsigned short value) {mp = value;}
    void addBaseHp(int value) {hp = std::clamp(hp + value, 0, hpMax);}
    void addBaseMp(int value) {mp = std::clamp(mp + value, 0, mpMax);}
};
struct StatusChange {
    bool poisoned = false;
    void clear() {poisoned = false;}
};
struct HaveStatusInfo {
    HaveStatus haveStatus_;
    StatusChange statusChange_;
    struct {bool isEquipment(int) {return false;}} haveEquipment_;
    int noDamage_ = 0, deathChecks = 0;
    int getHp() {return haveStatus_.hp;}
    bool isDeath() {return getHp() == 0;}
    void execDeath() {++deathChecks;}
    void setMegazaruRing(bool) {}
    void setMeganteRing(bool) {}
    void addExp(int value) {haveStatus_.exp += value;}
    void setHp(unsigned short);
    void addBaseHp(int);
    void setMp(unsigned short);
    void addMp(int);
    void recovery();
    void revival();
};
struct PlayerStatus {
    HaveStatusInfo haveStatusInfo_;
    int walkDamageClears = 0;
    void clearWalkDamage() {++walkDamageClears;}
};
struct ItemData {unsigned char index_ = 0, equip_ = 0; unsigned short count_ = 0;};
struct HaveItemSack {
    std::array<ItemData, 162> items{};
    int getItemSortIndex(int id) {
        for (int i = 0; i < int(items.size()); ++i) if (items[i].index_ == id) return i;
        return -1;
    }
    int getMaxCount() {return int(items.size());}
    int getItem(int slot) {return items.at(slot).index_;}
    ItemData* getItemData(int slot) {return &items.at(slot);}
};
struct BasePartyStatus {
    unsigned gold_ = 0, bankMoney_ = 0, casinoCoin_ = 0, medalCoin_ = 0;
    int battleExp_ = 0, battleGold_ = 0;
    HaveItemSack haveItemSack_;
    void setGold(int value) {gold_ = std::clamp(value, 0, 999999);}
    void setBankMoney(unsigned value) {bankMoney_ = std::min(value, 99999000u);}
    void setCasinoCoin(unsigned value) {casinoCoin_ = std::min(value, 999999u);}
    void setMedalCoin(unsigned value) {medalCoin_ = std::min(value, 99u);}
    void addGold(int value) {setGold(int(gold_) + value);}
};
struct PartyStatus : BasePartyStatus {
    struct {unsigned flag_ = 0;} accessFlag_;
    int party_[13] = {1, 2};
    PlayerStatus players[14];
    void setPlayerMode() {accessFlag_.flag_ = 0x200;}
    void setAllPlayerMode() {
        accessFlag_.flag_ = 0x8;
        std::fill(party_, party_ + 13, 0);
        party_[0] = 1; party_[1] = 2; party_[2] = 3;
    }
    int getCount() {return int(std::count_if(party_, party_ + 13, [](int id) {return id != 0;}));}
    PlayerStatus* getPlayerStatus(int index) {return &players[party_[index]];}
    void reflectBattleExp();
    void reflectBattleGold();
};
struct BaseAction {inline static int doubleFlag_ = 0;};
inline PartyStatus g_Party;
struct {int language = 1;} inline g_Game;
}
struct GlobalDQ4 {
    int part = 12;
    int unkfunc_02058114(int value) {return part == value;}
};
inline GlobalDQ4 data_0210bb94;
struct TestStage {int mapUpdates = 0; void updateBattleMap() {++mapUpdates;}} inline g_Stage;
struct TestGlobal {int battles = 0; void startBattle() {++battles;}} inline g_Global;
namespace encount {
struct Encount {
    int brewCount_ = 0, chapter_ = 1, tileId_ = 1, enable_ = 1, brews = 0;
    bool isEncounted();
    bool isEncountedNext();
    void forceBrew(int);
    void forceEventBrew(int);
    void setTileId(int value) {tileId_ = value;}
    bool brew() {++brews; return true;}
};
}
namespace param {
template<class T, int N> struct ExcelFile {unsigned id; T data[N];};
struct ItemData {};
}
inline const param::ExcelFile<param::ItemData, 162> data_020978d8{};
struct TextAPI {
    static void setLanguage(int) {}
    static void extractText(char* text, int size, int, int id) {
        const auto name = "Item " + std::to_string(id);
        std::strncpy(text, name.c_str(), size - 1);
        text[size - 1] = 0;
    }
};
namespace menu {
struct MenuItem {
    int active_ = 0, result_ = 0, lastresult_ = 0;
    void execInput() {}
    void drawActive() {}
};
}
struct UnkMenuParts {int type_, slot_, tag_, a_, b_, c_, w_, h_; int x_ = 0, y_ = 0;};
inline int drawnTabs = 0, drawnText = 0, highestRow = 0;
inline void unkfunc_02050ee0(UnkMenuParts* parts, int*, int, int, int) {
    ++drawnText;
    highestRow = std::max(highestRow, parts[0].y_);
}
inline void unkfunc_native_tabs(const char* const*, int count, int, int, int, void (*)(int,int,int,int,int)) {drawnTabs = count;}
struct MenuTemplate_materiel {
    inline static int rows = 0;
    static void MATERIEL_SETTINGS(menu::MenuItem*, int count, int) {rows = count;}
};
struct TestOption {
    int battle = 0, bgm = 4, se = 4;
    int getBattleSpeed() {return battle;}
    void setBattleSpeed(int value) {battle = value;}
    int getBgmVolume() {return bgm;}
    int getSeVolume() {return se;}
    void setBgmVolume(int value) {bgm = value;}
    void setSeVolume(int value) {se = value;}
};
inline TestOption g_Option;
namespace dss {
struct Pad {unsigned pressed = 0; unsigned edge() {return pressed;}};
inline Pad g_Pad;
}
namespace pc {
inline std::filesystem::path testSaveDirectory;
inline std::filesystem::path saveDirectory() {return testSaveDirectory;}
inline std::string bindingName(int, int, int) {return "Key";}
}
