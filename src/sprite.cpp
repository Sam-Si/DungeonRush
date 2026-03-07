#include "sprite.h"

#include <cassert>

#include "render.h"

PositionBuffer::PositionBuffer() = default;

Sprite::Sprite() = default;

void PositionBuffer::push(PositionBufferSlot slot) {
  assert(size_ < POSITION_BUFFER_SIZE);
  buffer_[size_++] = slot;
}

int PositionBuffer::size() const { return size_; }

const PositionBufferSlot& PositionBuffer::at(int index) const {
  return buffer_.at(index);
}

Sprite::Sprite(const Sprite& model, int x, int y)
    : x_(x),
      y_(y),
      hp_(model.hp_),
      totalHp_(model.totalHp_),
      weapon_(model.weapon_),
      animation_(model.animation_ ? std::make_shared<Animation>(*model.animation_)
                                  : nullptr),
      face_(model.face_),
      direction_(model.direction_),
      lastAttack_(model.lastAttack_),
      dropRate_(model.dropRate_) {
  if (animation_) {
    animation_->setPosition(x_, y_);
  }
}

int Sprite::x() const { return x_; }
int Sprite::y() const { return y_; }
int Sprite::hp() const { return hp_; }
int Sprite::totalHp() const { return totalHp_; }
Weapon* Sprite::weapon() const { return weapon_; }
std::shared_ptr<Animation> Sprite::animation() const { return animation_; }
Direction Sprite::face() const { return face_; }
Direction Sprite::direction() const { return direction_; }
int Sprite::lastAttack() const { return lastAttack_; }
double Sprite::dropRate() const { return dropRate_; }

void Sprite::setPosition(int x, int y) {
  x_ = x;
  y_ = y;
  if (animation_) {
    animation_->setPosition(x_, y_);
  }
}

void Sprite::setHp(int hp) { hp_ = hp; }
void Sprite::setTotalHp(int totalHp) { totalHp_ = totalHp; }
void Sprite::setWeapon(Weapon* weapon) { weapon_ = weapon; }
void Sprite::setAnimation(const std::shared_ptr<Animation>& animation) {
  animation_ = animation;
}
void Sprite::setFace(Direction face) { face_ = face; }
void Sprite::setDirection(Direction direction) { direction_ = direction; }
void Sprite::setLastAttack(int lastAttack) { lastAttack_ = lastAttack; }
void Sprite::setDropRate(double dropRate) { dropRate_ = dropRate; }

PositionBuffer& Sprite::positionBuffer() { return positionBuffer_; }
const PositionBuffer& Sprite::positionBuffer() const { return positionBuffer_; }

void Sprite::enqueueDirectionChange(Direction newDirection,
                                    const std::shared_ptr<Sprite>& next) {
  if (direction_ == newDirection) {
    return;
  }
  direction_ = newDirection;
  if (newDirection == Direction::Left || newDirection == Direction::Right) {
    face_ = newDirection;
  }
  if (next) {
    PositionBufferSlot slot{x_, y_, direction_};
    next->positionBuffer().push(slot);
  }
}
