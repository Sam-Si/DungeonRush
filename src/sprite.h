#ifndef SNAKE_SPRITE_H_
#define SNAKE_SPRITE_H_

#include "adt.h"
#include "component.h"
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
  PositionBufferSlot pop();
  int size() const;

  const PositionBufferSlot& at(int index) const;

 private:
  std::array<PositionBufferSlot, POSITION_BUFFER_SIZE> buffer_{};
  int size_ = 0;
};

// ============================================================================
// Sprite - Composes TransformComponent, HealthComponent, RenderComponent,
//          and CombatComponent to avoid massive base class
// ============================================================================
class Sprite {
 public:
  Sprite();
  Sprite(const Sprite& model, int x, int y);
  Sprite(const Sprite&) = delete;
  Sprite& operator=(const Sprite&) = delete;
  Sprite(Sprite&&) noexcept = default;
  Sprite& operator=(Sprite&&) noexcept = default;
  ~Sprite() = default;

  // TransformComponent accessors
  int x() const;
  int y() const;
  Direction face() const;
  Direction direction() const;
  void setPosition(int x, int y);
  void setFace(Direction face);
  void setDirection(Direction direction);

  // HealthComponent accessors
  int hp() const;
  int totalHp() const;
  void setHp(int hp);
  void setTotalHp(int totalHp);

  // RenderComponent accessors
  std::shared_ptr<Animation> animation() const;
  void setAnimation(const std::shared_ptr<Animation>& animation);

  // CombatComponent accessors
  Weapon* weapon() const;
  int lastAttack() const;
  double dropRate() const;
  void setWeapon(Weapon* weapon);
  void setLastAttack(int lastAttack);
  void setDropRate(double dropRate);

  // PositionBuffer for chained movement
  PositionBuffer& positionBuffer();
  const PositionBuffer& positionBuffer() const;

  void enqueueDirectionChange(Direction newDirection,
                              const std::shared_ptr<Sprite>& next);

  // Direct component access for advanced usage
  TransformComponent& transform();
  const TransformComponent& transform() const;
  HealthComponent& health();
  const HealthComponent& health() const;
  RenderComponent& render();
  const RenderComponent& render() const;
  CombatComponent& combat();
  const CombatComponent& combat() const;

 private:
  TransformComponent transform_{};
  HealthComponent health_{};
  RenderComponent render_{};
  CombatComponent combat_{};
  PositionBuffer positionBuffer_{};
};

#endif
