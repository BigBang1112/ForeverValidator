
#include <cmath>

#include "engine/rendering/plug_bitmap.h"
#include "engine/core/gm_func.h"
#include "engine/rendering/plug_shader.h"
namespace {

unsigned char UnitFloatToByte(float value) {
    if (std::isnan(value) || value <= 0.0f) {
        return 0u;
    }
    if (value >= 1.0f) {
        return 0xffu;
    }
    const float scaled = value * 255.0f;
    return static_cast<unsigned char>(static_cast<unsigned int>(scaled));
}

}  // namespace

CPlugBitmapShader::CPlugBitmapShader(void) = default;
CPlugBitmapShader::~CPlugBitmapShader(void) = default;

void CPlugBitmapShader::SetBitmapOwner(CPlugBitmap *bitmap) {
    bitmapOwner_ = bitmap;
}

void CPlugBitmapShader::SetShader(CPlugShader *shader) {
    shader_.MwSetNod(shader);
}

int CPlugBitmapShader::SetBitmapToSwap(CPlugBitmap *bitmap) {
    if (bitmap != nullptr && bitmapOwner_ != nullptr) {
        const CPlugFileImg *ownerImage = bitmapOwner_->Image();
        const CPlugFileImg *swapImage = bitmap->Image();
        if (ownerImage != nullptr && swapImage != nullptr &&
            (ownerImage->Width() != swapImage->Width() ||
             ownerImage->Height() != swapImage->Height())) {
            return 0;
        }
    }
    bitmapToSwap_.MwSetNod(bitmap);
    return 1;
}

CPlugBitmap::CPlugBitmap(void) = default;
CPlugBitmap::~CPlugBitmap(void) = default;

void CPlugBitmap::SetDirty(int dirty) {
    dirty_ = dirty != 0;
}

bool CPlugBitmap::IsDirty(void) const {
    return dirty_;
}

CPlugFileImg *CPlugBitmap::Image(void) const {
    return image_.Get();
}

CPlugBitmap::EUsage CPlugBitmap::Usage(void) const {
    return usage_;
}

EPlugVideoTimer CPlugBitmap::DefaultVideoTimer(void) const {
    return videoTimer_;
}

void CPlugBitmap::SetRenderTexelsMustPersist(int enabled) {
    texelsMustPersist_ = enabled != 0;
    SetDirty(1);
}

void CPlugBitmap::SetIsNonPow2Conditional(int enabled) {
    nonPowerOfTwoConditional_ = enabled != 0;
    SetDirty(1);
}

void CPlugBitmap::SetRenderRequireBlending(int enabled) {
    requireBlending_ = enabled != 0;
    SetDirty(1);
}

void CPlugBitmap::SetRenderMultiSampleCount(unsigned long sampleCount) {
    multisampleCount_ = sampleCount;
    SetDirty(1);
}

void CPlugBitmap::SetRenderCanUseDetachedKeeper(int enabled) {
    canUseDetachedKeeper_ = enabled != 0;
    SetDirty(1);
}

void CPlugBitmap::SetDefaultVideoTimer(EPlugVideoTimer timerMode) {
    if (videoTimer_ == timerMode) {
        return;
    }
    videoTimer_ = timerMode;
    SetDirty(1);
}

void CPlugBitmap::SetWantedColorDepth(EColorDepth colorDepth) {
    if (wantedColorDepth_ == colorDepth) {
        return;
    }
    wantedColorDepth_ = colorDepth;
    SetDirty(1);
}

void CPlugBitmap::DeletePixels(void) {
    if (image_ && image_->IsInSystemMemory() && deleteImagePixels_) {
        image_->DeletePixels();
    }
}

void CPlugBitmap::SetDefaultTexAddress(
        EGxTexAddress uAddress,
        EGxTexAddress vAddress,
        EGxTexAddress wAddress) {
    if (uAddress_ == uAddress && vAddress_ == vAddress &&
        wAddress_ == wAddress) {
        return;
    }
    uAddress_ = uAddress;
    vAddress_ = vAddress;
    wAddress_ = wAddress;
    if (generatedMipData_) {
        SetDirty(1);
    }
}

void CPlugBitmap::SetBorderAlpha(int enabled, float alpha) {
    borderAddsAlpha_ = enabled != 0;
    borderColor_.SetAlpha(UnitFloatToByte(alpha));
    SetDirty(1);
}

bool CPlugBitmap::SupportsUsage(EUsage usage) const {
    if (!image_) {
        return false;
    }
    const unsigned long bytesPerPixel = image_->BytesPerPixel();
    switch (usage) {
    case EUsage_Color:
        return bytesPerPixel >= 3ul ||
               (bytesPerPixel == 2ul && image_->IsTwoBytePackedColor());
    case EUsage_Opacity:
        return bytesPerPixel == 1ul || bytesPerPixel == 3ul;
    case EUsage_Luminance:
    case EUsage_NormalMap:
    case EUsage_NormalMapAlternate:
    case EUsage_Generated:
    case EUsage_Mask:
    case EUsage_MaskWithMip:
    case EUsage_MaskLinear:
    case EUsage_MaskSrgb:
    case EUsage_NormalMapSigned:
    case EUsage_NormalMapHeight:
    case EUsage_NormalMapAlpha:
        return bytesPerPixel == 1ul;
    case EUsage_RenderTarget:
    case EUsage_GeneratedAuxiliary:
        return image_->IsGeneratedImage() && bytesPerPixel != 0ul;
    case EUsage_Vector2:
    case EUsage_Vector2Alternate:
    case EUsage_Vector2Signed:
        return bytesPerPixel == 2ul;
    case EUsage_NormalMapRgb:
        return bytesPerPixel == 3ul || bytesPerPixel == 4ul;
    case EUsage_NormalMapPacked:
        return bytesPerPixel == 2ul || bytesPerPixel == 3ul;
    case EUsage_LuminanceAlpha:
    case EUsage_GeneratedAlpha:
        return bytesPerPixel == 1ul || bytesPerPixel == 2ul ||
               bytesPerPixel == 4ul;
    case EUsage_NormalMapBgra:
    case EUsage_NormalMapRgba:
        return bytesPerPixel == 4ul;
    case EUsage_AnyFormat:
        return true;
    case EUsage_NormalMapRgbOnly:
        return bytesPerPixel == 3ul;
    default:
        return false;
    }
}

int CPlugBitmap::CheckUsage(EUsage usage) {
    return SupportsUsage(usage) ? 1 : 0;
}

CPlugBitmap::EUsage CPlugBitmap::FallbackUsageForCurrentFormat(void) const {
    switch (usage_) {
    case EUsage_NormalMap:
        return EUsage_NormalMapRgb;
    case EUsage_NormalMapAlternate:
        return EUsage_NormalMapPacked;
    case EUsage_NormalMapRgb:
        return EUsage_NormalMap;
    case EUsage_NormalMapPacked:
        return EUsage_NormalMapAlternate;
    case EUsage_NormalMapSigned:
        return EUsage_NormalMapBgra;
    case EUsage_NormalMapBgra:
        return EUsage_NormalMapSigned;
    case EUsage_NormalMapAlpha:
    case EUsage_NormalMapRgbOnly:
        return EUsage_NormalMapRgba;
    case EUsage_NormalMapRgba:
        return image_ && image_->Format() != CPlugFileImg::EFormat_Alpha8
                ? EUsage_NormalMapRgbOnly
                : EUsage_NormalMapAlpha;
    default:
        return usage_;
    }
}

void CPlugBitmap::ForceImageBorders(void) {
    if (!image_ || image_->IsTwoBytePackedColor()) {
        return;
    }
    image_->ForceBorders_BGRA(
            borderColor_,
            borderUsesSourceColor_ ? 1 : 0,
            borderAddsAlpha_ ? 1 : 0,
            borderWidth_);
}

void CPlugBitmap::ApplyBorderPolicyIfReady(void) {
    if (!image_ || !(borderUsesSourceColor_ || borderAddsAlpha_)) {
        return;
    }
    if (!image_->IsInSystemMemory()) {
        image_->ReGenerateForceTexelLoading();
    }
    if (image_->IsInSystemMemory()) {
        ForceImageBorders();
    }
}

int CPlugBitmap::ReGenerate(void) {
    if (!image_) {
        return 0;
    }
    if (!image_->IsInSystemMemory() &&
        !image_->ReGenerateForceTexelLoading()) {
        return 0;
    }
    bool forcedBorders = false;
    if ((borderUsesSourceColor_ || borderAddsAlpha_)) {
        ForceImageBorders();
        forcedBorders = true;
    }
    if ((generatedMipData_ && image_ &&
           (!image_->HasGeneratedMipData() || (forcedBorders))) ||
        (false)) {
        return 0;
    }
    return 1;
}

CPlugBitmap::EPixelUpdate CPlugBitmap::PixelUpdateKind(void) const {
    return pixelUpdate_ ? pixelUpdate_->kind : PixelUpdate_None;
}

CPlugBitmapRender *CPlugBitmap::RenderPayload(void) const {
    if (!pixelUpdate_ || pixelUpdate_->kind != PixelUpdate_Render) {
        return nullptr;
    }
    const auto *render = std::get_if<CMwNodRef<CPlugBitmapRender>>(
            &pixelUpdate_->payload);
    return render != nullptr ? render->Get() : nullptr;
}

CPlugBitmap::SPixelDirty *CPlugBitmap::PixelDirtyPayload(void) {
    if (!pixelUpdate_ || pixelUpdate_->kind != PixelUpdate_Dirty) {
        return nullptr;
    }
    return std::get_if<SPixelDirty>(&pixelUpdate_->payload);
}

void CPlugBitmap::InstallVideoPixelUpdate(CPlugFileVideo &video) {
    SetPixelUpdate(PixelUpdate_Dirty, nullptr);
    if (SPixelDirty *dirty = PixelDirtyPayload()) {
        dirty->InstallVideoCallback(video);
    }
}

void CPlugBitmap::InstallPixelUpdatePayload(
        EPixelUpdate updateKind,
        CPlugBitmapRender *updateParam) {
    if (updateKind == PixelUpdate_None || updateKind == PixelUpdate_Unused) {
        pixelUpdate_.reset();
        return;
    }

    auto state = std::make_unique<PixelUpdateState>();
    state->kind = updateKind;
    switch (updateKind) {
    case PixelUpdate_Copy:
        state->payload.emplace<SPixelCopy>();
        break;
    case PixelUpdate_Blend:
        state->payload.emplace<SPixelBlend>();
        break;
    case PixelUpdate_Dirty:
        state->payload.emplace<SPixelDirty>();
        break;
    case PixelUpdate_Render: {
        CMwNodRef<CPlugBitmapRender> render;
        if (updateParam != nullptr) {
            render = updateParam;
        } else if (cubeRenderTarget_) {
            render = MakeMwNod<CPlugBitmapRenderCubeMap>();
        } else {
            render = MakeMwNod<CPlugBitmapRenderWater>();
        }
        state->payload.emplace<CMwNodRef<CPlugBitmapRender>>(
                std::move(render));
        break;
    }
    case PixelUpdate_Shader: {
        CMwNodRef<CPlugBitmapShader> shader = MakeMwNod<CPlugBitmapShader>();
        shader->SetBitmapOwner(this);
        state->payload.emplace<CMwNodRef<CPlugBitmapShader>>(
                std::move(shader));
        break;
    }
    case PixelUpdate_Specular: {
        deleteImagePixels_ = false;
        CDynaSpecular specular;
        if (image_) {
            specular.InitializeDefaultSpecular(*this);
        }
        state->payload.emplace<CDynaSpecular>(std::move(specular));
        break;
    }
    default:
        return;
    }
    pixelUpdate_ = std::move(state);
}

void CPlugBitmap::SetPixelUpdate(
        EPixelUpdate updateKind,
        CPlugBitmapRender *updateParam) {
    const EPixelUpdate oldKind = PixelUpdateKind();
    if (oldKind == updateKind &&
        (updateKind != PixelUpdate_Render || RenderPayload() == updateParam)) {
        return;
    }
    pixelUpdate_.reset();
    InstallPixelUpdatePayload(updateKind, updateParam);
    if (updateKind != PixelUpdate_None) {
        renderTexelReuse_ = false;
    }
    if (oldKind != updateKind) {
        SetDirty(1);
    }
}

int CPlugBitmap::SetUsage(EUsage usage) {
    if (usage_ == usage) {
        return 1;
    }
    if (!SupportsUsage(usage)) {
        return 0;
    }
    specialMaskMipPolicy_ = usage == EUsage_MaskWithMip;
    if (usage == EUsage_RenderTarget ||
        usage == EUsage_GeneratedAlpha ||
        usage == EUsage_GeneratedAuxiliary) {
        generatedMipData_ = false;
    }
    usage_ = usage;
    if (usage != EUsage_RenderTarget &&
        PixelUpdateKind() == PixelUpdate_Render) {
        SetPixelUpdate(PixelUpdate_None, nullptr);
    }
    SetDirty(1);
    return 1;
}

void CPlugBitmap::ResolveUnsupportedUsageFromImage(void) {
    if (SupportsUsage(usage_)) {
        return;
    }
    const EUsage fallback = FallbackUsageForCurrentFormat();
    if (fallback != usage_ && SetUsage(fallback)) {
        return;
    }
    if (image_ && image_->IsGeneratedImage() &&
        !image_->IsInSystemMemory()) {
        if (SetUsage(EUsage_RenderTarget) ||
            SetUsage(EUsage_Generated)) {
            return;
        }
    } else if (image_) {
        const unsigned long bytesPerPixel = image_->BytesPerPixel();
        if (bytesPerPixel == 1ul && SetUsage(EUsage_Opacity)) {
            return;
        }
        if (bytesPerPixel == 2ul &&
            SetUsage(image_->IsTwoBytePackedColor()
                     ? EUsage_Color
                     : EUsage_Vector2Alternate)) {
            return;
        }
    }
    (void)SetUsage(EUsage_Color);
}

void CPlugBitmap::UpdateNonPowerOfTwoStateFromImage(void) {
    if (!image_) {
        nonPowerOfTwoImage_ = false;
        return;
    }
    const bool dimensionsArePowerOfTwo =
            GmFunc::IsPowerOfTwo(image_->Width()) != 0ul &&
            GmFunc::IsPowerOfTwo(image_->Height()) != 0ul;
    nonPowerOfTwoImage_ =
            CPlugFileImg::s_NonPowOfTwoSupport <=
                    CPlugFileImg::ENonPowOfTwo_Conditional &&
            !dimensionsArePowerOfTwo;
}

void CPlugBitmap::UpdateFromImage(void) {
    if (!image_) {
        return;
    }
    if (image_->HasAutomaticMipSource() &&
        !automaticMipSuppressed_ && !automaticMipDisabled_) {
        generatedMipData_ = true;
    }
    cubeRenderTarget_ = image_->IsCubeMap();
    if (cubeRenderTarget_ && !(usage_ == EUsage_RenderTarget ||
           usage_ == EUsage_GeneratedAlpha ||
           usage_ == EUsage_GeneratedAuxiliary)) {
        generatedMipData_ = image_->HasGeneratedMipData();
    } else if (image_->HasEmbeddedMipSource() &&
               !image_->HasGeneratedMipData()) {
        generatedMipData_ = false;
    }
    ResolveUnsupportedUsageFromImage();
    UpdateNonPowerOfTwoStateFromImage();
    if (usage_ == EUsage_Generated || nonPowerOfTwoImage_) {
        generatedMipData_ = false;
    }
    SetDirty(1);
}

void CPlugBitmap::SetImage(CPlugFileImg *image) {
    CPlugFileImg *oldImage = image_.Get();
    if (oldImage == image) {
        return;
    }
    if (dynamic_cast<CPlugFileVideo *>(oldImage) != nullptr) {
        SetPixelUpdate(PixelUpdate_None, nullptr);
    }
    image_.MwSetNod(image);
    if (!image_) {
        return;
    }
    ApplyBorderPolicyIfReady();
    SetDirty(1);
    UpdateFromImage();
    if (CPlugFileVideo *video = dynamic_cast<CPlugFileVideo *>(image)) {
        video->AutoInitBitmap(this);
    }
}

CMwNodRef<CPlugFileImg> CPlugBitmap::CreateGeneratedCheckerFile(
        unsigned long checkerIndex) {
    CMwNodRef<CPlugFileImg> file(new CPlugFileGen());
    static_cast<CPlugFileGen *>(file.Get())->GenChecker(checkerIndex);
    return file;
}

void CPlugBitmap::GenerateChecker(unsigned long checkerIndex) {
    CMwNodRef<CPlugFileImg> file =
            CreateGeneratedCheckerFile(checkerIndex);
    SetImage(file.Get());
    (void)SetUsage(GeneratedCheckerUsage(*file));
}

CPlugBitmap::SPixelBlend::SPixelBlend(void) = default;
CPlugBitmap::SPixelCopy::SPixelCopy(void) = default;
CPlugBitmap::SPixelDirty::SPixelDirty(void) = default;
CPlugBitmap::SPixelDirty::~SPixelDirty(void) = default;

void CPlugBitmap::SPixelDirty::InstallVideoCallback(
        CPlugFileVideo &videoFile) {
    videoSource = &videoFile;
}

void CPlugBitmap::CDynaSpecular::SetRect(
        unsigned long index,
        unsigned long x0,
        unsigned long y0,
        unsigned long x1,
        unsigned long y1) {
    if (index >= subMaps.size()) {
        subMaps.resize(index + 1ul);
    }
    SSpecularSubMapCat &subMap = subMaps[index];
    subMap.rectX0 = x0;
    subMap.rectY0 = y0;
    subMap.rectX1 = x1;
    subMap.rectY1 = y1;
}

void CPlugBitmap::CDynaSpecular::ComputeFromRects(
        const CPlugBitmap *bitmap) {
    if (bitmap == nullptr || bitmap->Image() == nullptr) {
        return;
    }
    const float imageWidth = static_cast<float>(bitmap->Image()->Width());
    const float imageHeight = static_cast<float>(bitmap->Image()->Height());
    if (imageWidth <= 0.0f || imageHeight <= 0.0f) {
        return;
    }
    for (SSpecularSubMapCat &subMap : subMaps) {
        const unsigned long storedWidth = subMap.rectX1 > subMap.rectX0
                ? subMap.rectX1 - subMap.rectX0 - 1ul
                : 0ul;
        const unsigned long storedHeight = subMap.rectY1 > subMap.rectY0
                ? subMap.rectY1 - subMap.rectY0 - 1ul
                : 0ul;
        subMap.normalizedWidth =
                static_cast<float>(storedWidth) / imageWidth;
        subMap.normalizedHeight =
                static_cast<float>(storedHeight) / imageHeight;
        subMap.normalizedX0Center =
                (static_cast<float>(subMap.rectX0) + 0.5f) / imageWidth;
        subMap.normalizedY0Center =
                (static_cast<float>(subMap.rectY0) + 0.5f) / imageHeight;
    }
}

void CPlugBitmap::CDynaSpecular::InitializeDefaultSpecular(
        const CPlugBitmap &bitmap) {
    if (bitmap.Image() == nullptr) {
        return;
    }
    subMaps.resize(1u);
    SetRect(
            0ul,
            0ul,
            0ul,
            bitmap.Image()->Width(),
            bitmap.Image()->Height());
    ComputeFromRects(&bitmap);
}
