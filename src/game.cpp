#include "game.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <cmath>
#include <memory>
#include <optional>
#include <vector>
#include <array>
#include <iterator>
#include <thread>
#include <chrono>

#include "ai.h"
#include "audio.h"
#include "bullet.h"
#include "helper.h"
#include "map.h"
#include "net.h"
#include "render.h"
#include "res.h"
#include "sprite.h"
#include "text.h"
#include "types.h"
#include "ui.h"
#include "weapon.h"

#ifdef DBG
#include <cassert>
#endif
extern const int SCALE_FACTOR;
extern const int n, m;

extern Texture textures[TEXTURES_SIZE];
const int MOVE_STEP = 2;

namespace {
Text* createGameText(const std::string& text, SDL_Color color) {
  SDL_Renderer* sdlRenderer = renderer();
  TTF_Font* ttfFont = font();
  if (!sdlRenderer || !ttfFont) {
    return nullptr;
  }
  return new Text(text, color, sdlRenderer, ttfFont);
}

void destroyGameText(Text* text) {
  delete text;
}
}  // namespace

extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;
extern Effect effects[];

extern Weapon weapons[WEAPONS_SIZE];
extern Sprite commonSprites[COMMON_SPRITE_SIZE];

// Map
std::array<std::array<Block, MAP_SIZE>, MAP_SIZE> map;
std::array<std::array<Item, MAP_SIZE>, MAP_SIZE> itemMap;
extern bool hasMap[MAP_SIZE][MAP_SIZE];
std::array<std::array<bool, MAP_SIZE>, MAP_SIZE> hasEnemy{};
int spikeDamage = 1;
// Sprite
std::array<std::shared_ptr<Snake>, SPRITES_MAX_NUM> spriteSnake{};
BulletList bullets;

int gameLevel, stage;
int spritesCount, playersCount, flasksCount, herosCount, flasksSetting,
    herosSetting, spritesSetting, bossSetting;
// Win
int GAME_WIN_NUM;
int termCount;
bool willTerm;
int status;
// Drop rate
double GAME_LUCKY;
double GAME_DROPOUT_YELLOW_FLASKS;
double GAME_DROPOUT_WEAPONS;
double GAME_TRAP_RATE;
extern double AI_LOCK_LIMIT;
double GAME_MONSTERS_HP_ADJUST;
double GAME_MONSTERS_WEAPON_BUFF_ADJUST;
double GAME_MONSTERS_GEN_FACTOR;
void setLevel(int level) {
  gameLevel = level;
  spritesSetting = 25;
  bossSetting = 2;
  herosSetting = 8;
  flasksSetting = 6;
  GAME_LUCKY = 1;
  GAME_DROPOUT_YELLOW_FLASKS = 0.3;
  GAME_DROPOUT_WEAPONS = 0.7;
  GAME_TRAP_RATE = 0.005 * (level + 1);
  GAME_MONSTERS_HP_ADJUST = 1 + level * 0.8 + stage * level * 0.1;
  GAME_MONSTERS_GEN_FACTOR = 1 + level * 0.5 + stage * level * 0.05;
  GAME_MONSTERS_WEAPON_BUFF_ADJUST = 1 + level * 0.8 + stage * level * 0.02;
  AI_LOCK_LIMIT = MAX(1, 7 - level * 2 - stage / 2);
  GAME_WIN_NUM = 10 + level * 5 + stage * 3;
  if (level == 0) {
  } else if (level == 1) {
    GAME_DROPOUT_WEAPONS = 0.98;
    herosSetting = 5;
    flasksSetting = 4;
    spritesSetting = 28;
    bossSetting = 3;
  } else if (level == 2) {
    GAME_DROPOUT_WEAPONS = 0.98;
    GAME_DROPOUT_YELLOW_FLASKS = 0.3;
    spritesSetting = 28;
    herosSetting = 5;
    flasksSetting = 3;
    bossSetting = 5;
  }
  spritesSetting += stage / 2 * (level + 1);
  bossSetting += stage / 3;
}

std::vector<std::shared_ptr<Score>> startGame(int localPlayers,
                                              int remotePlayers,
                                              bool localFirst) {
  std::vector<std::shared_ptr<Score>> scores;
  scores.reserve(localPlayers);
  for (int i = 0; i < localPlayers; i++) {
    scores.push_back(std::make_shared<Score>());
  }
  int status = 0;
  stage = 0;
  do {
    initGame(localPlayers, remotePlayers, localFirst);
    setLevel(gameLevel);
    status = gameLoop();
    for (int i = 0; i < localPlayers; i++) {
      if (spriteSnake[i]) {
        scores[i]->addScore(*spriteSnake[i]->score());
      }
    }
    destroyGame(status);
    stage++;
  } while (status == 0);
  return scores;
}

void appendSpriteToSnake(const std::shared_ptr<Snake>& snake, int spriteId,
                         int x, int y, Direction direction) {
  if (!snake) {
    return;
  }
  snake->incrementNum();
  snake->score()->addGot(1);

  std::shared_ptr<Sprite> snakeHead = nullptr;
  if (!snake->sprites().empty()) {
    snakeHead = snake->sprites().front();
    x = snakeHead->x();
    y = snakeHead->y();
    const int delta =
        (snakeHead->animation()->origin()->width() * SCALE_FACTOR +
         commonSprites[spriteId].animation()->origin()->width() * SCALE_FACTOR) /
        2;
    if (snakeHead->direction() == Direction::Left) {
      x -= delta;
    } else if (snakeHead->direction() == Direction::Right) {
      x += delta;
    } else if (snakeHead->direction() == Direction::Up) {
      y -= delta;
    } else {
      y += delta;
    }
  }
  auto sprite = std::make_shared<Sprite>(commonSprites[spriteId], x, y);
  sprite->setDirection(direction);
  if (direction == Direction::Left) {
    sprite->setFace(Direction::Left);
  }
  if (snakeHead) {
    sprite->setDirection(snakeHead->direction());
    sprite->setFace(snakeHead->face());
    if (snakeHead->animation()) {
      sprite->animation()->setCurrentFrame(
          snakeHead->animation()->currentFrame());
    }
  }
  snake->sprites().push_front(sprite);

  if (sprite->animation()) {
    pushAnimationToRender(RENDER_LIST_SPRITE_ID, sprite->animation());
  }

  if (snake->buffs()[BUFF_DEFFENCE]) {
    shieldSprite(sprite, snake->buffs()[BUFF_DEFFENCE]);
  }
}

void initPlayer(int playerType) {
  spritesCount++;
  const auto snake = std::make_shared<Snake>(
      MOVE_STEP, playersCount, static_cast<PlayerType>(playerType));
  spriteSnake[playersCount] = snake;
  appendSpriteToSnake(snake, SPRITE_KNIGHT, SCREEN_WIDTH / 2,
                      SCREEN_HEIGHT / 2 + playersCount * 2 * UNIT,
                      Direction::Right);
  playersCount++;
}
void generateHeroItem(int x, int y) {
  const int heroId = randInt(SPRITE_KNIGHT, SPRITE_LIZARD);
  auto modelAnimation = commonSprites[heroId].animation();
  if (!modelAnimation) {
    return;
  }
  auto animation = std::make_shared<Animation>(*modelAnimation);
  animation->setAt(At::BottomCenter);
  x *= UNIT;
  y *= UNIT;
  animation->setPosition(x + UNIT / 2, y + UNIT - 3);
  itemMap[x / UNIT][y / UNIT] =
      Item(ItemType::Hero, heroId, 0, animation);
  pushAnimationToRender(RENDER_LIST_SPRITE_ID, animation);
}
void generateItem(int x, int y, ItemType type) {
  int textureId = RES_FLASK_BIG_RED, id = 0, belong = SPRITE_KNIGHT;
  if (type == ItemType::HpMedicine) {
    textureId = RES_FLASK_BIG_RED;
  } else if (type == ItemType::HpExtraMedicine) {
    textureId = RES_FLASK_BIG_YELLOW;
  } else if (type == ItemType::Weapon) {
    int kind = randInt(0, 5);
    if (kind == 0) {
      textureId = RES_ICE_SWORD;
      id = WEAPON_ICE_SWORD;
      belong = SPRITE_KNIGHT;
    } else if (kind == 1) {
      textureId = RES_HOLY_SWORD;
      id = WEAPON_HOLY_SWORD;
      belong = SPRITE_KNIGHT;
    } else if (kind == 2) {
      textureId = RES_THUNDER_STAFF;
      id = WEAPON_THUNDER_STAFF;
      belong = SPRITE_WIZZARD;
    } else if (kind == 3) {
      textureId = RES_PURPLE_STAFF;
      id = WEAPON_PURPLE_STAFF;
      belong = SPRITE_WIZZARD;
    } else if (kind == 4) {
      textureId = RES_GRASS_SWORD;
      id = WEAPON_SOLID_CLAW;
      belong = SPRITE_LIZARD;
    } else if (kind == 5) {
      textureId = RES_POWERFUL_BOW;
      id = WEAPON_POWERFUL_BOW;
      belong = SPRITE_ELF;
    }
  }
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_MAP_ITEMS_ID], &textures[textureId], nullptr,
      LoopType::Infinite, 3, x * UNIT, y * UNIT, SDL_FLIP_NONE, 0,
      At::BottomLeft);
  itemMap[x][y] = Item(type, id, belong, animation);
}
void takeHpMedcine(Snake* snake, int delta, bool extra) {
  if (!snake) {
    return;
  }
  for (const auto& sprite : snake->sprites()) {
    if (!sprite) {
      continue;
    }
    if (sprite->hp() == sprite->totalHp() && !extra) {
      continue;
    }
    int addHp = static_cast<int>(delta * sprite->totalHp() / 100.0);
    if (!extra) {
      addHp = MAX(0, MIN(sprite->totalHp() - sprite->hp(), addHp));
    }
    sprite->setHp(sprite->hp() + addHp);
    auto animation = createAndPushAnimation(
        animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_HP_MED], nullptr,
        LoopType::Once, SPRITE_ANIMATION_DURATION, sprite->x(), sprite->y(),
        SDL_FLIP_NONE, 0, At::BottomCenter);
    bindAnimationToSprite(animation, sprite, false);
  }
}
bool takeWeapon(Snake* snake, Item* weaponItem) {
  if (!snake || !weaponItem) {
    return false;
  }
  Weapon* weapon = &weapons[weaponItem->id()];
  bool taken = false;
  for (const auto& sprite : snake->sprites()) {
    if (!sprite) {
      continue;
    }
    const auto animation = sprite->animation();
    const auto modelAnimation = commonSprites[weaponItem->belong()].animation();
    if (animation && modelAnimation &&
        animation->origin() == modelAnimation->origin() &&
        sprite->weapon() == commonSprites[weaponItem->belong()].weapon()) {
      sprite->setWeapon(weapon);
      const auto itemAnimation = weaponItem->animation();
      if (itemAnimation && itemAnimation->origin()) {
        auto effect = createAndPushAnimation(
            animationsList[RENDER_LIST_EFFECT_ID], itemAnimation->origin(),
            nullptr, LoopType::Infinite, 3, sprite->x(), sprite->y(),
            SDL_FLIP_NONE, 0, At::BottomCenter);
        bindAnimationToSprite(effect, sprite, true);
      }
      sprite->setHp(sprite->hp() +
                    static_cast<int>(GAME_HP_MEDICINE_EXTRA_DELTA / 100.0 *
                                     sprite->totalHp() * 5));
      auto effect = createAndPushAnimation(
          animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_HP_MED], nullptr,
          LoopType::Once, SPRITE_ANIMATION_DURATION, 0, 0, SDL_FLIP_NONE, 0,
          At::BottomCenter);
      bindAnimationToSprite(effect, sprite, true);
      taken = true;
      break;
    }
  }
  return taken;
}
void dropItemNearSprite(const Sprite* sprite, ItemType itemType) {
  if (!sprite) {
    return;
  }
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      const int x = sprite->x() / UNIT + dx;
      const int y = sprite->y() / UNIT + dy;
      if (inr(x, 0, n - 1) && inr(y, 0, m - 1) && hasMap[x][y] &&
          itemMap[x][y].type() == ItemType::None) {
        generateItem(x, y, itemType);
      }
      return;
    }
  }
}
void generateHeroItemAllMap() {
  int x = 0;
  int y = 0;
  do {
    x = randInt(1, n - 2);
    y = randInt(1, m - 2);
  } while (!hasMap[x][y] || map[x][y].type() != BlockType::Floor ||
           itemMap[x][y].type() != ItemType::None ||
           !hasMap[x - 1][y] + !hasMap[x + 1][y] + !hasMap[x][y + 1] +
                   !hasMap[x][y - 1] >=
               1);
  generateHeroItem(x, y);
}
void clearItemMap() {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      itemMap[i][j].setType(ItemType::None);
    }
  }
}
void initItemMap(int hCount, int fCount) {
  int x = 0;
  int y = 0;
  while (hCount--) {
    generateHeroItemAllMap();
    herosCount++;
  }
  while (fCount--) {
    do {
      x = randInt(0, n - 1);
      y = randInt(0, m - 1);
    } while (!hasMap[x][y] || map[x][y].type() != BlockType::Floor ||
             itemMap[x][y].type() != ItemType::None);
    generateItem(x, y, ItemType::HpMedicine);
    flasksCount++;
  }
}
int generateEnemy(int x, int y, int minLen, int maxLen, int minId, int maxId,
                  int step) {
  const auto snake = std::make_shared<Snake>(step, GAME_MONSTERS_TEAM,
                                             PlayerType::Computer);
  spriteSnake[spritesCount++] = snake;
  hasEnemy[x][y] = 1;
  const bool vertical = randInt(0, 1) != 0;
  int len = 1;
  if (vertical) {
    while (inr(y + len, 0, m - 1) && hasMap[x][y + len] &&
           map[x][y + len].type() == BlockType::Floor &&
           itemMap[x][y + len].type() == ItemType::None && !hasEnemy[x][y + len]) {
      len++;
    }
  } else {
    while (inr(x + len, 0, n - 1) && hasMap[x + len][y] &&
           map[x + len][y].type() == BlockType::Floor &&
           itemMap[x + len][y].type() == ItemType::None && !hasEnemy[x + len][y]) {
      len++;
    }
  }
  minLen = MIN(minLen, len);
  maxLen = MIN(maxLen, len);
  len = randInt(minLen, maxLen);
  for (int i = 0; i < len; i++) {
    int xx = x;
    int yy = y;
    if (vertical) {
      yy += i;
    } else {
      xx += i;
    }
    hasEnemy[xx][yy] = 1;
    xx *= UNIT;
    yy *= UNIT;
    yy += UNIT;
    xx += UNIT / 2;
    const int spriteId = randInt(minId, maxId);
    appendSpriteToSnake(snake, spriteId, xx, yy,
                        vertical ? Direction::Down : Direction::Right);
  }
  return len;
}
Point getAvaliablePos() {
  int x = 0;
  int y = 0;
  do {
    x = randInt(0, n - 1);
    y = randInt(0, m - 1);
  } while (!hasMap[x][y] || map[x][y].type() != BlockType::Floor ||
           itemMap[x][y].type() != ItemType::None || hasEnemy[x][y] ||
           !hasMap[x - 1][y] + !hasMap[x + 1][y] + !hasMap[x][y + 1] +
                   !hasMap[x][y - 1] >=
               1);
  return {x, y};
}
void initEnemies(int enemiesCount) {
  for (auto& row : hasEnemy) {
    row.fill(false);
  }
  for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
      hasEnemy[n / 2 + i][m / 2 + j] = 1;
    }
  }
  for (int i = 0; i < enemiesCount;) {
    const double random = randDouble() * GAME_MONSTERS_GEN_FACTOR;
    const Point pos = getAvaliablePos();
    const int x = pos.x;
    const int y = pos.y;
    int minLen = 2;
    int maxLen = 4;
    int step = 1;
    int startId = SPRITE_TINY_ZOMBIE;
    int endId = SPRITE_TINY_ZOMBIE;
    if (random < 0.3) {
      startId = SPRITE_TINY_ZOMBIE;
      endId = SPRITE_SKELET;
    } else if (random < 0.4) {
      startId = SPRITE_WOGOL;
      endId = SPRITE_CHROT;
      step = 2;
    } else if (random < 0.5) {
      startId = SPRITE_ZOMBIE;
      endId = SPRITE_ICE_ZOMBIE;
    } else if (random < 0.8) {
      startId = SPRITE_MUDDY;
      endId = SPRITE_SWAMPY;
    } else {
      startId = SPRITE_MASKED_ORC;
      endId = SPRITE_NECROMANCER;
    }
    i += generateEnemy(x, y, minLen, maxLen, startId, endId, step);
  }
  for (int bossCount = 0; bossCount < bossSetting; bossCount++) {
    const Point pos = getAvaliablePos();
    generateEnemy(pos.x, pos.y, 1, 1, SPRITE_BIG_ZOMBIE, SPRITE_BIG_DEMON, 1);
  }
}

/*
 * Put buff animation on snake
 */

void freezeSnake(Snake* snake, int duration) {
  if (!snake) {
    return;
  }
  auto& buffs = snake->buffs();
  if (buffs[BUFF_FROZEN]) {
    return;
  }
  if (!buffs[BUFF_DEFFENCE]) {
    buffs[BUFF_FROZEN] += duration;
  }
  std::shared_ptr<Effect> effect;
  if (buffs[BUFF_DEFFENCE]) {
    effect = std::make_shared<Effect>(effects[EFFECT_VANISH30]);
    duration = 30;
  }
  for (const auto& sprite : snake->sprites()) {
    if (!sprite) {
      continue;
    }
    const Effect* effectPtr = effect ? effect.get() : nullptr;
    auto animation = createAndPushAnimation(
        animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_ICE], effectPtr,
        LoopType::Once, duration, sprite->x(), sprite->y(), SDL_FLIP_NONE, 0,
        At::BottomCenter);
    animation->setScaled(false);
    if (buffs[BUFF_DEFFENCE]) {
      continue;
    }
    bindAnimationToSprite(animation, sprite, true);
  }
}

void slowDownSnake(Snake* snake, int duration) {
  if (!snake) {
    return;
  }
  auto& buffs = snake->buffs();
  if (buffs[BUFF_SLOWDOWN]) {
    return;
  }
  if (!buffs[BUFF_DEFFENCE]) {
    buffs[BUFF_SLOWDOWN] += duration;
  }
  std::shared_ptr<Effect> effect;
  if (buffs[BUFF_DEFFENCE]) {
    effect = std::make_shared<Effect>(effects[EFFECT_VANISH30]);
    duration = 30;
  }
  for (const auto& sprite : snake->sprites()) {
    if (!sprite) {
      continue;
    }
    const Effect* effectPtr = effect ? effect.get() : nullptr;
    auto animation = createAndPushAnimation(
        animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_SOLIDFX], effectPtr,
        LoopType::Lifespan, 40, sprite->x(), sprite->y(), SDL_FLIP_NONE, 0,
        At::BottomCenter);
    animation->setLifeSpan(duration);
    animation->setScaled(false);
    if (buffs[BUFF_DEFFENCE]) {
      continue;
    }
    bindAnimationToSprite(animation, sprite, true);
  }
}

void shieldSprite(const std::shared_ptr<Sprite>& sprite, int duration) {
  if (!sprite) {
    return;
  }
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_HOLY_SHIELD], nullptr,
      LoopType::Lifespan, 40, sprite->x(), sprite->y(), SDL_FLIP_NONE, 0,
      At::BottomCenter);
  bindAnimationToSprite(animation, sprite, true);
  animation->setLifeSpan(duration);
}

void shieldSnake(const std::shared_ptr<Snake>& snake, int duration) {
  if (!snake) {
    return;
  }
  auto& buffs = snake->buffs();
  if (buffs[BUFF_DEFFENCE]) {
    return;
  }
  buffs[BUFF_DEFFENCE] += duration;
  for (const auto& sprite : snake->sprites()) {
    shieldSprite(sprite, duration);
  }
}

void attackUpSprite(const std::shared_ptr<Sprite>& sprite, int duration) {
  if (!sprite) {
    return;
  }
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_ATTACK_UP], nullptr,
      LoopType::Lifespan, SPRITE_ANIMATION_DURATION, sprite->x(), sprite->y(),
      SDL_FLIP_NONE, 0, At::BottomCenter);
  bindAnimationToSprite(animation, sprite, true);
  animation->setLifeSpan(duration);
}

void attackUpSnkae(const std::shared_ptr<Snake>& snake, int duration) {
  if (!snake) {
    return;
  }
  auto& buffs = snake->buffs();
  if (buffs[BUFF_ATTACK]) {
    return;
  }
  buffs[BUFF_ATTACK] += duration;
  for (const auto& sprite : snake->sprites()) {
    attackUpSprite(sprite, duration);
  }
}

/*
  Initialize and deinitialize game and snake
*/

void initGame(int localPlayers, int remotePlayers, bool localFirst) {
  randomBgm();
  status = 0;
  termCount = willTerm = 0;
  spritesCount = playersCount = flasksCount = herosCount = 0;
  initRenderer();
  initCountDownBar();

  // create default hero at (w/2, h/2) (as well push ani)
  for (int i = 0; i < localPlayers + remotePlayers; i++) {
    PlayerType playerType = PlayerType::Local;
    if (localFirst) {
      playerType = i < localPlayers ? PlayerType::Local : PlayerType::Remote;
    } else {
      playerType = i < remotePlayers ? PlayerType::Remote : PlayerType::Local;
    }
    initPlayer(static_cast<int>(playerType));
    shieldSnake(spriteSnake[i], 300);
  }
  initInfo();
  // create map
  initRandomMap(0.7, 7, GAME_TRAP_RATE);

  clearItemMap();
  // create enemies
  initEnemies(spritesSetting);
  pushMapToRender();
  bullets.clear();
}

void destroyGame(int status) {
  while (spritesCount) {
    destroySnake(spriteSnake[--spritesCount]);
    spriteSnake[spritesCount] = nullptr;
  }
  for (int i = 0; i < ANIMATION_LINK_LIST_NUM; i++) {
    animationsList[i].clear();
  }
  bullets.clear();

  blackout();
  const char* msg = status == 0 ? "Stage Clear" : "Game Over";
  extern SDL_Color WHITE;
  if (Text* rawText = createGameText(msg, WHITE)) {
    std::shared_ptr<Text> text(rawText, destroyGameText);
    renderCenteredText(text.get(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2);
  }
  if (SDL_Renderer* sdlRenderer = renderer()) {
    SDL_RenderPresent(sdlRenderer);
  }
  std::this_thread::sleep_for(std::chrono::seconds(RENDER_GAMEOVER_DURATION));
  clearRenderer();
}

void destroySnake(const std::shared_ptr<Snake>& snake) {
  if (!snake) {
    return;
  }
  for (auto it = bullets.begin(); it != bullets.end();) {
    if (!(*it) || (*it)->owner().get() == snake.get()) {
      it = bullets.erase(it);
    } else {
      ++it;
    }
  }
  snake->sprites().clear();
}

/*
  Helper function to determine whehter a snake is a player
*/
bool isPlayer(const std::shared_ptr<Snake>& snake) {
  for (int i = 0; i < playersCount; i++) {
    if (snake && snake == spriteSnake[i]) {
      return true;
    }
  }
  return false;
}

/*
  Verdict if a sprite crushes on other objects
*/
bool crushVerdict(const std::shared_ptr<Sprite>& sprite, bool loose,
                  bool useAnimationBox) {
  if (!sprite) {
    return false;
  }
  const int x = sprite->x();
  const int y = sprite->y();
  SDL_Rect block;
  SDL_Rect box = useAnimationBox ? getSpriteAnimationBox(sprite.get())
                                 : getSpriteFeetBox(sprite.get());

  // If the sprite is out of the map, then consider it as crushed
  if (inr(x / UNIT, 0, n - 1) && inr(y / UNIT, 0, m - 1)) {
  } else {
    return true;
  }
  // Loop over the cells nearby the sprite to know better if it falls out of map
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      int xx = x / UNIT + dx, yy = y / UNIT + dy;
      if (inr(xx, 0, n - 1) && inr(yy, 0, m - 1)) {
        block = getMapRect(xx, yy);
        if (RectRectCross(&box, &block) && !hasMap[xx][yy]) {
          return true;
        }
      }
    }
  }

  // If it has crushed on other sprites
  for (int i = 0; i < spritesCount; i++) {
    const auto& snake = spriteSnake[i];
    if (!snake) {
      continue;
    }
    bool self = false;
    for (const auto& other : snake->sprites()) {
      if (!other) {
        continue;
      }
      if (other.get() != sprite.get()) {
        SDL_Rect otherBox = useAnimationBox ? getSpriteAnimationBox(other.get())
                                            : getSpriteFeetBox(other.get());
        if (RectRectCross(&box, &otherBox)) {
          if ((self && loose)) {
          } else {
            return true;
          }
        }
      } else {
        self = true;
      }
    }
  }
  return false;
}

void dropItem(const std::shared_ptr<Sprite>& sprite) {
  if (!sprite) {
    return;
  }
  double random = randDouble() * sprite->dropRate() * GAME_LUCKY;
#ifdef DBG
// printf("%lf\n", random);
#endif
  if (random < GAME_DROPOUT_YELLOW_FLASKS) {
    dropItemNearSprite(sprite.get(), ItemType::HpExtraMedicine);
  } else if (random > GAME_DROPOUT_WEAPONS) {
    dropItemNearSprite(sprite.get(), ItemType::Weapon);
  }
}

void invokeWeaponBuff(const std::shared_ptr<Snake>& src, Weapon* weapon,
                      const std::shared_ptr<Snake>& dest, int damage) {
  if (!weapon || !dest) {
    return;
  }
  double random;
  const auto& effects = weapon->effects();
  for (int i = BUFF_BEGIN; i < BUFF_END; i++) {
    random = randDouble();
    if (src && src->team() == GAME_MONSTERS_TEAM) {
      random *= GAME_MONSTERS_WEAPON_BUFF_ADJUST;
    }
    if (random < effects[i].chance) {
      switch (i) {
        case BUFF_FROZEN:
          freezeSnake(dest.get(), effects[i].duration);
          break;
        case BUFF_SLOWDOWN:
          slowDownSnake(dest.get(), effects[i].duration);
          break;
        case BUFF_DEFFENCE:
          if (src) {
            shieldSnake(src, effects[i].duration);
          }
          break;
        case BUFF_ATTACK:
          if (src) {
            attackUpSnkae(src, effects[i].duration);
          }
          break;
        default:
          break;
      }
    }
  }
}

void dealDamage(const std::shared_ptr<Snake>& src,
                const std::shared_ptr<Snake>& dest,
                const std::shared_ptr<Sprite>& target, int damage) {
  if (!dest || !target) {
    return;
  }
  double calcDamage = damage;
  if (dest->buffs()[BUFF_FROZEN]) {
    calcDamage *= GAME_FROZEN_DAMAGE_K;
  }
  if (src && src != spriteSnake[GAME_MONSTERS_TEAM]) {
    if (src->buffs()[BUFF_ATTACK]) {
      calcDamage *= GAME_BUFF_ATTACK_K;
    }
  }
  if (dest != spriteSnake[GAME_MONSTERS_TEAM]) {
    if (dest->buffs()[BUFF_DEFFENCE]) {
      calcDamage /= GAME_BUFF_DEFENSE_K;
    }
  }
  target->setHp(target->hp() - static_cast<int>(calcDamage));
  if (src) {
    src->score()->addDamage(static_cast<int>(calcDamage));
    if (target->hp() <= 0) {
      src->score()->addKilled(1);
    }
  }
  dest->score()->addStand(damage);
}

bool makeSnakeCross(const std::shared_ptr<Snake>& snake) {
  if (!snake) {
    return false;
  }
  auto& snakeSprites = snake->sprites();
  if (snakeSprites.empty()) {
    return false;
  }
  // Trap and Item ( everything related to block ) verdict
  for (int i = 0; i < SCREEN_WIDTH / UNIT; i++) {
    for (int j = 0; j < SCREEN_HEIGHT / UNIT; j++) {
      if (!hasMap[i][j]) {
        continue;
      }
      SDL_Rect block = {i * UNIT, j * UNIT, UNIT, UNIT};
      if (map[i][j].type() == BlockType::Trap && map[i][j].enabled()) {
        for (const auto& sprite : snakeSprites) {
          if (!sprite) {
            continue;
          }
          SDL_Rect spriteRect = getSpriteFeetBox(sprite.get());
          if (RectRectCross(&spriteRect, &block)) {
            dealDamage(nullptr, snake, sprite, spikeDamage);
          }
        }
      }
      if (isPlayer(snake)) {
        const auto headSprite = snakeSprites.front();
        if (!headSprite) {
          continue;
        }
        SDL_Rect headBox = getSpriteFeetBox(headSprite.get());
        if (itemMap[i][j].type() != ItemType::None) {
          if (RectRectCross(&headBox, &block)) {
            bool taken = true;
            auto animation = itemMap[i][j].animation();
            if (itemMap[i][j].type() == ItemType::Hero) {
              playAudio(AUDIO_COIN);
              appendSpriteToSnake(snake, itemMap[i][j].id(), 0, 0,
                                  Direction::Right);
              herosCount--;
              if (animation) {
                animationsList[RENDER_LIST_SPRITE_ID].remove(animation);
              }
            } else if (itemMap[i][j].type() == ItemType::HpMedicine ||
                       itemMap[i][j].type() == ItemType::HpExtraMedicine) {
              playAudio(AUDIO_MED);
              takeHpMedcine(snake.get(), GAME_HP_MEDICINE_DELTA,
                            itemMap[i][j].type() == ItemType::HpExtraMedicine);
              flasksCount -= itemMap[i][j].type() == ItemType::HpMedicine;

              if (animation) {
                animationsList[RENDER_LIST_MAP_ITEMS_ID].remove(animation);
              }
            } else if (itemMap[i][j].type() == ItemType::Weapon) {
              taken = takeWeapon(snake.get(), &itemMap[i][j]);
              if (taken) {
                playAudio(AUDIO_MED);
                if (animation) {
                  animationsList[RENDER_LIST_MAP_ITEMS_ID].remove(animation);
                }
              }
            }
            if (taken) {
              itemMap[i][j].setType(ItemType::None);
            }
          }
        }
      }
    }
  }
  // Death verdict, create death ani
  for (auto& sprite : snakeSprites) {
    if (!sprite || sprite->hp() > 0) {
      continue;
    }
    playAudio(AUDIO_HUMAN_DEATH);
    Texture* death = sprite->animation() ? sprite->animation()->origin() : nullptr;
    if (!death) {
      continue;
    }
    if (isPlayer(snake)) {
      death++;
    }
    dropItem(sprite);
    createAndPushAnimation(
        animationsList[RENDER_LIST_DEATH_ID], &textures[RES_SKULL], nullptr,
        LoopType::Infinite, 1,
        sprite->x() + randInt(-MAP_SKULL_SPILL_RANGE, MAP_SKULL_SPILL_RANGE),
        sprite->y() + randInt(-MAP_SKULL_SPILL_RANGE, MAP_SKULL_SPILL_RANGE),
        sprite->face() == Direction::Left ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL,
        0, At::BottomCenter);
    createAndPushAnimation(
        animationsList[RENDER_LIST_DEATH_ID], death, &effects[EFFECT_DEATH],
        LoopType::Once, SPRITE_ANIMATION_DURATION, sprite->x(), sprite->y(),
        sprite->face() == Direction::Right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL,
        0, At::BottomCenter);
    clearBindInAnimationsList(sprite, RENDER_LIST_EFFECT_ID);
    clearBindInAnimationsList(sprite, RENDER_LIST_SPRITE_ID);
    if (const auto animation = sprite->animation()) {
      animationsList[RENDER_LIST_SPRITE_ID].remove(animation);
    }
    sprite->setAnimation(nullptr);
  }
  // Remove dead sprites.
  for (auto it = snakeSprites.begin(); it != snakeSprites.end();) {
    if (!(*it) || (*it)->hp() > 0) {
      ++it;
      continue;
    }
    it = snakeSprites.erase(it);
  }
  if (snakeSprites.empty()) {
    return false;
  }
  const auto snakeHead = snakeSprites.front();
  bool die = crushVerdict(snakeHead, !isPlayer(snake), false);
  if (die) {
    for (const auto& sprite : snakeSprites) {
      if (sprite) {
        sprite->setHp(0);
      }
    }
  }

  return die;
}
bool makeBulletCross(const std::shared_ptr<Bullet>& bullet) {
  if (!bullet) {
    return false;
  }
  Weapon* weapon = bullet->parent();
  const auto animation = bullet->animation();
  if (!weapon || !animation || !animation->origin()) {
    return false;
  }
  bool hit = false;
  const int width = MIN(animation->origin()->width(), animation->origin()->height()) *
                    (animation->scaled() ? SCALE_FACTOR : 1) * 0.8;
  SDL_Rect bulletBox = {bullet->x() - width / 2, bullet->y() - width / 2, width,
                        width};
  if (!hasMap[bullet->x() / UNIT][bullet->y() / UNIT]) {
    if (const auto& deathAnimation = weapon->deathAnimation()) {
      auto effect = std::make_shared<Animation>(*deathAnimation);
      effect->setPosition(bullet->x(), bullet->y());
      pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
    }

    hit = true;
  }
  if (!hit) {
    for (int i = 0; i < spritesCount; i++) {
      const auto& snake = spriteSnake[i];
      if (!snake || bullet->team() == snake->team()) {
        continue;
      }
      for (const auto& target : snake->sprites()) {
        if (!target) {
          continue;
        }
        SDL_Rect box = getSpriteBoundBox(target.get());
        if (RectRectCross(&box, &bulletBox)) {
          if (const auto& deathAnimation = weapon->deathAnimation()) {
            auto effect = std::make_shared<Animation>(*deathAnimation);
            effect->setPosition(bullet->x(), bullet->y());
            pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
          }
          hit = true;
          if (weapon->type() == WeaponType::GunPoint ||
              weapon->type() == WeaponType::GunPointMulti) {
            dealDamage(bullet->owner(), snake, target, weapon->damage());
            invokeWeaponBuff(bullet->owner(), weapon, snake, weapon->damage());
            return hit;
          }
          break;
        }
      }
    }
  }
  if (hit) {
    playAudio(weapon->deathAudio());
    for (int i = 0; i < spritesCount; i++) {
      const auto& snake = spriteSnake[i];
      if (!snake || bullet->team() == snake->team()) {
        continue;
      }
      for (const auto& target : snake->sprites()) {
        if (!target) {
          continue;
        }
        if (distance(Point{target->x(), target->y()},
                     Point{bullet->x(), bullet->y()}) <=
            weapon->effectRange()) {
          dealDamage(bullet->owner(), snake, target, weapon->damage());
          invokeWeaponBuff(bullet->owner(), weapon, snake, weapon->damage());
        }
      }
    }
  }
  return hit;
}
void makeCross() {
  for (int i = 0; i < spritesCount; i++) {
    if (spriteSnake[i]) {
      makeSnakeCross(spriteSnake[i]);
    }
  }
  for (auto it = bullets.begin(); it != bullets.end();) {
    if (makeBulletCross(*it)) {
      if (const auto animation = (*it)->animation()) {
        animationsList[RENDER_LIST_EFFECT_ID].remove(animation);
      }
      it = bullets.erase(it);
      continue;
    }
    ++it;
  }
}
void moveSprite(const std::shared_ptr<Sprite>& sprite, int step) {
  if (!sprite) {
    return;
  }
  const Direction dir = sprite->direction();
  int x = sprite->x();
  int y = sprite->y();
  if (dir == Direction::Left) {
    x -= step;
  } else if (dir == Direction::Right) {
    x += step;
  } else if (dir == Direction::Up) {
    y -= step;
  } else if (dir == Direction::Down) {
    y += step;
  }
  sprite->setPosition(x, y);
}
void moveSnake(const std::shared_ptr<Snake>& snake) {
  if (!snake || snake->buffs()[BUFF_FROZEN]) {
    return;
  }
  int step = snake->moveStep();
  if (snake->buffs()[BUFF_SLOWDOWN]) {
    step = MAX(step / 2, 1);
  }
  for (const auto& sprite : snake->sprites()) {
    if (!sprite) {
      continue;
    }
    auto& buffer = sprite->positionBuffer();
    for (int move = 0; move < step; ++move) {
      if (buffer.size() > 0) {
        const PositionBufferSlot& slot = buffer.at(0);
        if (sprite->x() == slot.x && sprite->y() == slot.y) {
          sprite->setDirection(slot.direction);
        }
      }
      moveSprite(sprite, 1);
    }
  }
}
void updateMap() {
  const int maskedTime = static_cast<int>(renderFrames() % SPIKE_TIME_MASK);
  for (int i = 0; i < SCREEN_WIDTH / UNIT; i++) {
    for (int j = 0; j < SCREEN_HEIGHT / UNIT; j++) {
      if (hasMap[i][j] && map[i][j].type() == BlockType::Trap) {
        if (!maskedTime) {
          createAndPushAnimation(animationsList[RENDER_LIST_MAP_SPECIAL_ID],
                                 &textures[RES_FLOOR_SPIKE_OUT_ANI], nullptr,
                                 LoopType::Once, SPIKE_ANI_DURATION, i * UNIT,
                                 j * UNIT, SDL_FLIP_NONE, 0, At::TopLeft);
        } else if (maskedTime == SPIKE_ANI_DURATION - 1) {
          map[i][j].setEnabled(true);
          if (auto animation = map[i][j].animation()) {
            animation->setOrigin(&textures[RES_FLOOR_SPIKE_ENABLED]);
          }
        } else if (maskedTime == SPIKE_ANI_DURATION + SPIKE_OUT_INTERVAL - 1) {
          createAndPushAnimation(animationsList[RENDER_LIST_MAP_SPECIAL_ID],
                                 &textures[RES_FLOOR_SPIKE_IN_ANI], nullptr,
                                 LoopType::Once, SPIKE_ANI_DURATION, i * UNIT,
                                 j * UNIT, SDL_FLIP_NONE, 0, At::TopLeft);
          map[i][j].setEnabled(false);
          if (auto animation = map[i][j].animation()) {
            animation->setOrigin(&textures[RES_FLOOR_SPIKE_DISABLED]);
          }
        }
      }
    }
  }
}
void updateBuffDuration() {
  for (int i = 0; i < spritesCount; i++) {
    const auto& snake = spriteSnake[i];
    if (!snake) {
      continue;
    }
    auto& buffs = snake->buffs();
    for (int j = BUFF_BEGIN; j < BUFF_END; j++) {
      if (buffs[j] > 0) {
        buffs[j]--;
      }
    }
  }
}
void makeSpriteAttack(const std::shared_ptr<Sprite>& sprite,
                      const std::shared_ptr<Snake>& snake) {
  if (!sprite || !snake) {
    return;
  }
  Weapon* weapon = sprite->weapon();
  if (!weapon) {
    return;
  }
  if (static_cast<int>(renderFrames()) - sprite->lastAttack() < weapon->gap()) {
    return;
  }
  bool attacked = false;
  for (int i = 0; i < spritesCount; i++) {
    const auto& targetSnake = spriteSnake[i];
    if (!targetSnake || snake->team() == targetSnake->team()) {
      continue;
    }
    for (const auto& target : targetSnake->sprites()) {
      if (!target) {
        continue;
      }
      if (distance(Point{sprite->x(), sprite->y()},
                   Point{target->x(), target->y()}) > weapon->shootRange()) {
        continue;
      }
      const double rad = atan2(target->y() - sprite->y(),
                               target->x() - sprite->x());
      if (weapon->type() == WeaponType::SwordPoint ||
          weapon->type() == WeaponType::SwordRange) {
        if (const auto& deathAnimation = weapon->deathAnimation()) {
          auto effect = std::make_shared<Animation>(*deathAnimation);
          bindAnimationToSprite(effect, target, false);
          if (effect->angle() != -1) {
            effect->setAngle(rad * 180 / PI);
          }
          pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
        }
        dealDamage(snake, targetSnake, target, weapon->damage());
        invokeWeaponBuff(snake, weapon, targetSnake, weapon->damage());
        attacked = true;
        if (weapon->type() == WeaponType::SwordPoint) {
          goto ATTACK_END;
        }
      } else {
        const auto bulletAnimation = weapon->flyAnimation();
        auto bullet = std::make_shared<Bullet>(snake, weapon, sprite->x(),
                                               sprite->y(), rad, snake->team(),
                                               bulletAnimation);
        bullets.push_back(bullet);
        pushAnimationToRender(RENDER_LIST_EFFECT_ID, bullet->animation());
        attacked = true;
        if (weapon->type() != WeaponType::GunPointMulti) {
          goto ATTACK_END;
        }
      }
    }
  }
ATTACK_END:
  if (attacked) {
    if (const auto& birthAnimation = weapon->birthAnimation()) {
      auto effect = std::make_shared<Animation>(*birthAnimation);
      bindAnimationToSprite(effect, sprite, true);
      effect->setAt(At::BottomCenter);
      pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
    }
    if (weapon->type() == WeaponType::SwordPoint ||
        weapon->type() == WeaponType::SwordRange) {
      playAudio(weapon->deathAudio());
    } else {
      playAudio(weapon->birthAudio());
    }
    sprite->setLastAttack(static_cast<int>(renderFrames()));
  }
}
void makeSnakeAttack(const std::shared_ptr<Snake>& snake) {
  if (!snake || snake->buffs()[BUFF_FROZEN]) {
    return;
  }
  for (const auto& sprite : snake->sprites()) {
    makeSpriteAttack(sprite, snake);
  }
}
bool isWin() {
  if (playersCount != 1) {
    return false;
  }
  return spriteSnake[0] && spriteSnake[0]->num() >= GAME_WIN_NUM;
}

enum class GameStatus { StageClear, GameOver };
void setTerm(GameStatus s) {
  stopBgm();
  if (s == GameStatus::StageClear) {
    playAudio(AUDIO_WIN);
  } else {
    playAudio(AUDIO_LOSE);
  }
  status = s == GameStatus::StageClear ? 0 : 1;
  willTerm = true;
  termCount = RENDER_TERM_COUNT;
}
void pauseGame() {
  pauseSound();
  playAudio(AUDIO_BUTTON1);
  dim();
  const char msg[] = "Paused";
  extern SDL_Color WHITE;
  if (Text* rawText = createGameText(msg, WHITE)) {
    std::shared_ptr<Text> text(rawText, destroyGameText);
    renderCenteredText(text.get(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 1);
  }
  if (SDL_Renderer* sdlRenderer = renderer()) {
    SDL_RenderPresent(sdlRenderer);
  }
  SDL_Event e;
  for (bool quit = 0; !quit;) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN) {
        quit = true;
        break;
      }
    }
  }
  resumeSound();
  playAudio(AUDIO_BUTTON1);
}

std::optional<Direction> arrowsToDirection(int keyValue) {
  switch (keyValue) {
    case SDLK_LEFT:
      return Direction::Left;
    case SDLK_RIGHT:
      return Direction::Right;
    case SDLK_UP:
      return Direction::Up;
    case SDLK_DOWN:
      return Direction::Down;
  }
  return std::nullopt;
}

std::optional<Direction> wasdToDirection(int keyValue) {
  switch (keyValue) {
    case SDLK_a:
      return Direction::Left;
    case SDLK_d:
      return Direction::Right;
    case SDLK_w:
      return Direction::Up;
    case SDLK_s:
      return Direction::Down;
  }
  return std::nullopt;
}

bool handleLocalKeypress() {
  static SDL_Event e;
  bool quit = false;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      quit = true;
      setTerm(GameStatus::GameOver);
    } else if (e.type == SDL_KEYDOWN) {
      int keyValue = e.key.keysym.sym;
      if (keyValue == SDLK_ESCAPE) {
        pauseGame();
      }
      for (int id = 0; id <= 1 && id < playersCount; id++) {
        const auto& player = spriteSnake[id];
        if (!player || player->playerType() != PlayerType::Local) {
          continue;
        }
        if (player->buffs()[BUFF_FROZEN] || player->sprites().empty()) {
          continue;
        }
        const auto direction =
            id == 0 ? arrowsToDirection(keyValue) : wasdToDirection(keyValue);
        if (direction.has_value()) {
          sendPlayerMovePacket(id, static_cast<int>(*direction));
          auto head = player->sprites().front();
          if (head) {
            auto next = player->sprites().size() > 1
                            ? *std::next(player->sprites().begin())
                            : nullptr;
            head->enqueueDirectionChange(*direction, next);
          }
        }
      }
    }
  }
  return quit;
}

void handleLanKeypress() {
  static LanPacket packet;
  int status = recvLanPacket(&packet);
  if (!status) return;  // nop
  unsigned type = packet.type;
  if (type == HEADER_PLAYERMOVE) {
    auto* playerMovePacket = reinterpret_cast<PlayerMovePacket*>(&packet);
    const auto& player = spriteSnake[playerMovePacket->playerId];
    int direction = playerMovePacket->direction;
    fprintf(stderr, "recv: player move, %d, %d\n", playerMovePacket->playerId,
            direction);
    if (player && !player->sprites().empty()) {
      auto head = player->sprites().front();
      if (head) {
        auto next = player->sprites().size() > 1
                        ? *std::next(player->sprites().begin())
                        : nullptr;
        head->enqueueDirectionChange(static_cast<Direction>(direction), next);
      }
    }
  } else if (type == HEADER_GAMEOVER) {
    fprintf(stderr, "recv: game over, %d\n", -1);
    setTerm(GameStatus::GameOver);
  }
}

int gameLoop() {
  // int posx = 0, posy = SCREEN_HEIGHT / 2;
  // Game loop
  for (bool quit = 0; !quit;) {
    quit = handleLocalKeypress();
    if (quit) sendGameOverPacket(3);
    if (lanClientSocket != NULL) handleLanKeypress();

    updateMap();

    for (int i = 0; i < spritesCount; i++) {
      const auto& snake = spriteSnake[i];
      if (!snake || snake->sprites().empty()) {
        continue;  // some snakes killed by before but not clean up yet
      }
      if (i >= playersCount && renderFrames() % AI_DECIDE_RATE == 0) {
        AiInput(snake);
      }
      moveSnake(snake);
      makeSnakeAttack(snake);
    }
    for (const auto& bullet : bullets) {
      if (bullet) {
        bullet->update();
      }
    }
    if (renderFrames() % GAME_MAP_RELOAD_PERIOD == 0) {
      initItemMap(herosSetting - herosCount, flasksSetting - flasksCount);
    }
    for (int i = 0; i < spritesCount; i++) {
      const auto& snake = spriteSnake[i];
      updateAnimationOfSnake(snake);
      if (!snake) {
        continue;
      }
      if (snake->buffs()[BUFF_FROZEN]) {
        for (const auto& sprite : snake->sprites()) {
          if (sprite && sprite->animation()) {
            sprite->animation()->setCurrentFrame(
                sprite->animation()->currentFrame() - 1);
          }
        }
      }
    }
    makeCross();
    render();
    updateBuffDuration();
    for (int i = playersCount; i < spritesCount; i++) {
      if (!spriteSnake[i] || spriteSnake[i]->num() == 0) {
        destroySnake(spriteSnake[i]);
        spriteSnake[i] = nullptr;
        for (int j = i; j + 1 < spritesCount; j++) {
          spriteSnake[j] = spriteSnake[j + 1];
        }
        spriteSnake[spritesCount--] = nullptr;
      }
    }
    if (willTerm) {
      termCount--;
      if (!termCount) {
        break;
      }
    } else {
      int alivePlayer = -1;
      for (int i = 0; i < playersCount; i++) {
        if (!spriteSnake[i] || spriteSnake[i]->sprites().empty()) {
          setTerm(GameStatus::GameOver);
          sendGameOverPacket(alivePlayer);
          break;
        }
        alivePlayer = i;
      }
      if (isWin()) {
        setTerm(GameStatus::StageClear);
      }
    }
  }
  return status;
}
