#include "audio.h"

#include "helper.h"
#include "res.h"

namespace {
int getBgmCount() {
  return bgmCount();
}
}

void MixMusicDeleter::operator()(Mix_Music* music) const {
  if (music) {
    Mix_FreeMusic(music);
  }
}

void MixChunkDeleter::operator()(Mix_Chunk* chunk) const {
  if (chunk) {
    Mix_FreeChunk(chunk);
  }
}

AudioSystem& AudioSystem::instance() {
  static AudioSystem instance;
  return instance;
}

AudioSystem::~AudioSystem() { clear(); }

void AudioSystem::setBgm(int id, std::unique_ptr<Mix_Music, MixMusicDeleter> music) {
  if (id < 0 || id >= static_cast<int>(bgms_.size())) {
    return;
  }
  bgms_[id] = std::move(music);
}

void AudioSystem::addSound(std::unique_ptr<Mix_Chunk, MixChunkDeleter> sound) {
  sounds_.push_back(std::move(sound));
}

Mix_Music* AudioSystem::bgm(int id) const {
  if (id < 0 || id >= static_cast<int>(bgms_.size())) {
    return nullptr;
  }
  return bgms_[id].get();
}

Mix_Chunk* AudioSystem::sound(int id) const {
  if (id < 0 || id >= static_cast<int>(sounds_.size())) {
    return nullptr;
  }
  return sounds_[id].get();
}

int AudioSystem::soundsCount() const {
  return static_cast<int>(sounds_.size());
}

void AudioSystem::clear() {
  for (auto& bgm : bgms_) {
    bgm.reset();
  }
  sounds_.clear();
  nowBgmId_ = -1;
}

void AudioSystem::playBgm(int id) {
  if (nowBgmId_ == id) {
    return;
  }
  Mix_Music* music = bgm(id);
  if (!music) {
    return;
  }
  if (nowBgmId_ == -1) {
    Mix_PlayMusic(music, -1);
  } else {
    Mix_FadeInMusic(music, -1, BGM_FADE_DURATION);
  }
  nowBgmId_ = id;
}

void AudioSystem::stopBgm() {
  Mix_FadeOutMusic(BGM_FADE_DURATION);
  nowBgmId_ = -1;
}

void AudioSystem::randomBgm(int bgmCount) {
  if (bgmCount <= 1) {
    return;
  }
  playBgm(randInt(1, bgmCount - 1));
}

void AudioSystem::playAudio(int id) {
  if (id >= 0 && id < soundsCount()) {
    Mix_PlayChannel(-1, sound(id), 0);
  }
}

void AudioSystem::pauseSound() {
  Mix_Pause(-1);
  Mix_PauseMusic();
}

void AudioSystem::resumeSound() {
  Mix_Resume(-1);
  Mix_ResumeMusic();
}

void playBgm(int id) { AudioSystem::instance().playBgm(id); }
void playAudio(int id) { AudioSystem::instance().playAudio(id); }
void randomBgm() { AudioSystem::instance().randomBgm(getBgmCount()); }
void stopBgm() { AudioSystem::instance().stopBgm(); }
void pauseSound() { AudioSystem::instance().pauseSound(); }
void resumeSound() { AudioSystem::instance().resumeSound(); }
