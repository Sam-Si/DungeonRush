#ifndef SNAKE_ENTITY_FACTORY_H_
#define SNAKE_ENTITY_FACTORY_H_

#include "types.h"
#include "sprite.h"
#include "player.h"
#include "weapon.h"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
// Forward Declarations
// ============================================================================
class EntityPrototype;
class MonsterPrototype;
class WeaponPrototype;
class ItemPrototype;
class PrototypeRegistry;
class EntityFactory;
class MonsterFactory;
class WeaponFactory;
class ItemFactory;
struct EntityGenerationConfig;
struct SpawnContext;

// ============================================================================
// SpawnContext - Context for entity spawning
// ============================================================================
struct SpawnContext {
  int x = 0;
  int y = 0;
  int team = 0;
  PlayerType playerType = PlayerType::Computer;
  Direction direction = Direction::Right;
  double levelScaling = 1.0;
  double difficultyFactor = 1.0;
};

// ============================================================================
// EntityPrototype - Abstract base class for entity templates
// ============================================================================
class EntityPrototype {
 public:
  EntityPrototype() = default;
  EntityPrototype(const EntityPrototype&) = default;
  EntityPrototype& operator=(const EntityPrototype&) = default;
  EntityPrototype(EntityPrototype&&) noexcept = default;
  EntityPrototype& operator=(EntityPrototype&&) noexcept = default;
  virtual ~EntityPrototype() = default;

  virtual std::unique_ptr<EntityPrototype> clone() const = 0;
  virtual std::string getPrototypeId() const = 0;
  virtual std::string getCategory() const = 0;

  void setPrototypeId(const std::string& id) { prototypeId_ = id; }
  void setCategory(const std::string& category) { category_ = category; }
  void setWeight(double weight) { weight_ = weight; }
  void setMinLevel(int level) { minLevel_ = level; }
  void setMaxLevel(int level) { maxLevel_ = level; }

  double getWeight() const { return weight_; }
  int getMinLevel() const { return minLevel_; }
  int getMaxLevel() const { return maxLevel_; }

 protected:
  std::string prototypeId_;
  std::string category_;
  double weight_ = 1.0;
  int minLevel_ = 0;
  int maxLevel_ = 999;
};

// ============================================================================
// MonsterPrototype - Template for monster generation
// ============================================================================
class MonsterPrototype final : public EntityPrototype {
 public:
  MonsterPrototype() = default;
  MonsterPrototype(const MonsterPrototype&) = default;
  MonsterPrototype& operator=(const MonsterPrototype&) = default;
  MonsterPrototype(MonsterPrototype&&) noexcept = default;
  MonsterPrototype& operator=(MonsterPrototype&&) noexcept = default;
  ~MonsterPrototype() override = default;

  std::unique_ptr<EntityPrototype> clone() const override {
    return std::make_unique<MonsterPrototype>(*this);
  }

  std::string getPrototypeId() const override { return prototypeId_; }
  std::string getCategory() const override { return category_; }

  void setSpriteId(int id) { spriteId_ = id; }
  void setMoveStep(int step) { moveStep_ = step; }
  void setBaseHp(int hp) { baseHp_ = hp; }
  void setWeaponId(int id) { weaponId_ = id; }
  void setDropRate(double rate) { dropRate_ = rate; }
  void setIsBoss(bool boss) { isBoss_ = boss; }

  int getSpriteId() const { return spriteId_; }
  int getMoveStep() const { return moveStep_; }
  int getBaseHp() const { return baseHp_; }
  int getWeaponId() const { return weaponId_; }
  double getDropRate() const { return dropRate_; }
  bool getIsBoss() const { return isBoss_; }

 private:
  int spriteId_ = 0;
  int moveStep_ = 1;
  int baseHp_ = 100;
  int weaponId_ = WEAPON_MONSTER_CLAW;
  double dropRate_ = 1.0;
  bool isBoss_ = false;
};

// ============================================================================
// WeaponPrototype - Template for weapon generation
// ============================================================================
class WeaponPrototype final : public EntityPrototype {
 public:
  WeaponPrototype() = default;
  WeaponPrototype(const WeaponPrototype&) = default;
  WeaponPrototype& operator=(const WeaponPrototype&) = default;
  WeaponPrototype(WeaponPrototype&&) noexcept = default;
  WeaponPrototype& operator=(WeaponPrototype&&) noexcept = default;
  ~WeaponPrototype() override = default;

  std::unique_ptr<EntityPrototype> clone() const override {
    return std::make_unique<WeaponPrototype>(*this);
  }

  std::string getPrototypeId() const override { return prototypeId_; }
  std::string getCategory() const override { return category_; }

  void setWeaponId(int id) { weaponId_ = id; }
  void setWeaponType(WeaponType type) { weaponType_ = type; }
  void setShootRange(int range) { shootRange_ = range; }
  void setEffectRange(int range) { effectRange_ = range; }
  void setBaseDamage(int damage) { baseDamage_ = damage; }
  void setGap(int gap) { gap_ = gap; }
  void setBulletSpeed(int speed) { bulletSpeed_ = speed; }
  void setTextureId(int id) { textureId_ = id; }
  void setBelongSpriteId(int id) { belongSpriteId_ = id; }
  void setBirthAudio(int audio) { birthAudio_ = audio; }
  void setDeathAudio(int audio) { deathAudio_ = audio; }
  void setBuffChance(int buffIndex, double chance) {
    if (buffIndex >= 0 && buffIndex < BUFF_END) {
      buffChances_[buffIndex] = chance;
    }
  }
  void setBuffDuration(int buffIndex, int duration) {
    if (buffIndex >= 0 && buffIndex < BUFF_END) {
      buffDurations_[buffIndex] = duration;
    }
  }

  int getWeaponId() const { return weaponId_; }
  WeaponType getWeaponType() const { return weaponType_; }
  int getShootRange() const { return shootRange_; }
  int getEffectRange() const { return effectRange_; }
  int getBaseDamage() const { return baseDamage_; }
  int getGap() const { return gap_; }
  int getBulletSpeed() const { return bulletSpeed_; }
  int getTextureId() const { return textureId_; }
  int getBelongSpriteId() const { return belongSpriteId_; }
  int getBirthAudio() const { return birthAudio_; }
  int getDeathAudio() const { return deathAudio_; }
  const std::array<double, BUFF_END>& getBuffChances() const { return buffChances_; }
  const std::array<int, BUFF_END>& getBuffDurations() const { return buffDurations_; }

 private:
  int weaponId_ = 0;
  WeaponType weaponType_ = WeaponType::SwordPoint;
  int shootRange_ = 64;
  int effectRange_ = 32;
  int baseDamage_ = 10;
  int gap_ = 30;
  int bulletSpeed_ = 4;
  int textureId_ = 0;
  int belongSpriteId_ = 0;
  int birthAudio_ = -1;
  int deathAudio_ = -1;
  std::array<double, BUFF_END> buffChances_{};
  std::array<int, BUFF_END> buffDurations_{};
};

// ============================================================================
// ItemPrototype - Template for item generation
// ============================================================================
class ItemPrototype final : public EntityPrototype {
 public:
  ItemPrototype() = default;
  ItemPrototype(const ItemPrototype&) = default;
  ItemPrototype& operator=(const ItemPrototype&) = default;
  ItemPrototype(ItemPrototype&&) noexcept = default;
  ItemPrototype& operator=(ItemPrototype&&) noexcept = default;
  ~ItemPrototype() override = default;

  std::unique_ptr<EntityPrototype> clone() const override {
    return std::make_unique<ItemPrototype>(*this);
  }

  std::string getPrototypeId() const override { return prototypeId_; }
  std::string getCategory() const override { return category_; }

  void setItemType(ItemType type) { itemType_ = type; }
  void setItemId(int id) { itemId_ = id; }
  void setBelongSpriteId(int id) { belongSpriteId_ = id; }
  void setTextureId(int id) { textureId_ = id; }
  void setHpRestorePercent(int percent) { hpRestorePercent_ = percent; }
  void setIsExtra(bool extra) { isExtra_ = extra; }

  ItemType getItemType() const { return itemType_; }
  int getItemId() const { return itemId_; }
  int getBelongSpriteId() const { return belongSpriteId_; }
  int getTextureId() const { return textureId_; }
  int getHpRestorePercent() const { return hpRestorePercent_; }
  bool getIsExtra() const { return isExtra_; }

 private:
  ItemType itemType_ = ItemType::None;
  int itemId_ = 0;
  int belongSpriteId_ = 0;
  int textureId_ = 0;
  int hpRestorePercent_ = 0;
  bool isExtra_ = false;
};

// ============================================================================
// PrototypeRegistry - Registry for storing and retrieving entity prototypes
// Uses Prototype pattern: stores templates that can be cloned
// ============================================================================
class PrototypeRegistry {
 public:
  PrototypeRegistry() = default;
  PrototypeRegistry(const PrototypeRegistry&) = delete;
  PrototypeRegistry& operator=(const PrototypeRegistry&) = delete;
  PrototypeRegistry(PrototypeRegistry&&) noexcept = default;
  PrototypeRegistry& operator=(PrototypeRegistry&&) noexcept = default;
  ~PrototypeRegistry() = default;

  static PrototypeRegistry& getInstance();

  void registerPrototype(const std::string& id, std::unique_ptr<EntityPrototype> prototype);
  
  const EntityPrototype* getPrototype(const std::string& id) const;
  const MonsterPrototype* getMonsterPrototype(const std::string& id) const;
  const WeaponPrototype* getWeaponPrototype(const std::string& id) const;
  const ItemPrototype* getItemPrototype(const std::string& id) const;

  std::vector<const MonsterPrototype*> getMonsterPrototypesByCategory(
      const std::string& category) const;
  std::vector<const WeaponPrototype*> getWeaponPrototypesByCategory(
      const std::string& category) const;
  std::vector<const ItemPrototype*> getItemPrototypesByCategory(
      const std::string& category) const;

  std::vector<const MonsterPrototype*> getAllMonsterPrototypes() const;
  std::vector<const WeaponPrototype*> getAllWeaponPrototypes() const;
  std::vector<const ItemPrototype*> getAllItemPrototypes() const;

  void clear();
  void initializeDefaultPrototypes();

 private:
  std::map<std::string, std::unique_ptr<EntityPrototype>> prototypes_;
};

// ============================================================================
// EntityGenerationConfig - Configuration for level-based entity generation
// ============================================================================
struct EntityGenerationConfig {
  int minSnakeLength = 2;
  int maxSnakeLength = 4;
  int minBossLength = 1;
  int maxBossLength = 1;
  
  std::vector<std::string> monsterCategories;
  std::vector<std::string> bossCategories;
  std::vector<std::string> weaponCategories;
  std::vector<std::string> itemCategories;
  
  double hpScaling = 1.0;
  double damageScaling = 1.0;
  double dropRateScaling = 1.0;
  double monsterGenFactor = 1.0;
  
  int heroCount = 8;
  int flaskCount = 6;
  int enemyCount = 25;
  int bossCount = 2;
  
  double yellowFlaskDropRate = 0.3;
  double weaponDropRate = 0.7;
  double luckyFactor = 1.0;
};

// ============================================================================
// EntityFactory - Abstract factory interface
// ============================================================================
class EntityFactory {
 public:
  virtual ~EntityFactory() = default;

  virtual std::shared_ptr<Snake> createSnake(const SpawnContext& context) const = 0;
  virtual std::shared_ptr<Sprite> createSprite(const SpawnContext& context) const = 0;
};

// ============================================================================
// MonsterFactory - Concrete factory for creating monsters
// ============================================================================
class MonsterFactory final : public EntityFactory {
 public:
  MonsterFactory() = default;
  MonsterFactory(const MonsterFactory&) = delete;
  MonsterFactory& operator=(const MonsterFactory&) = delete;
  MonsterFactory(MonsterFactory&&) noexcept = default;
  MonsterFactory& operator=(MonsterFactory&&) noexcept = default;
  ~MonsterFactory() override = default;

  std::shared_ptr<Snake> createSnake(const SpawnContext& context) const override;
  std::shared_ptr<Sprite> createSprite(const SpawnContext& context) const override;

  std::shared_ptr<Snake> createSnakeFromPrototype(
      const MonsterPrototype& proto, const SpawnContext& context) const;
  std::shared_ptr<Sprite> createSpriteFromPrototype(
      const MonsterPrototype& proto, const SpawnContext& context) const;

  std::shared_ptr<Snake> createRandomMonster(
      int x, int y, int gameLevel, double difficultyFactor) const;
  std::shared_ptr<Snake> createRandomBoss(
      int x, int y, int gameLevel, double difficultyFactor) const;

 private:
  const MonsterPrototype* selectRandomPrototype(
      const std::vector<const MonsterPrototype*>& prototypes, int gameLevel) const;
};

// ============================================================================
// WeaponFactory - Concrete factory for creating weapons
// ============================================================================
class WeaponFactory final {
 public:
  WeaponFactory() = default;
  WeaponFactory(const WeaponFactory&) = delete;
  WeaponFactory& operator=(const WeaponFactory&) = delete;
  WeaponFactory(WeaponFactory&&) noexcept = default;
  WeaponFactory& operator=(WeaponFactory&&) noexcept = default;
  ~WeaponFactory() = default;

  std::unique_ptr<Weapon> createWeapon(const WeaponPrototype& proto) const;
  std::unique_ptr<Weapon> createWeaponFromId(int weaponId) const;
  std::unique_ptr<Weapon> createRandomWeapon(int gameLevel) const;
  std::unique_ptr<Weapon> createWeaponForSprite(int spriteId) const;

 private:
  const WeaponPrototype* selectRandomPrototype(int gameLevel) const;
};

// ============================================================================
// ItemFactory - Concrete factory for creating items
// ============================================================================
class ItemFactory final {
 public:
  ItemFactory() = default;
  ItemFactory(const ItemFactory&) = delete;
  ItemFactory& operator=(const ItemFactory&) = delete;
  ItemFactory(ItemFactory&&) noexcept = default;
  ItemFactory& operator=(ItemFactory&&) noexcept = default;
  ~ItemFactory() = default;

  Item createItem(const ItemPrototype& proto, int x, int y) const;
  Item createHpMedicine(int x, int y, bool extra = false) const;
  Item createWeaponItem(int x, int y, int gameLevel) const;
  Item createHeroItem(int x, int y) const;
  Item createRandomItem(int x, int y, int gameLevel) const;

 private:
  const ItemPrototype* selectRandomPrototype(ItemType type, int gameLevel) const;
};

// ============================================================================
// EntitySpawner - High-level interface for spawning entities using factories
// ============================================================================
class EntitySpawner {
 public:
  EntitySpawner();
  EntitySpawner(const EntitySpawner&) = delete;
  EntitySpawner& operator=(const EntitySpawner&) = delete;
  EntitySpawner(EntitySpawner&&) noexcept = default;
  EntitySpawner& operator=(EntitySpawner&&) noexcept = default;
  ~EntitySpawner() = default;

  void initializeLevel(int level, int stage);
  
  std::shared_ptr<Snake> spawnMonster(int x, int y);
  std::shared_ptr<Snake> spawnBoss(int x, int y);
  Item spawnHeroItem(int x, int y);
  Item spawnFlaskItem(int x, int y, bool extra = false);
  Item spawnWeaponItem(int x, int y);
  
  const EntityGenerationConfig& getConfig() const { return config_; }
  void setConfig(const EntityGenerationConfig& config) { config_ = config; }

 private:
  EntityGenerationConfig config_;
  std::unique_ptr<MonsterFactory> monsterFactory_;
  std::unique_ptr<WeaponFactory> weaponFactory_;
  std::unique_ptr<ItemFactory> itemFactory_;
  int currentLevel_ = 0;
  int currentStage_ = 0;
};

// ============================================================================
// Initialization Functions
// ============================================================================
void initializeEntityPrototypes();

#endif  // SNAKE_ENTITY_FACTORY_H_
