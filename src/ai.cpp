#include "ai.h"

#include "helper.h"
#include "map.h"
#include "res.h"
#include "sprite.h"

#include <array>
#include <memory>

extern std::array<std::array<Block, MAP_SIZE>, MAP_SIZE> map;
extern std::array<std::array<Item, MAP_SIZE>, MAP_SIZE> itemMap;
extern bool hasMap[MAP_SIZE][MAP_SIZE];
extern std::array<std::array<bool, MAP_SIZE>, MAP_SIZE> hasEnemy;
extern int spikeDamage;
extern int playersCount;
extern const int n, m;
extern const int SCALE_FACTOR;

// Sprite
extern std::shared_ptr<Snake> spriteSnake[SPRITES_MAX_NUM];
extern int spritesCount;
double AI_LOCK_LIMIT;

int trapVerdict(const std::shared_ptr<Sprite>& sprite) {
  if (!sprite) {
    return 0;
  }
  int ret = 0;
  const int x = sprite->x();
  const int y = sprite->y();
  const SDL_Rect box = getSpriteFeetBox(sprite.get());
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      const int xx = x / UNIT + dx;
      const int yy = y / UNIT + dy;
      if (inr(xx, 0, n - 1) && inr(yy, 0, m - 1)) {
        const SDL_Rect block = getMapRect(xx, yy);
        if (RectRectCross(&box, &block) && hasMap[xx][yy] &&
            map[xx][yy].type() == BlockType::Trap) {
          ret += map[xx][yy].enabled() ? 2 : 1;
        }
      }
    }
  }
  return ret;
}

int getPowerfulPlayer() {
  int maxNum = 0;
  int mxCount = 0;
  int id = -1;
  for (int i = 0; i < playersCount; i++) {
    if (!spriteSnake[i]) {
      continue;
    }
    const int num = spriteSnake[i]->num();
    if (num > maxNum) {
      maxNum = num;
      mxCount = 1;
      id = i;
    } else if (num == maxNum) {
      mxCount++;
    }
  }
  if (id == -1 || mxCount != 1 || !spriteSnake[id]) {
    return -1;
  }
  return spriteSnake[id]->num() >= AI_LOCK_LIMIT ? id : -1;
}

int balanceVerdict(const std::shared_ptr<Sprite>& sprite, int id) {
  if (id == -1 || !spriteSnake[id] || spriteSnake[id]->sprites().empty()) {
    return 0;
  }
  const auto player = spriteSnake[id]->sprites().front();
  if (!player) {
    return 0;
  }
  int ret = 0;
  if (player->x() < sprite->x() && sprite->direction() == Direction::Left) {
    ret++;
  }
  if (player->x() > sprite->x() && sprite->direction() == Direction::Right) {
    ret++;
  }
  if (player->y() > sprite->y() && sprite->direction() == Direction::Down) {
    ret++;
  }
  if (player->y() < sprite->y() && sprite->direction() == Direction::Up) {
    ret++;
  }
  return ret;
}

int testOneMove(const std::shared_ptr<Snake>& snake, Direction direction) {
  if (!snake || snake->sprites().empty()) {
    return 0;
  }
  const auto snakeHead = snake->sprites().front();
  if (!snakeHead) {
    return 0;
  }
  const Direction recover = snakeHead->direction();
  snakeHead->setDirection(direction);
  int crush = 0;
  int trap = 0;
  int playerBalance = 0;
  const int powerful = getPowerfulPlayer();
  for (int i = 1; i <= AI_PREDICT_STEPS; i++) {
    moveSprite(snakeHead, snake->moveStep() * i);
    updateAnimationOfSprite(snakeHead);
    crush -= crushVerdict(snakeHead, false, true) * 1000;
    trap -= trapVerdict(snakeHead);
    playerBalance += balanceVerdict(snakeHead, powerful) * 10;
    // revoke position
    moveSprite(snakeHead, -snake->moveStep() * i);
    updateAnimationOfSprite(snakeHead);
  }
  snakeHead->setDirection(recover);
  return trap + crush + playerBalance;
}

int compareChoiceByValue(const void* x, const void* y) {
  const auto* a = static_cast<const Choice*>(x);
  const auto* b = static_cast<const Choice*>(y);
  return b->value - a->value;
}

void AiInput(const std::shared_ptr<Snake>& snake) {
  if (!snake || snake->sprites().empty()) {
    return;
  }
  const auto snakeHead = snake->sprites().front();
  if (!snakeHead) {
    return;
  }
  const Direction currentDirection = snakeHead->direction();
  const int originValue = testOneMove(snake, currentDirection);
  bool change = originValue < 0;
  if (randDouble() < AI_PATH_RANDOM) {
    change = true;
  }
  if (change) {
    std::array<Choice, 4> choices{};
    int count = 0;
    for (const Direction candidate :
         {Direction::Left, Direction::Right, Direction::Up, Direction::Down}) {
      if (candidate != currentDirection) {
        const int value = testOneMove(snake, candidate);
        if (value >= originValue) {
          choices[count++] = {value, candidate};
        }
      }
    }
    if (count) {
      int maxValue = choices[0].value;
      int nowChoice = 0;
      for (int i = 0; i < count; i++) {
        if (choices[i].value > maxValue) {
          maxValue = choices[i].value;
          nowChoice = i;
        }
      }
      if (maxValue > originValue) {
        std::shared_ptr<Sprite> nextSprite = nullptr;
        if (snake->sprites().size() > 1) {
          auto it = snake->sprites().begin();
          ++it;
          nextSprite = *it;
        }
        snakeHead->enqueueDirectionChange(choices[nowChoice].direction,
                                          nextSprite);
      }
    }
  }
}
