#include "systems/collision_manager.h"

#include "helper.h"
#include "map.h"
#include "res.h"

extern const int SCALE_FACTOR;
extern const int n, m;
extern bool hasMap[MAP_SIZE][MAP_SIZE];
extern std::array<std::array<Block, MAP_SIZE>, MAP_SIZE> map;

namespace snake {
namespace systems {

bool CollisionManager::checkCrush(const std::shared_ptr<Sprite>& sprite,
                                   bool loose, bool useAnimationBox,
                                   entities::EntityManager& entityManager) {
  if (!sprite) {
    return false;
  }
  const int x = sprite->x();
  const int y = sprite->y();
  SDL_Rect block;
  SDL_Rect box = useAnimationBox ? getSpriteAnimationBox(sprite.get())
                                 : getSpriteFeetBox(sprite.get());

  // If the sprite is out of the map, then consider it as crushed
  if (!inr(x / UNIT, 0, n - 1) || !inr(y / UNIT, 0, m - 1)) {
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
  const int count = entityManager.snakeCount();
  for (int i = 0; i < count; i++) {
    const auto& snake = entityManager.getSnake(i);
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
            // continue checking
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

bool CollisionManager::isPlayer(const std::shared_ptr<Snake>& snake,
                                 entities::EntityManager& entityManager) const {
  const int playerCount = entityManager.playerCount();
  for (int i = 0; i < playerCount; i++) {
    if (snake && snake == entityManager.getSnake(i)) {
      return true;
    }
  }
  return false;
}

}  // namespace systems
}  // namespace snake
