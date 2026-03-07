#ifndef SNAKE_SPRITE_H_
#define SNAKE_SPRITE_H_

#include "adt.h"
#include "types.h"
#include "weapon.h"

#include <array>
#include <memory>

class Sprite;

struct PositionBufferSlot {
  int x = 0;
  int y = 0;
  Direction direction = Direction::Right;
};

class PositionBuffer {
 public:
  PositionBuffer();

  void push(PositionBufferSlot slot);
  int size() const;

  const PositionBufferSlot& at(int index) const;

 private:
  std::array<PositionBufferSlot, POSITION_BUFFER_SIZE> buffer_{};
  int size_ = 0;
};

class Sprite {
 public:
  Sprite();
  Sprite(const Sprite& model, int x, int y);
  Sprite(const Sprite&) = delete;
  Sprite& operator=(const Sprite&) = delete;
  Sprite(Sprite&&) noexcept = default;
  Sprite& operator=(Sprite&&) noexcept = default;
  ~Sprite() = default;

  int x() const;
  int y() const;
  int hp() const;
  int totalHp() const;
  Weapon* weapon() const;
  std::shared_ptr<Animation> animation() const;
  Direction face() const;
  Direction direction() const;
  int lastAttack() const;
  double dropRate() const;

  void setPosition(int x, int y);
  void setHp(int hp);
  void setTotalHp(int totalHp);
  void setWeapon(Weapon* weapon);
  void setAnimation(const std::shared_ptr<Animation>& animation);
  void setFace(Direction face);
  void setDirection(Direction direction);
  void setLastAttack(int lastAttack);
  void setDropRate(double dropRate);

  PositionBuffer& positionBuffer();
  const PositionBuffer& positionBuffer() const;

  void enqueueDirectionChange(Direction newDirection,
                              const std::shared_ptr<Sprite>& next);

 private:
  int x_ = 0;
  int y_ = 0;
  int hp_ = 0;
  int totalHp_ = 0;
  Weapon* weapon_ = nullptr;
  std::shared_ptr<Animation> animation_{};
  Direction face_ = Direction::Right;
  Direction direction_ = Direction::Right;
  PositionBuffer positionBuffer_{};
  int lastAttack_ = 0;
  double dropRate_ = 0.0;
};

#endif
