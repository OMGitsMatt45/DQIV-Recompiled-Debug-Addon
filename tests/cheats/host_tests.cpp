#include "HostGame.hpp"
#include "Settings.hpp"
#include "ov016/MenuSettings/SettingsPanel.hpp"
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {int checks = 0;}
#define CHECK(expr) do {if (!(expr)) throw std::runtime_error(#expr); ++checks;} while (0)
void frame(int, int, int, int, int) {}
bool input(SettingsPanel& panel, menu::MenuItem& item, unsigned keys = 0, int result = 0) {
    dss::g_Pad.pressed = keys;
    item.result_ = result;
    int redraw = 0;
    const bool leave = panel.update(item, redraw);
    dss::g_Pad.pressed = 0;
    return leave;
}
int rowIndex(const SettingsPanel& panel, int row) {
    for (int i = 0; i < panel.rowCount_; ++i) if (panel.rows_[i] == row) return i;
    throw std::runtime_error("Menu row missing");
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    pc::testSaveDirectory = argv[1];
    std::filesystem::create_directories(pc::saveDirectory());
    auto& settings = pc::gameSettings();
    CHECK(!settings.cheats.battleInvincible && !settings.cheats.noRandomEncounters);
    CHECK(settings.cheats.expMultiplier == 1 && settings.cheats.goldMultiplier == 1);
    CHECK(settings.centredCamera);
    for (int multiplier : pc::cheatMultipliers) {
        CHECK(pc::normalizeCheatMultiplier(multiplier) == multiplier);
        CHECK(pc::scaleCheatReward(25, multiplier, 0xffffff) == 25 * multiplier);
    }
    CHECK(pc::normalizeCheatMultiplier(INT_MIN) == 1);
    CHECK(pc::normalizeCheatMultiplier(INT_MAX) == 64);
    CHECK(pc::normalizeCheatMultiplier(7) == 4);
    CHECK(pc::changeCheatMultiplier(1, -1) == 1);
    CHECK(pc::changeCheatMultiplier(64, 1) == 64);
    CHECK(pc::scaleCheatReward(INT_MAX, 64, 999999) == 999999);
    CHECK(pc::scaleCheatReward(INT_MAX, 64, 0xffffff) == 0xffffff);
    CHECK(pc::scaleCheatReward(-12, 1, 999999) == -12);

    // Actual settings serializer: missing keys, round trip, malformed values.
    pc::loadSettings();
    settings.cheats = {true, true, 32, 8};
    settings.volume = 30;
    settings.centredCamera = false;
    settings.keys[0] = 'v';
    pc::saveSettings();
    settings = pc::GameSettings{};
    pc::loadSettings();
    CHECK(settings.cheats.battleInvincible && settings.cheats.noRandomEncounters);
    CHECK(settings.cheats.expMultiplier == 32 && settings.cheats.goldMultiplier == 8);
    CHECK(settings.volume == 30 && settings.keys[0] == 'v');
    CHECK(!settings.centredCamera);
    {
        std::ofstream file(pc::testSaveDirectory / "settings.ini");
        file << "volume=70\ncheatExpMultiplier=-1\ncheatGoldMultiplier=2147483647\ncheatBattleInvincible=garbage\n";
    }
    settings = pc::GameSettings{};
    pc::loadSettings();
    CHECK(settings.cheats.expMultiplier == 1 && settings.cheats.goldMultiplier == 64);
    CHECK(!settings.cheats.battleInvincible && settings.volume == 70);
    CHECK(settings.centredCamera); // Older settings files keep upstream's new default.
    settings.cheats = pc::CheatSettings{};

    // Upstream 1.5's menu quit request survives alongside the cheat settings.
    CHECK(!pc::takeQuitRequest());
    pc::requestQuit();
    CHECK(pc::takeQuitRequest());
    CHECK(!pc::takeQuitRequest());
    CHECK(std::string(pc::quitLabel(0)) == "ゲームを おわる");
    CHECK(std::string(pc::quitLabel(1)) == "Quit game");
    for (int language = 2; language <= 5; ++language)
        CHECK(std::string(pc::quitLabel(language)) != "Quit game");

    auto& party = status::g_Party;
    CHECK(pc::lastCheatItemId() == 161);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 0, 1) == pc::cheatInvalidItem);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 162, 1) == pc::cheatInvalidItem);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 1, 100) == pc::cheatInvalidItem);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 1, 0) == pc::cheatInvalidItem);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 1, 90) == 90);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 1, 20) == 9);
    CHECK(party.haveItemSack_.items[0].count_ == 99);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 1, 1) == pc::cheatStackFull);
    // A hole before an existing stack must not create duplicate item IDs.
    party.haveItemSack_.items = {};
    party.haveItemSack_.items[8] = {5, 0, 10};
    CHECK(pc::runCheatAction(pc::cheatAddItem, 5, 3) == 3);
    CHECK(party.haveItemSack_.items[0].index_ == 0 && party.haveItemSack_.items[8].count_ == 13);
    for (auto& item : party.haveItemSack_.items) item = {1, 0, 10};
    CHECK(pc::runCheatAction(pc::cheatAddItem, 2, 1) == pc::cheatBagFull);
    CHECK(pc::runCheatAction(pc::cheatAddItem, 1, 1) == 1);
    party.haveItemSack_.items = {};

    for (const auto action : {pc::cheatMaxGold, pc::cheatMaxBank, pc::cheatMaxCasino, pc::cheatMaxMedals})
        CHECK(pc::runCheatAction(action) == pc::cheatDone);
    CHECK(party.gold_ == 999999 && party.bankMoney_ == 99999000);
    CHECK(party.casinoCoin_ == 999999 && party.medalCoin_ == 99);
    party.accessFlag_.flag_ = 0x800;
    std::fill(party.party_, party.party_ + 13, 0);
    party.party_[0] = 2;
    for (int i = 1; i <= 3; ++i) {
        auto& info = party.players[i].haveStatusInfo_;
        info.haveStatus_.hp = 0;
        info.haveStatus_.mp = 0;
        info.statusChange_.poisoned = true;
    }
    CHECK(pc::runCheatAction(pc::cheatRecoverParty) == pc::cheatDone);
    for (int i = 1; i <= 3; ++i) {
        const auto& player = party.players[i];
        CHECK(player.haveStatusInfo_.haveStatus_.hp == 100);
        CHECK(player.haveStatusInfo_.haveStatus_.mp == 50);
        CHECK(!player.haveStatusInfo_.statusChange_.poisoned && player.walkDamageClears == 1);
    }
    CHECK(party.accessFlag_.flag_ == 0x800 && party.party_[0] == 2 && party.party_[1] == 0);

    // Actual game methods, extracted unchanged from their translation units.
    auto& info = party.players[1].haveStatusInfo_;
    settings.cheats.battleInvincible = true;
    g_GlobalDQ4.part = BATTLE_PART;
    info.setHp(0); info.addBaseHp(-50); info.setMp(0); info.addMp(-20);
    CHECK(info.getHp() == 100 && info.haveStatus_.mp == 50);
    info.haveStatus_.isPlayer_ = false;
    info.setHp(70); info.addBaseHp(-10); info.setMp(40); info.addMp(-5);
    CHECK(info.getHp() == 60 && info.haveStatus_.mp == 35);
    info.haveStatus_.isPlayer_ = true;
    g_GlobalDQ4.part = 12;
    info.setHp(50); info.addMp(-5);
    CHECK(info.getHp() == 50 && info.haveStatus_.mp == 30);
    g_GlobalDQ4.part = BATTLE_PART;
    settings.cheats.battleInvincible = false;
    info.addBaseHp(-10); info.setMp(10);
    CHECK(info.getHp() == 40 && info.haveStatus_.mp == 10);
    info.noDamage_ = 1;
    info.setHp(0); info.addMp(-10);
    CHECK(info.getHp() == 40 && info.haveStatus_.mp == 10);
    info.noDamage_ = 0;

    encount::Encount encounter;
    encounter.brewCount_ = 2;
    CHECK(encounter.isEncounted() && encounter.isEncountedNext());
    settings.cheats.noRandomEncounters = true;
    CHECK(!encounter.isEncounted() && encounter.brewCount_ == 0);
    encounter.brewCount_ = 2;
    CHECK(!encounter.isEncountedNext() && encounter.brewCount_ == 0);
    encounter.forceEventBrew(25);
    CHECK(encounter.tileId_ == 25 && encounter.brews == 1);
    CHECK(g_Global.battles == 1 && g_Stage.mapUpdates == 1 && encounter.enable_ == 1);
    settings.cheats.noRandomEncounters = false;
    encounter.brewCount_ = 2;
    CHECK(encounter.isEncountedNext());

    party.party_[0] = 1; party.party_[1] = 2; party.party_[2] = 3;
    party.players[1].haveStatusInfo_.haveStatus_.hp = 100;
    party.players[2].haveStatusInfo_.haveStatus_.hp = 0;
    party.players[3].haveStatusInfo_.haveStatus_.isBattleNpc_ = true;
    settings.cheats.expMultiplier = 4;
    settings.cheats.goldMultiplier = 8;
    status::BaseAction::doubleFlag_ = 1;
    party.battleExp_ = 50; party.battleGold_ = 10; party.gold_ = 0;
    party.reflectBattleExp(); party.reflectBattleGold();
    CHECK(party.battleExp_ == 400 && party.battleGold_ == 160 && party.gold_ == 160);
    CHECK(party.players[1].haveStatusInfo_.haveStatus_.exp == 400);
    CHECK(party.players[2].haveStatusInfo_.haveStatus_.exp == 0);
    CHECK(party.players[3].haveStatusInfo_.haveStatus_.exp == 0);
    status::BaseAction::doubleFlag_ = 0;
    CHECK(pc::runCheatAction(pc::cheatReset) == pc::cheatDone);
    CHECK(!settings.cheats.battleInvincible && !settings.cheats.noRandomEncounters);
    CHECK(settings.cheats.expMultiplier == 1 && settings.cheats.goldMultiplier == 1);
    CHECK(party.gold_ == 160); // Resetting switches does not undo saved edits.

    SettingsPanel panel;
    menu::MenuItem item;
    panel.open(false);
    for (int i = 0; i < 4; ++i) input(panel, item, PAD_BUTTON_R);
    CHECK(panel.page_ == pc::pageGame);
    panel.draw(item, 0, 256, frame);
    CHECK(drawnTabs == 4);
    input(panel, item, PAD_BUTTON_R);
    CHECK(panel.page_ == pc::pageDisplay);
    panel.execute(item);
    item.active_ = rowIndex(panel, pc::settingCentredCamera);
    input(panel, item, PAD_KEY_LEFT);
    CHECK(!settings.centredCamera && pc::takeSettingsApply());
    input(panel, item, 0, 2);
    CHECK(settings.centredCamera && pc::takeSettingsApply());
    for (int language = 0; language <= 5; ++language) {
        settings.language = language;
        CHECK(std::string(pc::settingLabel(pc::settingCentredCamera, language)).size() > 0);
        CHECK(std::string(pc::settingHint(pc::settingCentredCamera, language)).size() > 0);
        const auto enabled = pc::settingValue(pc::settingCentredCamera, language);
        input(panel, item, PAD_KEY_RIGHT);
        CHECK(pc::settingValue(pc::settingCentredCamera, language) != enabled);
        input(panel, item, PAD_KEY_RIGHT);
        CHECK(settings.centredCamera && pc::settingValue(pc::settingCentredCamera, language) == enabled);
        CHECK(pc::takeSettingsApply());
        panel.draw(item, 0, 256, frame);
        CHECK(drawnTabs == 4 && highestRow <= 176);
    }
    panel.open(true);
    input(panel, item, PAD_BUTTON_L);
    CHECK(panel.page_ == pc::pageCheats && panel.rowCount_ == 14);
    panel.execute(item);
    CHECK(MenuTemplate_materiel::rows == 14);
    item.active_ = rowIndex(panel, pc::settingCheatInvincible);
    input(panel, item, 0, 2);
    CHECK(settings.cheats.battleInvincible && !pc::takeSettingsApply());
    item.active_ = rowIndex(panel, pc::settingCheatExp);
    input(panel, item, PAD_KEY_RIGHT);
    CHECK(settings.cheats.expMultiplier == 2);
    item.active_ = rowIndex(panel, SettingsPanel::ROW_CHEAT_GOLD);
    party.gold_ = 0;
    input(panel, item, PAD_KEY_RIGHT);
    CHECK(party.gold_ == 0); // Left/right never executes one-shot actions.
    input(panel, item, 0, 2);
    CHECK(party.gold_ == 999999);
    item.active_ = rowIndex(panel, SettingsPanel::ROW_CHEAT_ITEM);
    input(panel, item, PAD_KEY_LEFT);
    CHECK(panel.cheatItem_ == 161);
    input(panel, item, PAD_KEY_RIGHT);
    CHECK(panel.cheatItem_ == 1);
    item.active_ = rowIndex(panel, SettingsPanel::ROW_CHEAT_QUANTITY);
    for (int i = 0; i < 110; ++i) input(panel, item, PAD_KEY_RIGHT);
    CHECK(panel.cheatQuantity_ == 99);
    for (int i = 0; i < 110; ++i) input(panel, item, PAD_KEY_LEFT);
    CHECK(panel.cheatQuantity_ == 1);
    item.active_ = rowIndex(panel, SettingsPanel::ROW_CHEAT_ADD);
    input(panel, item, 0, 2);
    CHECK(panel.lastCheatResult_ == 1 && party.haveItemSack_.items[0].index_ == 1);
    item.active_ = rowIndex(panel, SettingsPanel::ROW_CHEAT_RESET);
    input(panel, item, 0, 2);
    CHECK(!settings.cheats.battleInvincible && settings.cheats.expMultiplier == 1);
    for (int language = 0; language <= 5; ++language) {
        settings.language = language;
        for (int active = 0; active < panel.rowCount_; ++active) {
            item.active_ = active;
            panel.draw(item, 0, 256, frame);
            CHECK(drawnTabs == 5 && highestRow <= 176);
        }
    }
    item.active_ = rowIndex(panel, SettingsPanel::ROW_BACK);
    CHECK(input(panel, item, 0, 2));
    CHECK(panel.page_ == pc::pageGame);
    input(panel, item, PAD_BUTTON_R);
    input(panel, item, PAD_BUTTON_R);
    CHECK(panel.page_ == pc::pageKeyboard);
    item.active_ = 0;
    input(panel, item, 0, 2);
    CHECK(pc::rebindWaiting() && pc::rebindPage() == pc::pageKeyboard);
    pc::cancelRebind();
    std::cout << checks << " checks pass (native code plus game/menu fixtures).\n";
}
