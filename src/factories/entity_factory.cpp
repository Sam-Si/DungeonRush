#include "entity_factory.h"

#include "game.h"
#include "helper.h"
#include "render.h"
#include "res.h"

#include <algorithm>
#include <random>

// ============================================================================
// External Declarations (from game.cpp and other files)
// ============================================================================
extern const int SCALE_FACTOR;
extern const int n, m;
extern Texture textures[TEXTURES_SIZE];
extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;
extern Effect effects[];
extern Weapon weapons[WEAPONS_SIZE];
extern Sprite commonSprites[COMMON_SPRITE_SIZE];
extern std::array<std::array<Item, MAP_SIZE>, MAP_SIZE> itemMap;
extern bool hasMap[MAP_SIZE][MAP_SIZE];
extern std::array<std::array<Block, MAP_SIZE>, MAP_SIZE> map;
extern std::array<std::array<bool, MAP_SIZE>, MAP_SIZE> hasEnemy;

// ============================================================================
// PrototypeRegistry Implementation
// ============================================================================

PrototypeRegistry& PrototypeRegistry::getInstance() {
  static PrototypeRegistry instance;
  return instance;
}

void PrototypeRegistry::registerPrototype(
    const std::string& id, std::unique_ptr<EntityPrototype> prototype) {
  prototypes_[id] = std::move(prototype);
}

const EntityPrototype* PrototypeRegistry::getPrototype(const std::string& id) const {
  const auto it = prototypes_.find(id);
  if (it != prototypes_.end()) {
    return it->second.get();
  }
  return nullptr;
}

const MonsterPrototype* PrototypeRegistry::getMonsterPrototype(
    const std::string& id) const {
  return dynamic_cast<const MonsterPrototype*>(getPrototype(id));
}

const WeaponPrototype* PrototypeRegistry::getWeaponPrototype(
    const std::string& id) const {
  return dynamic_cast<const WeaponPrototype*>(getPrototype(id));
}

const ItemPrototype* PrototypeRegistry::getItemPrototype(
    const std::string& id) const {
  return dynamic_cast<const ItemPrototype*>(getPrototype(id));
}

std::vector<const MonsterPrototype*> PrototypeRegistry::getMonsterPrototypesByCategory(
    const std::string& category) const {
  std::vector<const MonsterPrototype*> result;
  for (const auto& [id, proto] : prototypes_) {
    if (proto->getCategory() == category) {
      if (const auto* monster = dynamic_cast<const MonsterPrototype*>(proto.get())) {
        result.push_back(monster);
      }
    }
  }
  return result;
}

std::vector<const WeaponPrototype*> PrototypeRegistry::getWeaponPrototypesByCategory(
    const std::string& category) const {
  std::vector<const WeaponPrototype*> result;
  for (const auto& [id, proto] : prototypes_) {
    if (proto->getCategory() == category) {
      if (const auto* weapon = dynamic_cast<const WeaponPrototype*>(proto.get())) {
        result.push_back(weapon);
      }
    }
  }
  return result;
}

std::vector<const ItemPrototype*> PrototypeRegistry::getItemPrototypesByCategory(
    const std::string& category) const {
  std::vector<const ItemPrototype*> result;
  for (const auto& [id, proto] : prototypes_) {
    if (proto->getCategory() == category) {
      if (const auto* item = dynamic_cast<const ItemPrototype*>(proto.get())) {
        result.push_back(item);
      }
    }
  }
  return result;
}

std::vector<const MonsterPrototype*> PrototypeRegistry::getAllMonsterPrototypes() const {
  std::vector<const MonsterPrototype*> result;
  for (const auto& [id, proto] : prototypes_) {
    if (const auto* monster = dynamic_cast<const MonsterPrototype*>(proto.get())) {
      result.push_back(monster);
    }
  }
  return result;
}

std::vector<const WeaponPrototype*> PrototypeRegistry::getAllWeaponPrototypes() const {
  std::vector<const WeaponPrototype*> result;
  for (const auto& [id, proto] : prototypes_) {
    if (const auto* weapon = dynamic_cast<const WeaponPrototype*>(proto.get())) {
      result.push_back(weapon);
    }
  }
  return result;
}

std::vector<const ItemPrototype*> PrototypeRegistry::getAllItemPrototypes() const {
  std::vector<const ItemPrototype*> result;
  for (const auto& [id, proto] : prototypes_) {
    if (const auto* item = dynamic_cast<const ItemPrototype*>(proto.get())) {
      result.push_back(item);
    }
  }
  return result;
}

void PrototypeRegistry::clear() {
  prototypes_.clear();
}

// ============================================================================
// MonsterFactory Implementation
// ============================================================================

std::shared_ptr<Snake> MonsterFactory::createSnake(const SpawnContext& context) const {
  return std::make_shared<Snake>(context.levelScaling, GAME_MONSTERS_TEAM,
                                 context.playerType);
}

std::shared_ptr<Sprite> MonsterFactory::createSprite(const SpawnContext& context) const {
  auto sprite = std::make_shared<Sprite>(commonSprites[SPRITE_TINY_ZOMBIE],
                                         context.x, context.y);
  sprite->setDirection(context.direction);
  if (context.direction == Direction::Left) {
    sprite->setFace(Direction::Left);
  }
  return sprite;
}

std::shared_ptr<Snake> MonsterFactory::createSnakeFromPrototype(
    const MonsterPrototype& proto, const SpawnContext& context) const {
  const int scaledHp = static_cast<int>(proto.getBaseHp() * context.levelScaling);
  const int step = proto.getMoveStep();

  auto snake = std::make_shared<Snake>(step, GAME_MONSTERS_TEAM, PlayerType::Computer);
  snake->score()->setGot(0);

  return snake;
}

std::shared_ptr<Sprite> MonsterFactory::createSpriteFromPrototype(
    const MonsterPrototype& proto, const SpawnContext& context) const {
  const int spriteId = proto.getSpriteId();
  auto sprite = std::make_shared<Sprite>(commonSprites[spriteId], context.x, context.y);
  
  sprite->setDirection(context.direction);
  if (context.direction == Direction::Left) {
    sprite->setFace(Direction::Left);
  }
  
  const int scaledHp = static_cast<int>(proto.getBaseHp() * context.levelScaling);
  sprite->setTotalHp(scaledHp);
  sprite->setHp(scaledHp);
  sprite->setDropRate(proto.getDropRate() * context.difficultyFactor);
  
  if (proto.getWeaponId() >= 0 && proto.getWeaponId() < WEAPONS_SIZE) {
    sprite->setWeapon(&weapons[proto.getWeaponId()]);
  }
  
  if (sprite->animation()) {
    pushAnimationToRender(RENDER_LIST_SPRITE_ID, sprite->animation());
  }
  
  return sprite;
}

const MonsterPrototype* MonsterFactory::selectRandomPrototype(
    const std::vector<const MonsterPrototype*>& prototypes, int gameLevel) const {
  if (prototypes.empty()) {
    return nullptr;
  }
  
  std::vector<const MonsterPrototype*> validPrototypes;
  for (const auto* proto : prototypes) {
    if (proto && gameLevel >= proto->getMinLevel() && gameLevel <= proto->getMaxLevel()) {
      validPrototypes.push_back(proto);
    }
  }
  
  if (validPrototypes.empty()) {
    return prototypes.front();
  }
  
  double totalWeight = 0.0;
  for (const auto* proto : validPrototypes) {
    totalWeight += proto->getWeight();
  }
  
  double random = randDouble() * totalWeight;
  for (const auto* proto : validPrototypes) {
    random -= proto->getWeight();
    if (random <= 0.0) {
      return proto;
    }
  }
  
  return validPrototypes.back();
}

std::shared_ptr<Snake> MonsterFactory::createRandomMonster(
    int x, int y, int gameLevel, double difficultyFactor) const {
  const auto& registry = PrototypeRegistry::getInstance();
  const auto prototypes = registry.getMonsterPrototypesByCategory("normal");
  
  const MonsterPrototype* proto = selectRandomPrototype(prototypes, gameLevel);
  if (!proto) {
    return nullptr;
  }
  
  SpawnContext context;
  context.x = x;
  context.y = y;
  context.levelScaling = difficultyFactor;
  context.difficultyFactor = difficultyFactor;
  context.direction = randInt(0, 1) ? Direction::Right : Direction::Left;
  
  auto snake = createSnakeFromPrototype(*proto, context);
  
  const int length = randInt(2, 4);
  for (int i = 0; i < length; ++i) {
    int xx = x;
    int yy = y;
    if (context.direction == Direction::Down) {
      yy += i;
    } else {
      xx += i;
    }
    
    SpawnContext spriteContext = context;
    spriteContext.x = xx * UNIT + UNIT / 2;
    spriteContext.y = yy * UNIT + UNIT - 3;
    
    auto sprite = createSpriteFromPrototype(*proto, spriteContext);
    snake->sprites().push_front(sprite);
    snake->incrementNum();
    
    hasEnemy[xx][yy] = true;
  }
  
  return snake;
}

std::shared_ptr<Snake> MonsterFactory::createRandomBoss(
    int x, int y, int gameLevel, double difficultyFactor) const {
  const auto& registry = PrototypeRegistry::getInstance();
  const auto prototypes = registry.getMonsterPrototypesByCategory("boss");
  
  const MonsterPrototype* proto = selectRandomPrototype(prototypes, gameLevel);
  if (!proto) {
    return nullptr;
  }
  
  SpawnContext context;
  context.x = x;
  context.y = y;
  context.levelScaling = difficultyFactor * 1.5;
  context.difficultyFactor = difficultyFactor;
  context.direction = Direction::Right;
  
  auto snake = createSnakeFromPrototype(*proto, context);
  
  SpawnContext spriteContext = context;
  spriteContext.x = x * UNIT + UNIT / 2;
  spriteContext.y = y * UNIT + UNIT - 3;
  
  auto sprite = createSpriteFromPrototype(*proto, spriteContext);
  snake->sprites().push_front(sprite);
  snake->incrementNum();
  
  hasEnemy[x][y] = true;
  
  return snake;
}

// ============================================================================
// WeaponFactory Implementation
// ============================================================================

std::unique_ptr<Weapon> WeaponFactory::createWeapon(const WeaponPrototype& proto) const {
  auto weapon = std::make_unique<Weapon>();
  
  weapon->setType(proto.getWeaponType());
  weapon->setShootRange(proto.getShootRange());
  weapon->setEffectRange(proto.getEffectRange());
  weapon->setDamage(proto.getBaseDamage());
  weapon->setGap(proto.getGap());
  weapon->setBulletSpeed(proto.getBulletSpeed());
  weapon->setBirthAudio(proto.getBirthAudio());
  weapon->setDeathAudio(proto.getDeathAudio());
  
  auto& effects = weapon->effects();
  for (int i = 0; i < BUFF_END; ++i) {
    effects[i].chance = proto.getBuffChances()[i];
    effects[i].duration = proto.getBuffDurations()[i];
  }
  
  weapon->setBehavior(makeWeaponBehavior(proto.getWeaponType()));
  
  return weapon;
}

std::unique_ptr<Weapon> WeaponFactory::createWeaponFromId(int weaponId) const {
  if (weaponId >= 0 && weaponId < WEAPONS_SIZE) {
    return std::make_unique<Weapon>(weapons[weaponId]);
  }
  return nullptr;
}

const WeaponPrototype* WeaponFactory::selectRandomPrototype(int gameLevel) const {
  const auto& registry = PrototypeRegistry::getInstance();
  const auto prototypes = registry.getAllWeaponPrototypes();
  
  if (prototypes.empty()) {
    return nullptr;
  }
  
  std::vector<const WeaponPrototype*> validPrototypes;
  for (const auto* proto : prototypes) {
    if (proto && gameLevel >= proto->getMinLevel() && gameLevel <= proto->getMaxLevel()) {
      validPrototypes.push_back(proto);
    }
  }
  
  if (validPrototypes.empty()) {
    return prototypes.front();
  }
  
  double totalWeight = 0.0;
  for (const auto* proto : validPrototypes) {
    totalWeight += proto->getWeight();
  }
  
  double random = randDouble() * totalWeight;
  for (const auto* proto : validPrototypes) {
    random -= proto->getWeight();
    if (random <= 0.0) {
      return proto;
    }
  }
  
  return validPrototypes.back();
}

std::unique_ptr<Weapon> WeaponFactory::createRandomWeapon(int gameLevel) const {
  const WeaponPrototype* proto = selectRandomPrototype(gameLevel);
  if (!proto) {
    return nullptr;
  }
  return createWeapon(*proto);
}

std::unique_ptr<Weapon> WeaponFactory::createWeaponForSprite(int spriteId) const {
  const auto& registry = PrototypeRegistry::getInstance();
  const auto prototypes = registry.getAllWeaponPrototypes();
  
  for (const auto* proto : prototypes) {
    if (proto && proto->getBelongSpriteId() == spriteId) {
      return createWeapon(*proto);
    }
  }
  
  return nullptr;
}

// ============================================================================
// ItemFactory Implementation
// ============================================================================

Item ItemFactory::createItem(const ItemPrototype& proto, int x, int y) const {
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_MAP_ITEMS_ID], &textures[proto.getTextureId()],
      nullptr, LoopType::Infinite, 3, x * UNIT, y * UNIT, SDL_FLIP_NONE, 0,
      At::BottomLeft);
  
  return Item(proto.getItemType(), proto.getItemId(), proto.getBelongSpriteId(),
              animation);
}

Item ItemFactory::createHpMedicine(int x, int y, bool extra) const {
  const int textureId = extra ? RES_FLASK_BIG_YELLOW : RES_FLASK_BIG_RED;
  
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_MAP_ITEMS_ID], &textures[textureId], nullptr,
      LoopType::Infinite, 3, x * UNIT, y * UNIT, SDL_FLIP_NONE, 0, At::BottomLeft);
  
  return Item(extra ? ItemType::HpExtraMedicine : ItemType::HpMedicine, 0, 0,
              animation);
}

Item ItemFactory::createWeaponItem(int x, int y, int gameLevel) const {
  const auto& registry = PrototypeRegistry::getInstance();
  const WeaponPrototype* weaponProto = nullptr;
  
  const auto prototypes = registry.getAllWeaponPrototypes();
  std::vector<const WeaponPrototype*> validPrototypes;
  
  for (const auto* proto : prototypes) {
    if (proto && gameLevel >= proto->getMinLevel() && gameLevel <= proto->getMaxLevel()) {
      validPrototypes.push_back(proto);
    }
  }
  
  if (!validPrototypes.empty()) {
    double totalWeight = 0.0;
    for (const auto* proto : validPrototypes) {
      totalWeight += proto->getWeight();
    }
    
    double random = randDouble() * totalWeight;
    for (const auto* proto : validPrototypes) {
      random -= proto->getWeight();
      if (random <= 0.0) {
        weaponProto = proto;
        break;
      }
    }
    
    if (!weaponProto) {
      weaponProto = validPrototypes.back();
    }
  }
  
  if (!weaponProto) {
    weaponProto = registry.getWeaponPrototype("weapon_ice_sword");
  }
  
  if (!weaponProto) {
    auto animation = createAndPushAnimation(
        animationsList[RENDER_LIST_MAP_ITEMS_ID], &textures[RES_ICE_SWORD],
        nullptr, LoopType::Infinite, 3, x * UNIT, y * UNIT, SDL_FLIP_NONE, 0,
        At::BottomLeft);
    return Item(ItemType::Weapon, WEAPON_ICE_SWORD, SPRITE_KNIGHT, animation);
  }
  
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_MAP_ITEMS_ID], &textures[weaponProto->getTextureId()],
      nullptr, LoopType::Infinite, 3, x * UNIT, y * UNIT, SDL_FLIP_NONE, 0,
      At::BottomLeft);
  
  return Item(ItemType::Weapon, weaponProto->getWeaponId(),
              weaponProto->getBelongSpriteId(), animation);
}

Item ItemFactory::createHeroItem(int x, int y) const {
  const int heroId = randInt(SPRITE_KNIGHT, SPRITE_LIZARD);
  
  auto modelAnimation = commonSprites[heroId].animation();
  if (!modelAnimation) {
    auto animation = createAndPushAnimation(
        animationsList[RENDER_LIST_MAP_ITEMS_ID], &textures[RES_KNIGHT_M], nullptr,
        LoopType::Infinite, 3, x * UNIT + UNIT / 2, y * UNIT + UNIT - 3,
        SDL_FLIP_NONE, 0, At::BottomCenter);
    return Item(ItemType::Hero, heroId, 0, animation);
  }
  
  auto animation = std::make_shared<Animation>(*modelAnimation);
  animation->setAt(At::BottomCenter);
  animation->setPosition(x * UNIT + UNIT / 2, y * UNIT + UNIT - 3);
  pushAnimationToRender(RENDER_LIST_SPRITE_ID, animation);
  
  return Item(ItemType::Hero, heroId, 0, animation);
}

const ItemPrototype* ItemFactory::selectRandomPrototype(ItemType type, int gameLevel) const {
  const auto& registry = PrototypeRegistry::getInstance();
  const auto prototypes = registry.getAllItemPrototypes();
  
  std::vector<const ItemPrototype*> validPrototypes;
  for (const auto* proto : prototypes) {
    if (proto && proto->getItemType() == type &&
        gameLevel >= proto->getMinLevel() && gameLevel <= proto->getMaxLevel()) {
      validPrototypes.push_back(proto);
    }
  }
  
  if (validPrototypes.empty()) {
    return nullptr;
  }
  
  double totalWeight = 0.0;
  for (const auto* proto : validPrototypes) {
    totalWeight += proto->getWeight();
  }
  
  double random = randDouble() * totalWeight;
  for (const auto* proto : validPrototypes) {
    random -= proto->getWeight();
    if (random <= 0.0) {
      return proto;
    }
  }
  
  return validPrototypes.back();
}

Item ItemFactory::createRandomItem(int x, int y, int gameLevel) const {
  const double random = randDouble();
  
  if (random < 0.3) {
    return createHpMedicine(x, y, false);
  } else if (random < 0.5) {
    return createHpMedicine(x, y, true);
  } else {
    return createWeaponItem(x, y, gameLevel);
  }
}

// ============================================================================
// EntitySpawner Implementation
// ============================================================================

EntitySpawner::EntitySpawner()
    : monsterFactory_(std::make_unique<MonsterFactory>()),
      weaponFactory_(std::make_unique<WeaponFactory>()),
      itemFactory_(std::make_unique<ItemFactory>()) {}

void EntitySpawner::initializeLevel(int level, int stage) {
  currentLevel_ = level;
  currentStage_ = stage;
  
  config_.hpScaling = 1 + level * 0.8 + stage * level * 0.1;
  config_.damageScaling = 1 + level * 0.5 + stage * level * 0.05;
  config_.dropRateScaling = 1 + level * 0.8 + stage * level * 0.02;
  config_.monsterGenFactor = 1 + level * 0.5 + stage * level * 0.05;
  
  config_.yellowFlaskDropRate = 0.3;
  config_.weaponDropRate = 0.7;
  config_.luckyFactor = 1.0;
  
  if (level == 0) {
    config_.heroCount = 8;
    config_.flaskCount = 6;
    config_.enemyCount = 25;
    config_.bossCount = 2;
  } else if (level == 1) {
    config_.weaponDropRate = 0.98;
    config_.heroCount = 5;
    config_.flaskCount = 4;
    config_.enemyCount = 28;
    config_.bossCount = 3;
  } else if (level == 2) {
    config_.weaponDropRate = 0.98;
    config_.yellowFlaskDropRate = 0.3;
    config_.heroCount = 5;
    config_.flaskCount = 3;
    config_.enemyCount = 28;
    config_.bossCount = 5;
  }
  
  config_.enemyCount += stage / 2 * (level + 1);
  config_.bossCount += stage / 3;
}

std::shared_ptr<Snake> EntitySpawner::spawnMonster(int x, int y) {
  return monsterFactory_->createRandomMonster(x, y, currentLevel_,
                                               config_.hpScaling);
}

std::shared_ptr<Snake> EntitySpawner::spawnBoss(int x, int y) {
  return monsterFactory_->createRandomBoss(x, y, currentLevel_, config_.hpScaling);
}

Item EntitySpawner::spawnHeroItem(int x, int y) {
  return itemFactory_->createHeroItem(x, y);
}

Item EntitySpawner::spawnFlaskItem(int x, int y, bool extra) {
  return itemFactory_->createHpMedicine(x, y, extra);
}

Item EntitySpawner::spawnWeaponItem(int x, int y) {
  return itemFactory_->createWeaponItem(x, y, currentLevel_);
}

// ============================================================================
// Prototype Initialization
// ============================================================================

static void registerMonsterPrototypes(PrototypeRegistry& registry) {
  struct MonsterDef {
    const char* id;
    const char* category;
    int spriteId;
    int baseHp;
    int moveStep;
    int weaponId;
    double dropRate;
    double weight;
    int minLevel;
    int maxLevel;
  };
  
  const MonsterDef monsters[] = {
      // Normal monsters - Tier 1 (weak)
      {"monster_tiny_zombie", "normal", SPRITE_TINY_ZOMBIE, 80, 1,
       WEAPON_MONSTER_CLAW, 0.8, 1.0, 0, 999},
      {"monster_goblin", "normal", SPRITE_GOBLIN, 90, 1, WEAPON_MONSTER_CLAW,
       0.9, 1.0, 0, 999},
      {"monster_imp", "normal", SPRITE_IMP, 85, 1, WEAPON_MONSTER_CLAW, 0.85,
       1.0, 0, 999},
      {"monster_skelet", "normal", SPRITE_SKELET, 95, 1, WEAPON_MONSTER_CLAW2,
       0.9, 1.0, 0, 999},
      
      // Normal monsters - Tier 2 (medium)
      {"monster_zombie", "normal", SPRITE_ZOMBIE, 120, 1, WEAPON_MONSTER_CLAW,
       1.0, 0.8, 0, 999},
      {"monster_ice_zombie", "normal", SPRITE_ICE_ZOMBIE, 130, 1,
       WEAPON_MONSTER_CLAW, 1.1, 0.7, 1, 999},
      {"monster_muddy", "normal", SPRITE_MUDDY, 110, 1, WEAPON_MONSTER_CLAW,
       0.95, 0.9, 0, 999},
      {"monster_swampy", "normal", SPRITE_SWAMPY, 115, 1, WEAPON_MONSTER_CLAW,
       1.0, 0.85, 0, 999},
      
      // Normal monsters - Tier 3 (strong)
      {"monster_masked_orc", "normal", SPRITE_MASKED_ORC, 150, 1,
       WEAPON_MONSTER_CLAW2, 1.2, 0.6, 0, 999},
      {"monster_orc_warrior", "normal", SPRITE_ORC_WARRIOR, 160, 1,
       WEAPON_MONSTER_CLAW2, 1.3, 0.5, 1, 999},
      {"monster_orc_shaman", "normal", SPRITE_ORC_SHAMAN, 140, 1,
       WEAPON_FIREBALL, 1.1, 0.5, 1, 999},
      {"monster_necromancer", "normal", SPRITE_NECROMANCER, 145, 1,
       WEAPON_PURPLE_BALL, 1.2, 0.4, 1, 999},
      
      // Fast monsters
      {"monster_wogol", "normal", SPRITE_WOGOL, 100, 2, WEAPON_MONSTER_CLAW,
       0.9, 0.6, 0, 999},
      {"monster_chort", "normal", SPRITE_CHROT, 110, 2, WEAPON_MONSTER_CLAW,
       0.95, 0.5, 0, 999},
      
      // Boss monsters
      {"boss_big_zombie", "boss", SPRITE_BIG_ZOMBIE, 300, 1,
       WEAPON_MONSTER_CLAW2, 2.0, 0.33, 0, 999},
      {"boss_ogre", "boss", SPRITE_ORGRE, 350, 1, WEAPON_THROW_AXE, 2.2, 0.33,
       0, 999},
      {"boss_big_demon", "boss", SPRITE_BIG_DEMON, 400, 1, WEAPON_FIREBALL,
       2.5, 0.34, 0, 999},
  };
  
  for (const auto& def : monsters) {
    auto proto = std::make_unique<MonsterPrototype>();
    proto->setPrototypeId(def.id);
    proto->setCategory(def.category);
    proto->setSpriteId(def.spriteId);
    proto->setBaseHp(def.baseHp);
    proto->setMoveStep(def.moveStep);
    proto->setWeaponId(def.weaponId);
    proto->setDropRate(def.dropRate);
    proto->setWeight(def.weight);
    proto->setMinLevel(def.minLevel);
    proto->setMaxLevel(def.maxLevel);
    proto->setIsBoss(std::string(def.category) == "boss");
    
    registry.registerPrototype(def.id, std::move(proto));
  }
}

static void registerWeaponPrototypes(PrototypeRegistry& registry) {
  struct WeaponDef {
    const char* id;
    const char* category;
    int weaponId;
    WeaponType type;
    int shootRange;
    int effectRange;
    int damage;
    int gap;
    int bulletSpeed;
    int textureId;
    int belongSpriteId;
    int birthAudio;
    int deathAudio;
    double weight;
    int minLevel;
    int maxLevel;
  };
  
  const WeaponDef weapons[] = {
      // Knight weapons
      {"weapon_ice_sword", "knight", WEAPON_ICE_SWORD, WeaponType::SwordRange,
       64, 48, 25, 25, 0, RES_ICE_SWORD, SPRITE_KNIGHT, AUDIO_SWORD_HIT,
       AUDIO_SWORD_HIT, 1.0, 0, 999},
      {"weapon_holy_sword", "knight", WEAPON_HOLY_SWORD, WeaponType::SwordRange,
       64, 48, 30, 30, 0, RES_HOLY_SWORD, SPRITE_KNIGHT, AUDIO_SWORD_HIT,
       AUDIO_SWORD_HIT, 0.9, 0, 999},
      {"weapon_fire_sword", "knight", WEAPON_FIRE_SWORD, WeaponType::SwordRange,
       64, 48, 28, 28, 0, RES_FIRE_SWORD, SPRITE_KNIGHT, AUDIO_SWORD_HIT,
       AUDIO_FIREBALL_EXP, 0.8, 1, 999},
      
      // Wizard weapons
      {"weapon_thunder_staff", "wizard", WEAPON_THUNDER_STAFF,
       WeaponType::GunPoint, 256, 64, 20, 40, 8, RES_THUNDER_STAFF,
       SPRITE_WIZZARD, AUDIO_THUNDER, AUDIO_THUNDER, 1.0, 0, 999},
      {"weapon_purple_staff", "wizard", WEAPON_PURPLE_STAFF,
       WeaponType::GunPoint, 192, 48, 22, 35, 6, RES_PURPLE_STAFF,
       SPRITE_WIZZARD, AUDIO_LIGHT_SHOOT, AUDIO_FIREBALL_EXP, 0.9, 0, 999},
      
      // Lizard weapons
      {"weapon_solid_claw", "lizard", WEAPON_SOLID_CLAW, WeaponType::SwordRange,
       56, 40, 24, 22, 0, RES_GRASS_SWORD, SPRITE_LIZARD, AUDIO_CLAW_HIT,
       AUDIO_CLAW_HIT, 1.0, 0, 999},
      
      // Elf weapons
      {"weapon_powerful_bow", "elf", WEAPON_POWERFUL_BOW, WeaponType::GunPoint,
       320, 32, 35, 45, 12, RES_POWERFUL_BOW, SPRITE_ELF, AUDIO_SHOOT,
       AUDIO_ARROW_HIT, 1.0, 0, 999},
  };
  
  for (const auto& def : weapons) {
    auto proto = std::make_unique<WeaponPrototype>();
    proto->setPrototypeId(def.id);
    proto->setCategory(def.category);
    proto->setWeaponId(def.weaponId);
    proto->setWeaponType(def.type);
    proto->setShootRange(def.shootRange);
    proto->setEffectRange(def.effectRange);
    proto->setBaseDamage(def.damage);
    proto->setGap(def.gap);
    proto->setBulletSpeed(def.bulletSpeed);
    proto->setTextureId(def.textureId);
    proto->setBelongSpriteId(def.belongSpriteId);
    proto->setBirthAudio(def.birthAudio);
    proto->setDeathAudio(def.deathAudio);
    proto->setWeight(def.weight);
    proto->setMinLevel(def.minLevel);
    proto->setMaxLevel(def.maxLevel);
    
    registry.registerPrototype(def.id, std::move(proto));
  }
}

static void registerItemPrototypes(PrototypeRegistry& registry) {
  struct ItemDef {
    const char* id;
    const char* category;
    ItemType type;
    int itemId;
    int textureId;
    int belongSpriteId;
    int hpRestorePercent;
    bool isExtra;
    double weight;
    int minLevel;
    int maxLevel;
  };
  
  const ItemDef items[] = {
      {"item_hp_red", "consumable", ItemType::HpMedicine, 0, RES_FLASK_BIG_RED,
       0, 55, false, 1.0, 0, 999},
      {"item_hp_yellow", "consumable", ItemType::HpExtraMedicine, 0,
       RES_FLASK_BIG_YELLOW, 0, 33, true, 0.5, 0, 999},
  };
  
  for (const auto& def : items) {
    auto proto = std::make_unique<ItemPrototype>();
    proto->setPrototypeId(def.id);
    proto->setCategory(def.category);
    proto->setItemType(def.type);
    proto->setItemId(def.itemId);
    proto->setTextureId(def.textureId);
    proto->setBelongSpriteId(def.belongSpriteId);
    proto->setHpRestorePercent(def.hpRestorePercent);
    proto->setIsExtra(def.isExtra);
    proto->setWeight(def.weight);
    proto->setMinLevel(def.minLevel);
    proto->setMaxLevel(def.maxLevel);
    
    registry.registerPrototype(def.id, std::move(proto));
  }
}

void initializeEntityPrototypes() {
  PrototypeRegistry& registry = PrototypeRegistry::getInstance();
  registry.clear();
  
  registerMonsterPrototypes(registry);
  registerWeaponPrototypes(registry);
  registerItemPrototypes(registry);
}
