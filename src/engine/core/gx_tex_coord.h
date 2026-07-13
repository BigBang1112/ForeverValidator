#pragma once

#include <variant>
#include <vector>

struct GxTexCoord {
    float u = 0.0f;
    float v = 0.0f;
};

struct GxTexCoord3 {
    float u = 0.0f;
    float v = 0.0f;
    float w = 0.0f;
};

struct GxTexCoord4 {
    float u = 0.0f;
    float v = 0.0f;
    float w = 0.0f;
    float q = 0.0f;
};

enum class GxTexCoordDimension : unsigned long {
    Two = 0u,
    Three = 1u,
    Four = 2u,
};

class GxTexCoordSet {
public:
    using Values = std::variant<std::vector<GxTexCoord>,
                                std::vector<GxTexCoord3>,
                                std::vector<GxTexCoord4>>;

    GxTexCoordSet(void);
    static GxTexCoordSet Create(GxTexCoordDimension dimension,
                                unsigned long count);

    void Alloc(unsigned long count);
    GxTexCoordDimension Dimension(void) const;
    unsigned long Count(void) const;
    bool Empty(void) const;
    GxTexCoord *Data2(void);
    const GxTexCoord *Data2(void) const;
    void Reset(void);
    GxTexCoord4 Coordinate4At(unsigned long index) const;
    void SetCoordinate(unsigned long index, const GxTexCoord4 &coordinate);
    void Resize(unsigned long count);
    void InsertDefault(unsigned long index, unsigned long count);
    void Erase(unsigned long index, unsigned long count);
    void MoveRange(unsigned long destinationIndex,
                   unsigned long sourceIndex,
                   unsigned long count);
    void Apply2DScaleOffset(float uScale,
                            float vScale,
                            float uOffset,
                            float vOffset);

private:
    GxTexCoordDimension dimension_;
    Values values_;
};
