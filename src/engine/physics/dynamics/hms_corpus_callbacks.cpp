#include "engine/physics/dynamics/hms_corpus.h"
#include "engine/physics/dynamics/hms_item.h"

int CHmsCorpus::IsExcludedFromInitialForcePass(void) const {
    return item->GetProperties().collisionGroup !=
           CHmsItem::ECollisionGroup_None;
}

void CHmsCorpus::NotifyOwnerAfterContacts(void) {
    if (item != nullptr) {
        item->RunAfterContactsCallback();
    }
}
