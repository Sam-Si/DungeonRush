#ifndef SNAKE_COMPONENT_H_
#define SNAKE_COMPONENT_H_

#include "adt.h"
#include "types.h"

#include <array>
#include <memory>

class Weapon;
class Animation;
class AIBehavior;
class PositionBuffer;

// ============================================================================
// TransformComponent - Handles position, direction, and movement state
// ============================================================================
class TransformComponent {
 public:
  TransformComponent() = default;
  TransformComponent(int x, int y, Direction direction);
  TransformComponent(const TransformComponent&) = default;
  TransformComponent& operator=(const TransformComponent&) = default;
  TransformComponent(TransformComponent&&) noexcept = default;
  TransformComponent& operator=(TransformComponent&&) noexcept = default;
  ~TransformComponent() = default;

  int x() const noexcept;
  int y() const noexcept;
  Direction face() const noexcept;
  Direction direction() const noexcept;

  void setPosition(int x, int y) noexcept;
  void setX(int x) noexcept;
  void setY(int y) noexcept;
  void setFace(Direction face) noexcept;
  void setDirection(Direction direction) noexcept;

  void enqueueDirectionChange(Direction newDirection,
                              PositionBuffer& buffer) noexcept;

 private:
  int x_ = 0;
  int y_ = 0;
  Direction face_ = Direction::Right;
  Direction direction_ = Direction::Right;
};

// ============================================================================
// HealthComponent - Handles HP, damage, and health state
// ============================================================================
class HealthComponent {
 public:
  HealthComponent() = default;
  HealthComponent(int hp, int totalHp);
  HealthComponent(const HealthComponent&) = default;
  HealthComponent& operator=(const HealthComponent&) = default;
  HealthComponent(HealthComponent&&) noexcept = default;
  HealthComponent& operator=(HealthComponent&&) noexcept = default;
  ~HealthComponent() = default;

  int hp() const noexcept;
  int totalHp() const noexcept;
  bool isDead() const noexcept;
  bool isAlive() const noexcept;

  void setHp(int hp) noexcept;
  void setTotalHp(int totalHp) noexcept;
  void takeDamage(int damage) noexcept;
  void heal(int amount) noexcept;
  void reset() noexcept;

 private:
  int hp_ = 0;
  int totalHp_ = 0;
};

// ============================================================================
// RenderComponent - Handles animation and visual representation
// ============================================================================
class RenderComponent {
 public:
  RenderComponent() = default;
  explicit RenderComponent(const std::shared_ptr<Animation>& animation);
  RenderComponent(const RenderComponent&) = default;
  RenderComponent& operator=(const RenderComponent&) = default;
  RenderComponent(RenderComponent&&) noexcept = default;
  RenderComponent& operator=(RenderComponent&&) noexcept = default;
  ~RenderComponent() = default;

  std::shared_ptr<Animation> animation() const noexcept;
  bool hasAnimation() const noexcept;

  void setAnimation(const std::shared_ptr<Animation>& animation) noexcept;
  void clearAnimation() noexcept;
  void updatePosition(int x, int y) noexcept;

 private:
  std::shared_ptr<Animation> animation_{};
};

// ============================================================================
// CombatComponent - Handles weapon and attack state
// ============================================================================
class CombatComponent {
 public:
  CombatComponent() = default;
  explicit CombatComponent(Weapon* weapon);
  CombatComponent(const CombatComponent&) = default;
  CombatComponent& operator=(const CombatComponent&) = default;
  CombatComponent(CombatComponent&&) noexcept = default;
  CombatComponent& operator=(CombatComponent&&) noexcept = default;
  ~CombatComponent() = default;

  Weapon* weapon() const noexcept;
  bool hasWeapon() const noexcept;
  int lastAttack() const noexcept;
  double dropRate() const noexcept;

  void setWeapon(Weapon* weapon) noexcept;
  void setLastAttack(int lastAttack) noexcept;
  void setDropRate(double dropRate) noexcept;
  void recordAttack() noexcept;

 private:
  Weapon* weapon_ = nullptr;
  int lastAttack_ = 0;
  double dropRate_ = 0.0;
};

// ============================================================================
// BuffComponent - Handles buff/debuff state
// ============================================================================
class BuffComponent {
 public:
  BuffComponent() = default;
  BuffComponent(const BuffComponent&) = default;
  BuffComponent& operator=(const BuffComponent&) = default;
  BuffComponent(BuffComponent&&) noexcept = default;
  BuffComponent& operator=(BuffComponent&&) noexcept = default;
  ~BuffComponent() = default;

  const std::array<int, BUFF_END>& buffs() const noexcept;
  std::array<int, BUFF_END>& buffs() noexcept;
  int buff(int index) const noexcept;

  void setBuff(int index, int value) noexcept;
  void decrementBuff(int index) noexcept;
  void clearBuffs() noexcept;
  bool isFrozen() const noexcept;
  bool isSlowed() const noexcept;
  bool hasDefense() const noexcept;
  bool hasAttackUp() const noexcept;

 private:
  std::array<int, BUFF_END> buffs_{};
};

// ============================================================================
// AIComponent - Handles AI behavior strategy
// ============================================================================
class AIComponent {
 public:
  AIComponent() = default;
  explicit AIComponent(std::shared_ptr<AIBehavior> behavior);
  AIComponent(const AIComponent&) = default;
  AIComponent& operator=(const AIComponent&) = default;
  AIComponent(AIComponent&&) noexcept = default;
  AIComponent& operator=(AIComponent&&) noexcept = default;
  ~AIComponent() = default;

  AIBehavior* behavior() const noexcept;
  bool hasBehavior() const noexcept;

  void setBehavior(std::shared_ptr<AIBehavior> behavior) noexcept;
  void clearBehavior() noexcept;

 private:
  std::shared_ptr<AIBehavior> behavior_{};
};

// ============================================================================
// ScoreComponent - Handles scoring state
// ============================================================================
class ScoreComponent {
 public:
  ScoreComponent() = default;
  ScoreComponent(const ScoreComponent&) = default;
  ScoreComponent& operator=(const ScoreComponent&) = default;
  ScoreComponent(ScoreComponent&&) noexcept = default;
  ScoreComponent& operator=(ScoreComponent&&) noexcept = default;
  ~ScoreComponent() = default;

  const std::shared_ptr<Score>& score() const noexcept;
  std::shared_ptr<Score>& score() noexcept;

  void setScore(const std::shared_ptr<Score>& score) noexcept;
  void reset() noexcept;

 private:
  std::shared_ptr<Score> score_{};
};

#endif