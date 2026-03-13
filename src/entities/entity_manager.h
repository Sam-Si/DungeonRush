#ifndef SNAKE_ENTITIES_ENTITY_MANAGER_H_
#define SNAKE_ENTITIES_ENTITY_MANAGER_H_

#include "player.h"
#include "sprite.h"
#include "bullet.h"
#include "adt.h"

#include <array>
#include <memory>

namespace snake {
namespace entities {

constexpr int kSpritesMaxNum = 1024;

// ============================================================================
// EntityManager - Manages all game entities (snakes, bullets)
// ============================================================================
class EntityManager {
 public:
  EntityManager() = default;
  EntityManager(const EntityManager&) = delete;
  EntityManager& operator=(const EntityManager&) = delete;
  EntityManager(EntityManager&&) = default;
  EntityManager& operator=(EntityManager&&) = default;
  ~EntityManager() = default;

  // Snake management
  void addSnake(const std::shared_ptr<Snake>& snake);
  void removeSnake(int index);
  std::shared_ptr<Snake> getSnake(int index) const;
  int snakeCount() const;
  int playerCount() const;
  void setPlayerCount(int count);
  void incrementSpriteCount();
  int spriteCount() const;

  // Bullet management
  void addBullet(const std::shared_ptr<Bullet>& bullet);
  void removeBullet(const std::shared_ptr<Bullet>& bullet);
  BulletList& bullets();
  const BulletList& bullets() const;

  // Entity access
  std::array<std::shared_ptr<Snake>, kSpritesMaxNum>& snakes();
  const std::array<std::shared_ptr<Snake>, kSpritesMaxNum>& snakes() const;

  void clear();

 private:
  std::array<std::shared_ptr<Snake>, kSpritesMaxNum> snakes_{};
  BulletList bullets_{};
  int spritesCount_ = 0;
  int playersCount_ = 0;
};

}  // namespace entities
}  // namespace snake

#endif  // SNAKE_ENTITIES_ENTITY_MANAGER_H_
