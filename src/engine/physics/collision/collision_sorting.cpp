#include <array>
#include <cstddef>
#include <utility>

#include "engine/physics/collision/hms_collision.h"
#include "engine/physics/collision/hms_collision_manager.h"

enum CollisionSortOrder {
    CollisionSortOrder_LeftAfterRight = 1,
    CollisionSortOrder_LeftBeforeRight = -1,
};

int SHmsPhysicalCollision::CompareForCollisionResponseSort(
        const SHmsPhysicalCollision &other) const {
    const float *leftValues[] = {
        &contactPoint.x,
        &contactPoint.y,
        &contactPoint.z,
        &impulseNormal.x,
        &impulseNormal.y,
        &impulseNormal.z,
        &separation.x,
        &separation.y,
        &separation.z,
    };
    const float *rightValues[] = {
        &other.contactPoint.x,
        &other.contactPoint.y,
        &other.contactPoint.z,
        &other.impulseNormal.x,
        &other.impulseNormal.y,
        &other.impulseNormal.z,
        &other.separation.x,
        &other.separation.y,
        &other.separation.z,
    };

    for (u32 index = 0u; index < 9u; index++) {
        const float left = *leftValues[index];
        const float right = *rightValues[index];
        if (!(right <= left)) {
            return CollisionSortOrder_LeftAfterRight;
        }
        if (right < left) {
            return CollisionSortOrder_LeftBeforeRight;
        }
    }

    if (!sphereMergePrimary && other.sphereMergePrimary) {
        return CollisionSortOrder_LeftBeforeRight;
    }
    return CollisionSortOrder_LeftAfterRight;
}

int sCompareCollision(const SHmsPhysicalCollision *a,
                      const SHmsPhysicalCollision *b) {
    return a->CompareForCollisionResponseSort(*b);
}

void CHmsCollisionBuffer::SortForCollisionResponse(void) {
    constexpr size_t Cutoff = 8u;
    constexpr size_t StackSize = 30u;

    const auto compareAt = [this](size_t left, size_t right) {
        return sCompareCollision(&collisions[left], &collisions[right]);
    };
    const auto swapAt = [this](size_t left, size_t right) {
        if (left != right) {
            std::swap(collisions[left], collisions[right]);
        }
    };
    const auto shortSort = [&compareAt, &swapAt](size_t low, size_t high) {
        while (high > low) {
            size_t selected = low;
            for (size_t cursor = low + 1u; cursor <= high; cursor++) {
                if (compareAt(cursor, selected) > 0) {
                    selected = cursor;
                }
            }
            swapAt(selected, high);
            high--;
        }
    };

    if (collisions.size() < 2u) {
        return;
    }

    std::array<size_t, StackSize> lowStack{};
    std::array<size_t, StackSize> highStack{};
    size_t stackDepth = 0u;
    size_t low = 0u;
    size_t high = collisions.size() - 1u;

    for (;;) {
        const size_t count = high - low + 1u;
        if (count <= Cutoff) {
            shortSort(low, high);
        } else {
            size_t middle = low + count / 2u;
            if (compareAt(low, middle) > 0) {
                swapAt(low, middle);
            }
            if (compareAt(low, high) > 0) {
                swapAt(low, high);
            }
            if (compareAt(middle, high) > 0) {
                swapAt(middle, high);
            }

            size_t lowCursor = low;
            size_t highCursor = high;
            for (;;) {
                if (middle > lowCursor) {
                    do {
                        lowCursor++;
                    } while (lowCursor < middle &&
                             compareAt(lowCursor, middle) <= 0);
                }
                if (middle <= lowCursor) {
                    do {
                        lowCursor++;
                    } while (lowCursor <= high &&
                             compareAt(lowCursor, middle) <= 0);
                }

                do {
                    highCursor--;
                } while (highCursor > middle &&
                         compareAt(highCursor, middle) > 0);

                if (highCursor < lowCursor) {
                    break;
                }

                swapAt(lowCursor, highCursor);
                if (middle == highCursor) {
                    middle = lowCursor;
                } else if (middle == lowCursor) {
                    middle = highCursor;
                }
            }

            highCursor++;
            if (middle < highCursor) {
                do {
                    highCursor--;
                } while (highCursor > middle &&
                         compareAt(highCursor, middle) == 0);
            }
            if (middle >= highCursor) {
                do {
                    highCursor--;
                } while (highCursor > low &&
                         compareAt(highCursor, middle) == 0);
            }

            const size_t leftSpan = highCursor - low;
            const size_t rightSpan = high - lowCursor;
            if (leftSpan >= rightSpan) {
                if (low < highCursor) {
                    lowStack[stackDepth] = low;
                    highStack[stackDepth] = highCursor;
                    stackDepth++;
                }
                if (lowCursor < high) {
                    low = lowCursor;
                    continue;
                }
            } else {
                if (lowCursor < high) {
                    lowStack[stackDepth] = lowCursor;
                    highStack[stackDepth] = high;
                    stackDepth++;
                }
                if (low < highCursor) {
                    high = highCursor;
                    continue;
                }
            }
        }

        if (stackDepth == 0u) {
            return;
        }
        stackDepth--;
        low = lowStack[stackDepth];
        high = highStack[stackDepth];
    }
}
