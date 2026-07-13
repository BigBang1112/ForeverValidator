#include "engine/scene/scene_object.h"
#include "engine/physics/dynamics/hms_poc.h"
#include "engine/scene/plug_audio.h"
CSceneObject::CSceneObject(void) = default;

CSceneObject::~CSceneObject(void) = default;

CMotion *CSceneObject::GetMotion(void) const {
    return motion.Get();
}

void CSceneObject::OnEnterScene(void) {
}

void CSceneObject::OnLeaveScene(void) {
}

void CSceneObject::RemoveFromScene(void) {
    if (scene == nullptr) {
        return;
    }
    OnLeaveScene();
    scene = nullptr;
}

void CMotion::Stop(void) {
}

void CMotion::Play(void) {
}

void CMotion::Pause(void) {
}

CSceneMessageHandler::CSceneMessageHandler(void) = default;

CSceneMessageHandler::~CSceneMessageHandler(void) = default;

CScenePoc::CScenePoc(void) = default;

CScenePoc::~CScenePoc(void) = default;

CSceneSoundSource::CSceneSoundSource(void)
        : ownedSound(std::make_unique<CHmsSoundSource>()),
          sound(ownedSound.get()) {}

CSceneSoundSource::~CSceneSoundSource(void) = default;

void CSceneSoundSource::Play(void) {
    if (sound != nullptr) {
        sound->playing = true;
    }
}

void CSceneSoundSource::Stop(void) {
    if (sound != nullptr) {
        sound->playing = false;
    }
}

void CSceneSoundSource::AttachSound(CHmsSoundSource &inlineSound) {
    ownedSound.reset();
    sound = &inlineSound;
}

void CSceneSoundSource::CreateSound(CPlugSound *newSound) {
    if (sound == nullptr) {
        ownedSound = std::make_unique<CHmsSoundSource>();
        sound = ownedSound.get();
    }
    if (newSound != sound->sound.Get()) {
        sound->sound.MwSetNod(newSound);
    }
}
