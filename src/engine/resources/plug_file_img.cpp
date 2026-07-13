#include "engine/resources/plug_file_img.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include "engine/rendering/plug_bitmap.h"
namespace {

struct Rgb8 {
    unsigned char first;
    unsigned char second;
    unsigned char third;
};

struct Rgba8 {
    unsigned char first;
    unsigned char second;
    unsigned char third;
    unsigned char alpha;
};

constexpr Rgb8 White3{0xffu, 0xffu, 0xffu};
constexpr Rgb8 Black3{0x00u, 0x00u, 0x00u};
constexpr std::array<Rgb8, 4> ChannelPalette{{
        White3,
        {0xffu, 0x00u, 0x00u},
        {0x00u, 0xffu, 0x00u},
        {0x00u, 0x00u, 0xffu},
}};

template <typename PixelGenerator>
std::vector<unsigned char> MakeRgbPixels(
        unsigned long width,
        unsigned long height,
        PixelGenerator pixelAt) {
    std::vector<unsigned char> pixels(width * height * 3ul);
    for (unsigned long y = 0ul; y < height; ++y) {
        for (unsigned long x = 0ul; x < width; ++x) {
            const Rgb8 pixel = pixelAt(x, y);
            const unsigned long offset = (y * width + x) * 3ul;
            pixels[offset] = pixel.first;
            pixels[offset + 1ul] = pixel.second;
            pixels[offset + 2ul] = pixel.third;
        }
    }
    return pixels;
}

template <typename PixelGenerator>
std::vector<unsigned char> MakeRgbaPixels(
        unsigned long width,
        unsigned long height,
        PixelGenerator pixelAt) {
    std::vector<unsigned char> pixels(width * height * 4ul);
    for (unsigned long y = 0ul; y < height; ++y) {
        for (unsigned long x = 0ul; x < width; ++x) {
            const Rgba8 pixel = pixelAt(x, y);
            const unsigned long offset = (y * width + x) * 4ul;
            pixels[offset] = pixel.first;
            pixels[offset + 1ul] = pixel.second;
            pixels[offset + 2ul] = pixel.third;
            pixels[offset + 3ul] = pixel.alpha;
        }
    }
    return pixels;
}

}  // namespace

CPlugFileImg::ENonPowOfTwo CPlugFileImg::s_NonPowOfTwoSupport =
        CPlugFileImg::ENonPowOfTwo_Unsupported;
unsigned long CPlugFileImg::s_EnableLoadTexels = 0ul;

CPlugFileImg::CPlugFileImg(void) = default;
CPlugFileImg::~CPlugFileImg(void) = default;

unsigned long CPlugFileImg::FormatByteCount(EFormat format) {
    switch (format) {
    case EFormat_Alpha8:
        return 1ul;
    case EFormat_PackedColor16:
        return 2ul;
    case EFormat_RGB8:
        return 3ul;
    case EFormat_BGRA8:
        return 4ul;
    }
    throw std::invalid_argument("unsupported CPlugFileImg pixel format");
}

int CPlugFileImg::ReGenerate(void) {
    return IsInSystemMemory();
}

unsigned long CPlugFileImg::ComputeNbMipMapLevel(void) const {
    if (mipMaps_.storedLevelCount.has_value()) {
        return *mipMaps_.storedLevelCount;
    }
    unsigned long largest = std::max(Width(), Height());
    unsigned long levels = 1ul;
    while (largest > 1ul) {
        largest >>= 1u;
        ++levels;
    }
    return levels;
}

int CPlugFileImg::ReGenerateForceTexelLoading(void) {
    struct LoadCounterGuard {
        LoadCounterGuard(void) { ++CPlugFileImg::s_EnableLoadTexels; }
        ~LoadCounterGuard(void) { --CPlugFileImg::s_EnableLoadTexels; }
    } guard;
    return ReGenerate();
}

int CPlugFileImg::IsInSystemMemory(void) const {
    return pixels_.Empty() ? 0 : 1;
}

unsigned char *CPlugFileImg::GetPixel(
        unsigned long x,
        unsigned long y) const {
    return pixels_.Pixel(x, y);
}

unsigned long CPlugFileImg::GetDepth(void) const {
    return Depth();
}

unsigned long CPlugFileImg::GetByteSizePerComp(void) const {
    return BytesPerPixel();
}

void CPlugFileImg::DeletePixels(void) {
    pixels_.Clear();
}

unsigned long CPlugFileImg::Width(void) const {
    return pixels_.Extent().width;
}

unsigned long CPlugFileImg::Height(void) const {
    return pixels_.Extent().height;
}

unsigned long CPlugFileImg::Depth(void) const {
    return pixels_.Extent().depth;
}

unsigned long CPlugFileImg::BytesPerPixel(void) const {
    return FormatByteCount(format_);
}

unsigned long CPlugFileImg::PixelCount(void) const {
    return static_cast<unsigned long>(pixels_.PixelCount());
}

unsigned long CPlugFileImg::RowByteCount(void) const {
    return static_cast<unsigned long>(pixels_.RowByteCount());
}

CPlugFileImg::EFormat CPlugFileImg::Format(void) const {
    return format_;
}

bool CPlugFileImg::IsGeneratedImage(void) const {
    return dynamic_cast<const CPlugFileGen *>(this) != nullptr;
}

bool CPlugFileImg::HasGeneratedMipData(void) const {
    return mipMaps_.generatedData;
}

bool CPlugFileImg::HasAutomaticMipSource(void) const {
    return mipMaps_.automaticSource;
}

bool CPlugFileImg::HasEmbeddedMipSource(void) const {
    return mipMaps_.embeddedSource;
}

bool CPlugFileImg::IsCubeMap(void) const {
    return cubeMap_;
}

bool CPlugFileImg::IsTwoBytePackedColor(void) const {
    return format_ == EFormat_PackedColor16;
}

const std::vector<unsigned char> &CPlugFileImg::PixelBytes(void) const {
    return pixels_.Bytes();
}

void CPlugFileImg::AssignPixels(
        unsigned long width,
        unsigned long height,
        unsigned long depth,
        EFormat format,
        std::vector<unsigned char> pixels) {
    pixels_.Assign(
            {width, height, depth},
            FormatByteCount(format),
            std::move(pixels));
    format_ = format;
    mipMaps_.storedLevelCount.reset();
    mipMaps_.generatedData = true;
    mipMaps_.automaticSource = false;
    mipMaps_.embeddedSource = false;
    cubeMap_ = false;
}

void *CPlugFileImg::AllocatePixels(void) {
    return pixels_.Allocate();
}

void CPlugFileImg::SetPixels(
        const SDesc &description,
        unsigned char *pixels,
        unsigned long byteCount) {
    PlugImagePixels source;
    source.SetDescription(
            {description.width, description.height, description.depth},
            FormatByteCount(description.format));
    if (byteCount != source.PixelCount() * source.BytesPerPixel() ||
        (byteCount != 0ul && pixels == nullptr)) {
        throw std::invalid_argument(
                "CPlugFileImg source pixel count does not match its description");
    }
    std::vector<unsigned char> ownedPixels(byteCount);
    if (byteCount != 0ul) {
        std::copy_n(pixels, byteCount, ownedPixels.begin());
    }
    AssignPixels(
            description.width,
            description.height,
            description.depth,
            description.format,
            std::move(ownedPixels));
}

void CPlugFileImg::FinishGeneratedPixels(void) {
    mipMaps_.storedLevelCount = 1ul;
    mipMaps_.generatedData = false;
    mipMaps_.automaticSource = false;
    mipMaps_.embeddedSource = false;
    cubeMap_ = false;
}

void CPlugFileImg::AddAlpha_BGRA(unsigned char alpha) {
    const unsigned long sourceChannelCount = BytesPerPixel();
    const std::vector<unsigned char> sourcePixels = PixelBytes();
    std::vector<unsigned char> bgra(PixelCount() * 4ul);
    for (unsigned long index = 0ul; index < PixelCount(); ++index) {
        const unsigned long copiedChannels =
                std::min(sourceChannelCount, 3ul);
        for (unsigned long channel = 0ul; channel < copiedChannels; ++channel) {
            bgra[index * 4ul + channel] =
                    sourcePixels[index * sourceChannelCount + channel];
        }
        for (unsigned long channel = copiedChannels; channel < 3ul; ++channel) {
            bgra[index * 4ul + channel] = 0u;
        }
        bgra[index * 4ul + 3ul] = alpha;
    }
    format_ = EFormat_BGRA8;
    pixels_.ResetFormat(pixels_.Extent(), 4ul, std::move(bgra));
    mipMaps_.storedLevelCount = 1ul;
    mipMaps_.generatedData = false;
}

void CPlugFileImg::ForceBorders_BGRA(
        GxBGRAColor &color,
        int hasSourceColor,
        int addAlpha,
        unsigned long borderWidth) {
    if (pixels_.Empty() || Width() == 0ul || Height() == 0ul) {
        return;
    }
    if (addAlpha != 0 && format_ != EFormat_BGRA8) {
        AddAlpha_BGRA(0xffu);
    }

    const unsigned long channelCount = BytesPerPixel();
    const unsigned long width =
            std::min(std::max(borderWidth, 1ul), Width());
    const unsigned long height =
            std::min(std::max(borderWidth, 1ul), Height());

    const auto paint = [&](unsigned long x, unsigned long y) {
        if (hasSourceColor != 0) {
            if (channelCount >= 1ul) pixels_.SetChannel(x, y, 0ul, color.Blue());
            if (channelCount >= 2ul) pixels_.SetChannel(x, y, 1ul, color.Green());
            if (channelCount >= 3ul) pixels_.SetChannel(x, y, 2ul, color.Red());
        }
        if (addAlpha != 0 && channelCount >= 4ul) {
            pixels_.SetChannel(x, y, 3ul, color.Alpha());
        }
    };

    for (unsigned long y = 0ul; y < Height(); ++y) {
        for (unsigned long x = 0ul; x < Width(); ++x) {
            if (y < height || y >= Height() - height ||
                x < width || x >= Width() - width) {
                paint(x, y);
            }
        }
    }
}

CPlugFileVideo::CPlugFileVideo(void) = default;
CPlugFileVideo::~CPlugFileVideo(void) = default;

int CPlugFileVideo::ReGenerate(void) {
    return CPlugFileImg::ReGenerate();
}

void CPlugFileVideo::Play(
        EPlugVideoTimer timerMode,
        int restart,
        unsigned long frameIndex) {
    (void)timerMode;
    (void)restart;
    (void)frameIndex;
}

void CPlugFileVideo::UpdateVideoTexture(
        CPlugBitmap *bitmap,
        SGxPixRect *dirtyRect,
        const SPlugPixelDirtyFormat &dirtyFormat,
        unsigned char *pixels,
        unsigned long byteCount) {
    (void)bitmap;
    (void)dirtyRect;
    (void)dirtyFormat;
    (void)pixels;
    (void)byteCount;
}

void CPlugFileVideo::AutoInitBitmap(CPlugBitmap *bitmap) {
    if (bitmap == nullptr) {
        return;
    }
    bitmap->InstallVideoPixelUpdate(*this);
    bitmap->SetWantedColorDepth(CPlugBitmap::EColorDepth_High);
    Play(bitmap->DefaultVideoTimer(), 1, 0ul);
    if (!IsInSystemMemory()) {
        (void)ReGenerate();
    }

    SPlugPixelDirtyFormat dirtyFormat;
    dirtyFormat.bytesPerPixel = BytesPerPixel();
    for (unsigned long channel = 0ul;
         channel < dirtyFormat.channelPresent.size(); ++channel) {
        dirtyFormat.channelPresent[channel] =
                channel < dirtyFormat.bytesPerPixel;
    }
    UpdateVideoTexture(
            bitmap,
            nullptr,
            dirtyFormat,
            PixelBytes().empty() ? nullptr : GetPixel(0ul, 0ul),
            RowByteCount());
}

CPlugFileGen::CPlugFileGen(void) = default;
CPlugFileGen::~CPlugFileGen(void) = default;

int CPlugFileGen::ReGenerate(void) {
    if (!checkerId_) {
        return 0;
    }
    GenChecker(*checkerId_);
    return IsInSystemMemory();
}

void CPlugFileGen::GenChecker(unsigned long checkerIndex) {
    checkerId_ = checkerIndex;
    switch (checkerIndex) {
    case 0ul:
        AssignPixels(
                2ul,
                2ul,
                1ul,
                EFormat_RGB8,
                MakeRgbPixels(2ul, 2ul, [](unsigned long x, unsigned long y) {
                    return x == y ? White3 : Black3;
                }));
        break;
    case 1ul:
        AssignPixels(
                4ul,
                4ul,
                1ul,
                EFormat_RGB8,
                MakeRgbPixels(4ul, 4ul, [](unsigned long x, unsigned long y) {
                    return ChannelPalette[(x + 4ul - y) & 3ul];
                }));
        break;
    case 2ul:
        AssignPixels(
                4ul,
                4ul,
                1ul,
                EFormat_BGRA8,
                MakeRgbaPixels(
                        4ul,
                        4ul,
                        [](unsigned long x, unsigned long y) {
                            const bool evenRow = (y & 1ul) == 0ul;
                            const bool evenColumn = (x & 1ul) == 0ul;
                            const Rgb8 rgb = evenColumn
                                    ? White3
                                    : ChannelPalette[1];
                            const unsigned char alpha = evenRow
                                    ? (evenColumn ? 0x00u : 0xffu)
                                    : (evenColumn ? 0x80u : 0x40u);
                            return Rgba8{
                                    rgb.first,
                                    rgb.second,
                                    rgb.third,
                                    alpha,
                            };
                        }));
        break;
    case 3ul: {
        std::vector<unsigned char> pixels(16ul * 16ul);
        for (unsigned long y = 0ul; y < 16ul; ++y) {
            for (unsigned long x = 0ul; x < 16ul; ++x) {
                pixels[y * 16ul + x] =
                        ((x < 8ul) == (y < 8ul)) ? 0xffu : 0x00u;
            }
        }
        AssignPixels(
                16ul,
                16ul,
                1ul,
                EFormat_Alpha8,
                std::move(pixels));
        break;
    }
    case 4ul:
        AssignPixels(
                4ul,
                4ul,
                1ul,
                EFormat_BGRA8,
                MakeRgbaPixels(
                        4ul,
                        4ul,
                        [](unsigned long x, unsigned long y) {
                            const bool white = ((x + y) & 1ul) == 0ul;
                            const Rgb8 rgb =
                                    white ? White3 : ChannelPalette[1];
                            return Rgba8{
                                    rgb.first,
                                    rgb.second,
                                    rgb.third,
                                    static_cast<unsigned char>(
                                            white ? 0x00u : 0xffu),
                            };
                        }));
        break;
    case 5ul: {
        std::vector<unsigned char> pixels(8ul * 8ul);
        for (unsigned long y = 0ul; y < 8ul; ++y) {
            for (unsigned long x = 0ul; x < 8ul; ++x) {
                pixels[y * 8ul + x] =
                        x < 8ul - y ? 0xffu : 0x00u;
            }
        }
        AssignPixels(
                8ul,
                8ul,
                1ul,
                EFormat_Alpha8,
                std::move(pixels));
        break;
    }
    case 6ul:
        AssignPixels(
                2ul,
                2ul,
                1ul,
                EFormat_RGB8,
                MakeRgbPixels(2ul, 2ul, [](unsigned long x, unsigned long y) {
                    return ChannelPalette[(y * 2ul + x + 1ul) & 3ul];
                }));
        break;
    case 7ul:
        AssignPixels(
                4ul,
                4ul,
                1ul,
                EFormat_BGRA8,
                MakeRgbaPixels(
                        4ul,
                        4ul,
                        [](unsigned long x, unsigned long y) {
                            const Rgb8 rgb =
                                    ChannelPalette[(x + 4ul - y) & 3ul];
                            return Rgba8{
                                    rgb.first,
                                    rgb.second,
                                    rgb.third,
                                    static_cast<unsigned char>(
                                            (x & 1ul) == 0ul
                                                    ? 0xffu
                                                    : 0x00u),
                            };
                        }));
        break;
    default:
        DeletePixels();
        return;
    }
    FinishGeneratedPixels();
}
