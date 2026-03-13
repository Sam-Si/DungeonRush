#ifndef SNAKE_SYSTEMS_BUFF_MANAGER_H_
#define SNAKE_SYSTEMS_BUFF_MANAGER_H_

#include "entities/entity_manager.h"
#include "player.h"
#include "weapon.h"

namespace snake {
namespace systems {

// ============================================================================
// BuffManager - Handles buff effects on snakes
// ============================================================================
class BuffManager {
 public:
  BuffManager() = default;
  BuffManager(const BuffManager&) = delete;
  BuffManager& operator=(const BuffManager&) = delete;
  BuffManager(BuffManager&&) = default;
  BuffManager& operator=(BuffManager&&) = default;
  ~BuffManager() = default;

  void freezeSnake(Snake* snake, int duration);
  void slowDownSnake(Snake* snake, int duration);
  void shieldSnake(const std::shared_ptr<Snake>& snake, int duration);
  void attackUpSnake(const std::shared_ptr<Snake>& snake, int duration);
  void updateBuffDurations(entities::EntityManager& entityManager);
  void invokeWeaponBuff(const std::shared_ptr<Snake>& src, const Weapon& weapon,
                        const std::shared_ptr<Snake>& dest, int damage);

 private:
};

}  // namespace systems
}  // namespace snake

#endif  // SNAKE_SYSTEMS_BUFF_MANAGER_H_
