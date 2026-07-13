#include "engine/game/game_identifier.h"
SGameCtnIdentifier::SGameCtnIdentifier(void) = default;

SGameCtnIdentifier::SGameCtnIdentifier(
        const SGameCtnIdentifier &other) = default;

SGameCtnIdentifier::~SGameCtnIdentifier(void) = default;

int SGameCtnIdentifier::operator==(
        const SGameCtnIdentifier &other) const {
    return id == other.id &&
           collection == other.collection &&
           author == other.author;
}
