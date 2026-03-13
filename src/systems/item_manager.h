#ifndef SNAKE_SYSTEMS_ITEM_MANAGER_H_
#define SNAKE_SYSTEMS_ITEM_MANAGER_H_

#include "entities/entity_manager.h"
#include "sprite.h"
#include "types.h"

#include <array>

namespace snake {
namespace systems {

// ============================================================================
// ItemManager - Handles item generation and management
// ============================================================================
class ItemManager {
 public:
  ItemManager() = default;
  ItemManager(const ItemManager&) = delete;
  ItemManager& operator=(const ItemManager&) = delete;
  ItemManager(ItemManager&&) = default;
  ItemManager& operator=(ItemManager&&) = default;
  ~ItemManager() = default;

  void initItems(int heroCount, int flaskCount);
  void clearItems();
  void generateHeroItem(int x, int y);
  void generateItem(int x, int y, ItemType type);
  void dropItemNearSprite(const Sprite* sprite, ItemType itemType);
  bool checkItemPickup(const std::shared_ptr<Snake>& snake,
                       entities::EntityManager& entityManager);

  std::array<std::array<Item, MAP_SIZE>, MAP_SIZE>& itemMap();
  const std::array<std::array<Item, MAP_SIZE>, MAP_SIZE>& itemMap() const;

  int heroCount() const;
  void setHeroCount(int count);
  int flaskCount() const;
  void setFlaskCount(int count);

 private:
  std::array<std::array<Item, MAP_SIZE>, MAP_SIZE> itemMap_{};
  int herosCount_ = 0;
  int flasksCount_ = 0;
};

}  // namespace systems
}  // namespace snake

#endif  // SNAKE_SYSTEMS_ITEM_MANAGER_H_
