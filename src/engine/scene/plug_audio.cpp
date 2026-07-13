#include "engine/scene/plug_audio.h"
#include "engine/resources/plug_file_img.h"
CPlugAudio::CPlugAudio(void) = default;
CPlugAudio::~CPlugAudio(void) = default;

CPlugSound::CPlugSound(void) = default;
CPlugSound::~CPlugSound(void) = default;

CPlugSoundVideo::CPlugSoundVideo(void) = default;
CPlugSoundVideo::~CPlugSoundVideo(void) = default;

void CPlugSoundVideo::SetImage(CPlugFileImg *image) {
    if (CPlugFileVideo *video = dynamic_cast<CPlugFileVideo *>(image)) {
        videoFileImage.MwSetNod(video);
        stillImage.MwSetNod(nullptr);
        return;
    }
    videoFileImage.MwSetNod(nullptr);
    stillImage.MwSetNod(image);
}
