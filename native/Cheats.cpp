#include "Cheats.hpp"
#include "Settings.hpp"
#include "main/status/PartyStatus.hpp"
#include "main/status/PlayerStatus.hpp"
#include "main/param/Param.hpp"
#include "main/text/TextAPI.hpp"
#include <algorithm>

namespace pc {
int lastCheatItemId() {
    // Entry zero is empty; the compiled item table defines the valid range.
    return static_cast<int>(sizeof(itemDataTable.data) / sizeof(itemDataTable.data[0])) - 1;
}

void cheatItemName(char* text, int size, int itemId) {
    if (size <= 0) return;
    text[0] = '\0';
    if (itemId < 1 || itemId > lastCheatItemId()) return;
    TextAPI::extractText(text, size, 0x40000000, itemId);
    text[size - 1] = '\0';
}

namespace {
int addItem(int itemId, int quantity) {
    if (itemId < 1 || itemId > lastCheatItemId() || quantity < 1 || quantity > 99)
        return cheatInvalidItem;
    auto& sack = status::g_Party.haveItemSack_;
    int slot = sack.getItemSortIndex(itemId);
    if (slot < 0) {
        // The original add routine does not report a full bag reliably. Find
        // a slot first, and never index getItemCount(-1) for a new item.
        for (int i = 0; i < sack.getMaxCount(); ++i) {
            if (sack.getItem(i) == 0) {slot = i; break;}
        }
        if (slot < 0) return cheatBagFull;
    }
    auto* item = sack.getItemData(slot);
    const int current = item->index_ == itemId ? item->count_ : 0;
    if (current >= 99) return cheatStackFull;
    const int added = std::min(quantity, 99 - current);
    // Reuse an existing stack even when a hole precedes it; no duplicate IDs.
    item->index_ = static_cast<unsigned char>(itemId);
    item->equip_ = 0;
    item->count_ = static_cast<unsigned short>(current + added);
    return added;
}

void recoverParty() {
    auto& party = status::g_Party;
    const auto access = party.accessFlag_.flag_;
    int members[13];
    std::copy(party.party_, party.party_ + 13, members);
    // All recruited player characters, including an inaccessible carriage.
    // setAllPlayerMode does not clear unused slots in the original routine.
    std::fill(party.party_, party.party_ + 13, 0);
    party.setAllPlayerMode();
    for (int i = 0; i < party.getCount(); ++i) {
        auto* player = party.getPlayerStatus(i);
        player->haveStatusInfo_.revival();
        player->haveStatusInfo_.recovery();
        player->clearWalkDamage();
    }
    // The settings action must not change how its owning menu sees the party.
    party.accessFlag_.flag_ = access;
    std::copy(members, members + 13, party.party_);
}
}

int runCheatAction(CheatAction action, int itemId, int quantity) {
    auto& party = status::g_Party;
    switch (action) {
    case cheatMaxGold: party.setGold(999999); break;
    case cheatMaxBank: party.setBankMoney(99999000); break;
    case cheatMaxCasino: party.setCasinoCoin(999999); break;
    case cheatMaxMedals: party.setMedalCoin(99); break;
    case cheatRecoverParty: recoverParty(); break;
    case cheatAddItem: return addItem(itemId, quantity);
    case cheatReset: gameSettings().cheats = CheatSettings{}; break;
    default: return cheatInvalidItem;
    }
    return cheatDone;
}
}
