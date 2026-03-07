#ifndef SNAKE_AUDIO_H_
#define SNAKE_AUDIO_H_

#include <SDL.h>
#include <SDL_mixer.h>

#include <array>
#include <memory>
#include <vector>

#include "res.h"

constexpr int BGM_FADE_DURATION = 800;

struct MixMusicDeleter {
  void operator()(Mix_Music* music) const;
};

struct MixChunkDeleter {
  void operator()(Mix_Chunk* chunk) const;
};

class AudioSystem {
 public:
  static AudioSystem& instance();

  AudioSystem(const AudioSystem&) = delete;
  AudioSystem& operator=(const AudioSystem&) = delete;
  AudioSystem(AudioSystem&&) = delete;
  AudioSystem& operator=(AudioSystem&&) = delete;
  ~AudioSystem();

  void setBgm(int id, std::unique_ptr<Mix_Music, MixMusicDeleter> music);
  void addSound(std::unique_ptr<Mix_Chunk, MixChunkDeleter> sound);
  Mix_Music* bgm(int id) const;
  Mix_Chunk* sound(int id) const;
  int soundsCount() const;
  void clear();

  void playBgm(int id);
  void playAudio(int id);
  void randomBgm(int bgmCount);
  void stopBgm();
  void pauseSound();
  void resumeSound();

 private:
  AudioSystem() = default;

  std::array<std::unique_ptr<Mix_Music, MixMusicDeleter>, AUDIO_BGM_SIZE> bgms_{};
  std::vector<std::unique_ptr<Mix_Chunk, MixChunkDeleter>> sounds_{};
  int nowBgmId_ = -1;
};

void playBgm(int id);
void playAudio(int id);
void randomBgm();
void stopBgm();
void pauseSound();
void resumeSound();

#endif
