#ifndef SNAKE_GAME_H_
#define SNAKE_GAME_H_

#include "adt.h"
#include "player.h"
#include "sprite.h"
#include "types.h"
#include "map.h"
#include "render.h"
#include "core/game_state.h"

#include <memory>
#include <vector>
#include <array>
#include <functional>

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
#define GAME_FROZEN_DAMAGE_K 1.5
// Drop Rate
// Win

enum class GameEventType { PlayerDied, ItemPicked, SoundRequested };

struct GameEvent {
  GameEventType type = GameEventType::SoundRequested;
  int playerId = -1;
  ItemType itemType = ItemType::None;
  int audioId = -1;
};

class EventBus {
 public:
  using Listener = std::function<void(const GameEvent&)>;

  void subscribe(Listener listener);
  void emit(const GameEvent& event) const;
  void clear();

 private:
  std::vector<Listener> listeners_{};
};

// Forward declarations
class AIBehavior;
class GameLoopManager;
class MapManager;
class WeaponBehavior;
class Weapon;
struct GameContext;
struct GameEvent;
class EventBus;

// Global extern declarations (to be removed after full refactoring)
extern const int SCALE_FACTOR;
extern const int n, m;
extern Texture textures[];
extern Effect effects[];
extern Weapon weapons[];
extern Sprite commonSprites[];
extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;

#include "entities/entity_manager.h"
#include "systems/collision_manager.h"
#include "systems/item_manager.h"
#include "systems/buff_manager.h"

// Typedefs for backward compatibility
using EntityManager = snake::entities::EntityManager;
using CollisionManager = snake::systems::CollisionManager;
using ItemManager = snake::systems::ItemManager;
using BuffManager = snake::systems::BuffManager;

// ============================================================================
// GameLoopManager - Handles the main game loop logic
// ============================================================================
class GameLoopManager {
 public:
  GameLoopManager() = default;
  GameLoopManager(const GameLoopManager&) = delete;
  GameLoopManager& operator=(const GameLoopManager&) = delete;
  GameLoopManager(GameLoopManager&&) = default;
  GameLoopManager& operator=(GameLoopManager&&) = default;
  ~GameLoopManager() = default;

  // Main game loop
  int run(GameContext& ctx);

  // Input handling
  bool handleLocalKeypress(GameContext& ctx);
  void handleLanKeypress(GameContext& ctx);

  // Game state updates
  void updateMap(GameContext& ctx);
  void updateBuffs(GameContext& ctx);
  void moveEntities(GameContext& ctx);
  void processAttacks(GameContext& ctx);
  void processCollisions(GameContext& ctx);
  void processBullets(GameContext& ctx);
  void cleanupDeadEntities(GameContext& ctx);
  void checkWinCondition(GameContext& ctx);

  // Game state checks
  bool isWin(const GameContext& ctx) const;

 private:
  void pauseGame();
  void setTerm(GameContext& ctx, int status);
};

// ============================================================================
// GameContext - Central context holding all game state
// ============================================================================
struct GameContext {
  // Core state container - all global state lives here
  snake::core::GameState gameState;

  // Managers
  snake::entities::EntityManager entityManager;
  snake::systems::CollisionManager collisionManager;
  snake::systems::ItemManager itemManager;
  snake::systems::BuffManager buffManager;
  std::shared_ptr<AIBehavior> aiBehavior;
  EventBus eventBus;

  // Map state
  std::array<std::array<Block, MAP_SIZE>, MAP_SIZE> map{};
  std::array<std::array<bool, MAP_SIZE>, MAP_SIZE> hasMap{};
  std::array<std::array<bool, MAP_SIZE>, MAP_SIZE> hasEnemy{};

  // Game configuration (convenience accessors for legacy globals)
  int gameLevel() const;
  int& gameLevel();
  int stage() const;
  int& stage();
  int status() const;
  int& status();
  bool willTerm() const;
  bool& willTerm();
  int termCount() const;
  int& termCount();
  int winNum() const;
  int& winNum();

  int herosSetting = 8;
  int flasksSetting = 6;
  int spritesSetting = 25;
  int bossSetting = 2;
  int spikeDamage = 1;

  double gameLucky = 1.0;
  double dropoutYellowFlasks = 0.3;
  double dropoutWeapons = 0.7;
  double trapRate = 0.005;
  double monstersHpAdjust = 1.0;
  double monstersWeaponBuffAdjust = 1.0;
  double monstersGenFactor = 1.0;

  GameContext() = default;
  GameContext(const GameContext&) = delete;
  GameContext& operator=(const GameContext&) = delete;
  GameContext(GameContext&&) = default;
  GameContext& operator=(GameContext&&) = default;
  ~GameContext() = default;

  // Access the centralized game state
  snake::core::GameState& state() { return gameState; }
  const snake::core::GameState& state() const { return gameState; }

  void reset();
  void setLevel(int level);
  void initEnemies();
};

// ============================================================================
// Game Functions - High level game operations
// ============================================================================

void pushMapToRender();
std::vector<std::shared_ptr<Score>> startGame(int localPlayers,
                                              int remotePlayers,
                                              bool localFirst);
void initGame(int localPlayers, int remotePlayers, bool localFirst);
void destroyGame(int status);
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

// Get the global game context (temporary during refactoring)
GameContext& getGameContext();
void initializeEventObservers();

#endif
