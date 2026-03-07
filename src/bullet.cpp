#include "bullet.h"

#include <cmath>

#include "helper.h"

Bullet::Bullet(const std::shared_ptr<Snake>& owner, Weapon* parent, int x, int y,
               double rad, int team,
               const std::shared_ptr<Animation>& animation)
    : parent_(parent),
      x_(x),
      y_(y),
      team_(team),
      owner_(owner),
      rad_(rad),
      animation_(animation ? std::make_shared<Animation>(*animation) : nullptr) {
  if (animation_) {
    animation_->setPosition(x_, y_);
    animation_->setAngle(rad_ * 180.0 / PI);
  }
}

Weapon* Bullet::parent() const { return parent_; }
int Bullet::x() const { return x_; }
int Bullet::y() const { return y_; }
int Bullet::team() const { return team_; }
const std::shared_ptr<Snake>& Bullet::owner() const { return owner_; }
double Bullet::rad() const { return rad_; }
std::shared_ptr<Animation> Bullet::animation() const { return animation_; }

void Bullet::setPosition(int x, int y) {
  x_ = x;
  y_ = y;
  if (animation_) {
    animation_->setPosition(x_, y_);
  }
}

void Bullet::update() {
  const int speed = parent_ ? parent_->bulletSpeed() : 0;
  x_ += static_cast<int>(std::cos(rad_) * speed);
  y_ += static_cast<int>(std::sin(rad_) * speed);
  if (animation_) {
    animation_->setPosition(x_, y_);
  }
}
