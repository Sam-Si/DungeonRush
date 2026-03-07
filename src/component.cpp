#include "component.h"

#include "sprite.h"

// ============================================================================
// TransformComponent Implementation
// ============================================================================

TransformComponent::TransformComponent(int x, int y, Direction direction)
    : x_(x), y_(y), direction_(direction), face_(direction) {}

int TransformComponent::x() const noexcept { return x_; }
int TransformComponent::y() const noexcept { return y_; }
Direction TransformComponent::face() const noexcept { return face_; }
Direction TransformComponent::direction() const noexcept { return direction_; }

void TransformComponent::setPosition(int x, int y) noexcept {
  x_ = x;
  y_ = y;
}

void TransformComponent::setX(int x) noexcept { x_ = x; }
void TransformComponent::setY(int y) noexcept { y_ = y; }

void TransformComponent::setFace(Direction face) noexcept { face_ = face; }

void TransformComponent::setDirection(Direction direction) noexcept {
  direction_ = direction;
}

void TransformComponent::enqueueDirectionChange(
    Direction newDirection, PositionBuffer& buffer) noexcept {
  if (direction_ == newDirection) {
    return;
  }
  direction_ = newDirection;
  if (newDirection == Direction::Left || newDirection == Direction::Right) {
    face_ = newDirection;
  }
  // Buffer position change for chained sprites
  PositionBufferSlot slot{x_, y_, direction_};
  buffer.push(slot);
}

// ============================================================================
// HealthComponent Implementation
// ============================================================================

HealthComponent::HealthComponent(int hp, int totalHp)
    : hp_(hp), totalHp_(totalHp) {}

int HealthComponent::hp() const noexcept { return hp_; }
int HealthComponent::totalHp() const noexcept { return totalHp_; }
bool HealthComponent::isDead() const noexcept { return hp_ <= 0; }
bool HealthComponent::isAlive() const noexcept { return hp_ > 0; }

void HealthComponent::setHp(int hp) noexcept { hp_ = hp; }
void HealthComponent::setTotalHp(int totalHp) noexcept { totalHp_ = totalHp; }

void HealthComponent::takeDamage(int damage) noexcept {
  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }
}

void HealthComponent::heal(int amount) noexcept {
  hp_ += amount;
  if (hp_ > totalHp_) {
    hp_ = totalHp_;
  }
}

void HealthComponent::reset() noexcept { hp_ = totalHp_; }

// ============================================================================
// RenderComponent Implementation
// ============================================================================

RenderComponent::RenderComponent(
    const std::shared_ptr<Animation>& animation)
    : animation_(animation) {}

std::shared_ptr<Animation> RenderComponent::animation() const noexcept {
  return animation_;
}

bool RenderComponent::hasAnimation() const noexcept {
  return animation_ != nullptr;
}

void RenderComponent::setAnimation(
    const std::shared_ptr<Animation>& animation) noexcept {
  animation_ = animation;
}

void RenderComponent::clearAnimation() noexcept { animation_.reset(); }

void RenderComponent::updatePosition(int x, int y) noexcept {
  if (animation_) {
    animation_->setPosition(x, y);
  }
}

// ============================================================================
// CombatComponent Implementation
// ============================================================================

CombatComponent::CombatComponent(Weapon* weapon) : weapon_(weapon) {}

Weapon* CombatComponent::weapon() const noexcept { return weapon_; }
bool CombatComponent::hasWeapon() const noexcept { return weapon_ != nullptr; }
int CombatComponent::lastAttack() const noexcept { return lastAttack_; }
double CombatComponent::dropRate() const noexcept { return dropRate_; }

void CombatComponent::setWeapon(Weapon* weapon) noexcept { weapon_ = weapon; }
void CombatComponent::setLastAttack(int lastAttack) noexcept {
  lastAttack_ = lastAttack;
}
void CombatComponent::setDropRate(double dropRate) noexcept {
  dropRate_ = dropRate;
}
void CombatComponent::recordAttack() noexcept { ++lastAttack_; }

// ============================================================================
// BuffComponent Implementation
// ============================================================================

const std::array<int, BUFF_END>& BuffComponent::buffs() const noexcept {
  return buffs_;
}

std::array<int, BUFF_END>& BuffComponent::buffs() noexcept { return buffs_; }

int BuffComponent::buff(int index) const noexcept { return buffs_[index]; }

void BuffComponent::setBuff(int index, int value) noexcept {
  buffs_[index] = value;
}

void BuffComponent::decrementBuff(int index) noexcept {
  if (buffs_[index] > 0) {
    --buffs_[index];
  }
}

void BuffComponent::clearBuffs() noexcept { buffs_.fill(0); }

bool BuffComponent::isFrozen() const noexcept {
  return buffs_[BUFF_FROZEN] > 0;
}

bool BuffComponent::isSlowed() const noexcept {
  return buffs_[BUFF_SLOWDOWN] > 0;
}

bool BuffComponent::hasDefense() const noexcept {
  return buffs_[BUFF_DEFFENCE] > 0;
}

bool BuffComponent::hasAttackUp() const noexcept {
  return buffs_[BUFF_ATTACK] > 0;
}

// ============================================================================
// AIComponent Implementation
// ============================================================================

AIComponent::AIComponent(std::shared_ptr<AIBehavior> behavior)
    : behavior_(std::move(behavior)) {}

AIBehavior* AIComponent::behavior() const noexcept { return behavior_.get(); }

bool AIComponent::hasBehavior() const noexcept { return behavior_ != nullptr; }

void AIComponent::setBehavior(
    std::shared_ptr<AIBehavior> behavior) noexcept {
  behavior_ = std::move(behavior);
}

void AIComponent::clearBehavior() noexcept { behavior_.reset(); }

// ============================================================================
// ScoreComponent Implementation
// ============================================================================

const std::shared_ptr<Score>& ScoreComponent::score() const noexcept {
  return score_;
}

std::shared_ptr<Score>& ScoreComponent::score() noexcept { return score_; }

void ScoreComponent::setScore(const std::shared_ptr<Score>& score) noexcept {
  score_ = score;
}

void ScoreComponent::reset() noexcept { score_.reset(); }