#ifndef SNAKE_BULLET_H_
#define SNAKE_BULLET_H_

#include "types.h"
#include "weapon.h"
#include "player.h"

#include <memory>

class Bullet {
 public:
  Bullet(const std::shared_ptr<Snake>& owner, Weapon* parent, int x, int y,
         double rad, int team, const std::shared_ptr<Animation>& animation);
  Bullet(const Bullet&) = delete;
  Bullet& operator=(const Bullet&) = delete;
  Bullet(Bullet&&) noexcept = default;
  Bullet& operator=(Bullet&&) noexcept = default;
  ~Bullet() = default;

  Weapon* parent() const;
  int x() const;
  int y() const;
  int team() const;
  const std::shared_ptr<Snake>& owner() const;
  double rad() const;
  std::shared_ptr<Animation> animation() const;

  void setPosition(int x, int y);
  void update();

 private:
  Weapon* parent_ = nullptr;
  int x_ = 0;
  int y_ = 0;
  int team_ = 0;
  std::shared_ptr<Snake> owner_{};
  double rad_ = 0.0;
  std::shared_ptr<Animation> animation_{};
};

#endif
