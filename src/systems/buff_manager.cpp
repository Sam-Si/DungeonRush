#include "systems/buff_manager.h"

#include "helper.h"
#include "render.h"
#include "res.h"

#include <array>

extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;
extern Texture textures[];
extern Effect effects[];
extern double GAME_MONSTERS_WEAPON_BUFF_ADJUST;

constexpr int GAME_MONSTERS_TEAM = 9;

namespace snake {
namespace systems {

namespace {

class BuffEffectStrategy {
 public:
  virtual ~BuffEffectStrategy() = default;
  virtual void apply(const std::shared_ptr<Snake>& source,
                     const std::shared_ptr<Snake>& target,
                     int duration,
                     BuffManager& manager) const = 0;
};

class FrozenBuffStrategy final : public BuffEffectStrategy {
 public:
  void apply(const std::shared_ptr<Snake>&,
             const std::shared_ptr<Snake>& target,
             int duration,
             BuffManager& manager) const override {
    manager.freezeSnake(target.get(), duration);
  }
};

class SlowdownBuffStrategy final : public BuffEffectStrategy {
 public:
  void apply(const std::shared_ptr<Snake>&,
             const std::shared_ptr<Snake>& target,
             int duration,
             BuffManager& manager) const override {
    manager.slowDownSnake(target.get(), duration);
  }
};

class DefenseBuffStrategy final : public BuffEffectStrategy {
 public:
  void apply(const std::shared_ptr<Snake>& source,
             const std::shared_ptr<Snake>&,
             int duration,
             BuffManager& manager) const override {
    if (source) {
      manager.shieldSnake(source, duration);
    }
  }
};

class AttackBuffStrategy final : public BuffEffectStrategy {
 public:
  void apply(const std::shared_ptr<Snake>& source,
             const std::shared_ptr<Snake>&,
             int duration,
             BuffManager& manager) const override {
    if (source) {
      manager.attackUpSnake(source, duration);
    }
  }
};

const std::array<std::unique_ptr<BuffEffectStrategy>, BUFF_END>&
    getBuffStrategies() {
  static const std::array<std::unique_ptr<BuffEffectStrategy>, BUFF_END>
      kStrategies = {std::make_unique<FrozenBuffStrategy>(),
                     std::make_unique<SlowdownBuffStrategy>(),
                     std::make_unique<DefenseBuffStrategy>(),
                     std::make_unique<AttackBuffStrategy>()};
  return kStrategies;
}

}  // namespace

void BuffManager::freezeSnake(Snake* snake, int duration) {
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

void BuffManager::slowDownSnake(Snake* snake, int duration) {
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

void BuffManager::shieldSnake(const std::shared_ptr<Snake>& snake, int duration) {
  if (!snake) {
    return;
  }
  auto& buffs = snake->buffs();
  if (buffs[BUFF_DEFFENCE]) {
    return;
  }
  buffs[BUFF_DEFFENCE] += duration;
  for (const auto& sprite : snake->sprites()) {
    if (sprite) {
      auto animation = createAndPushAnimation(
          animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_HOLY_SHIELD], nullptr,
          LoopType::Lifespan, 40, sprite->x(), sprite->y(), SDL_FLIP_NONE, 0,
          At::BottomCenter);
      bindAnimationToSprite(animation, sprite, true);
      animation->setLifeSpan(duration);
    }
  }
}

void BuffManager::attackUpSnake(const std::shared_ptr<Snake>& snake, int duration) {
  if (!snake) {
    return;
  }
  auto& buffs = snake->buffs();
  if (buffs[BUFF_ATTACK]) {
    return;
  }
  buffs[BUFF_ATTACK] += duration;
  for (const auto& sprite : snake->sprites()) {
    if (sprite) {
      auto animation = createAndPushAnimation(
          animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_ATTACK_UP], nullptr,
          LoopType::Lifespan, SPRITE_ANIMATION_DURATION, sprite->x(), sprite->y(),
          SDL_FLIP_NONE, 0, At::BottomCenter);
      bindAnimationToSprite(animation, sprite, true);
      animation->setLifeSpan(duration);
    }
  }
}

void BuffManager::updateBuffDurations(entities::EntityManager& entityManager) {
  const int count = entityManager.snakeCount();
  for (int i = 0; i < count; i++) {
    const auto& snake = entityManager.getSnake(i);
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

void BuffManager::invokeWeaponBuff(const std::shared_ptr<Snake>& src,
                                    const Weapon& weapon,
                                    const std::shared_ptr<Snake>& dest,
                                    int) {
  if (!dest) {
    return;
  }
  double random = 0.0;
  const auto& weaponEffects = weapon.effects();
  const auto& strategies = getBuffStrategies();
  
  for (int i = BUFF_BEGIN; i < BUFF_END; i++) {
    random = randDouble();
    if (src && src->team() == GAME_MONSTERS_TEAM) {
      random *= GAME_MONSTERS_WEAPON_BUFF_ADJUST;
    }
    const auto& strategy = strategies[i];
    if (strategy && random < weaponEffects[i].chance) {
      strategy->apply(src, dest, weaponEffects[i].duration, *this);
    }
  }
}

}  // namespace systems
}  // namespace snake
