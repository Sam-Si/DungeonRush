#include "sprite.h"

#include <cassert>

#include "render.h"

PositionBuffer::PositionBuffer() = default;

Sprite::Sprite() = default;

void PositionBuffer::push(PositionBufferSlot slot) {
  assert(size_ < POSITION_BUFFER_SIZE);
  buffer_[size_++] = slot;
}

PositionBufferSlot PositionBuffer::pop() {
  assert(size_ > 0);
  PositionBufferSlot result = buffer_[0];
  for (int i = 0; i < size_ - 1; ++i) {
    buffer_[i] = buffer_[i + 1];
  }
  size_--;
  return result;
}

int PositionBuffer::size() const { return size_; }

const PositionBufferSlot& PositionBuffer::at(int index) const {
  return buffer_.at(index);
}

Sprite::Sprite(const Sprite& model, int x, int y)
    : transform_(x, y, model.transform_.direction()),
      health_(model.health_),
      render_(model.render_.animation()
                  ? std::make_shared<Animation>(*model.render_.animation())
                  : nullptr),
      combat_(model.combat_),
      positionBuffer_() {
  if (render_.hasAnimation()) {
    render_.updatePosition(x, y);
  }
}

// TransformComponent accessors
int Sprite::x() const { return transform_.x(); }
int Sprite::y() const { return transform_.y(); }
Direction Sprite::face() const { return transform_.face(); }
Direction Sprite::direction() const { return transform_.direction(); }

void Sprite::setPosition(int x, int y) {
  transform_.setPosition(x, y);
  render_.updatePosition(x, y);
}

void Sprite::setFace(Direction face) { transform_.setFace(face); }
void Sprite::setDirection(Direction direction) {
  transform_.setDirection(direction);
}

// HealthComponent accessors
int Sprite::hp() const { return health_.hp(); }
int Sprite::totalHp() const { return health_.totalHp(); }
void Sprite::setHp(int hp) { health_.setHp(hp); }
void Sprite::setTotalHp(int totalHp) { health_.setTotalHp(totalHp); }

// RenderComponent accessors
std::shared_ptr<Animation> Sprite::animation() const {
  return render_.animation();
}
void Sprite::setAnimation(const std::shared_ptr<Animation>& animation) {
  render_.setAnimation(animation);
}

// CombatComponent accessors
Weapon* Sprite::weapon() const { return combat_.weapon(); }
int Sprite::lastAttack() const { return combat_.lastAttack(); }
double Sprite::dropRate() const { return combat_.dropRate(); }
void Sprite::setWeapon(Weapon* weapon) { combat_.setWeapon(weapon); }
void Sprite::setLastAttack(int lastAttack) { combat_.setLastAttack(lastAttack); }
void Sprite::setDropRate(double dropRate) { combat_.setDropRate(dropRate); }

// PositionBuffer
PositionBuffer& Sprite::positionBuffer() { return positionBuffer_; }
const PositionBuffer& Sprite::positionBuffer() const { return positionBuffer_; }

void Sprite::enqueueDirectionChange(Direction newDirection,
                                    const std::shared_ptr<Sprite>& next) {
  if (transform_.direction() == newDirection) {
    return;
  }
  
  PositionBufferSlot slot{transform_.x(), transform_.y(), newDirection};
  transform_.setDirection(newDirection);
  if (newDirection == Direction::Left || newDirection == Direction::Right) {
    transform_.setFace(newDirection);
  }
  
  if (next) {
    next->positionBuffer().push(slot);
  }
}

// Direct component access
TransformComponent& Sprite::transform() { return transform_; }
const TransformComponent& Sprite::transform() const { return transform_; }
HealthComponent& Sprite::health() { return health_; }
const HealthComponent& Sprite::health() const { return health_; }
RenderComponent& Sprite::render() { return render_; }
const RenderComponent& Sprite::render() const { return render_; }
CombatComponent& Sprite::combat() { return combat_; }
const CombatComponent& Sprite::combat() const { return combat_; }
