#ifndef SNAKE_GAME_H_
#define SNAKE_GAME_H_

#include "adt.h"
#include "player.h"
#include "sprite.h"
#include "types.h"

#include <memory>
#include <vector>

// Control
#define SPRITES_MAX_NUM 1024
#define KEYPRESS_DELTA 17
#define MAX_PALYERS_NUM 2

// Game
// Spike
#define SPIKE_TIME_MASK 600
#define SPIKE_OUT_INTERVAL 120
#define SPIKE_ANI_DURATION 20
#define SPRITE_EFFECT_DELTA 20
// Bounder Box
#define BIG_SPRITE_EFFECT_DELTA 25
#define SPRITE_EFFECT_VERTICAL_DELTA 6
#define SPRITE_EFFECT_FEET 12
#define GAME_BULLET_EFFECTIVE 16
// Team
#define GAME_MONSTERS_TEAM 9
// Buff
#define GAME_HP_MEDICINE_DELTA 55
#define GAME_HP_MEDICINE_EXTRA_DELTA 33
#define GAME_MAP_RELOAD_PERIOD 120
#define GAME_BUFF_ATTACK_K 2.5
#define GAME_BUFF_DEFENSE_K 2
#define GAME_FROZEN_DAMAGE_K 0.1
// Drop Rate
// Win

void pushMapToRender();
std::vector<std::shared_ptr<Score>> startGame(int localPlayers,
                                              int remotePlayers,
                                              bool localFirst);
void initGame(int localPlayers, int remotePlayers, bool localFirst);
void destroyGame(int);
void destroySnake(const std::shared_ptr<Snake>& snake);
int gameLoop();
void updateAnimationOfSprite(const std::shared_ptr<Sprite>& self);
void updateAnimationOfBlock(Block* self);
// this should only be used to create once or permant animation
std::shared_ptr<Animation> createAndPushAnimation(AnimationList& list,
                                                  Texture* texture,
                                                  const Effect* effect,
                                                  LoopType loopType,
                                                  int duration, int x, int y,
                                                  SDL_RendererFlip flip,
                                                  double angle, At at);
bool isPlayer(const std::shared_ptr<Snake>& snake);
bool crushVerdict(const std::shared_ptr<Sprite>& sprite, bool loose,
                  bool useAnimationBox);
void moveSprite(const std::shared_ptr<Sprite>& sprite, int moveDelta);
void moveSnake(const std::shared_ptr<Snake>& snake);
void shieldSprite(const std::shared_ptr<Sprite>& sprite, int duration);
void shieldSnake(const std::shared_ptr<Snake>& snake, int duration);
void appendSpriteToSnake(const std::shared_ptr<Snake>& snake, int spriteId,
                         int x, int y, Direction direction);
void setLevel(int level);

#endif
