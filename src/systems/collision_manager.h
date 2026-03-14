#ifndef SNAKE_SYSTEMS_COLLISION_MANAGER_H_
#define SNAKE_SYSTEMS_COLLISION_MANAGER_H_

#include "entities/entity_manager.h"
#include "sprite.h"

namespace snake {
namespace systems {

// ============================================================================
// CollisionManager - Handles all collision detection
// ============================================================================
class CollisionManager {
 public:
  CollisionManager() = default;
  CollisionManager(const CollisionManager&) = delete;
  CollisionManager& operator=(const CollisionManager&) = delete;
  CollisionManager(CollisionManager&&) = default;
  CollisionManager& operator=(CollisionManager&&) = default;
  ~CollisionManager() = default;

  bool checkCrush(const std::shared_ptr<Sprite>& sprite, bool loose,
                  bool useAnimationBox, entities::EntityManager& entityManager);
  bool isPlayer(const std::shared_ptr<Snake>& snake, entities::EntityManager& entityManager) const;

 private:
};

}  // namespace systems
}  // namespace snake

#endif  // SNAKE_SYSTEMS_COLLISION_MANAGER_H_
