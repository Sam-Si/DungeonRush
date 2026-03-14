#include "entities/entity_manager.h"

namespace snake {
namespace entities {

void EntityManager::addSnake(const std::shared_ptr<Snake>& snake) {
  if (spritesCount_ < kSpritesMaxNum) {
    snakes_[spritesCount_++] = snake;
  }
}

void EntityManager::removeSnake(int index) {
  if (index >= 0 && index < spritesCount_) {
    snakes_[index] = nullptr;
  }
}

std::shared_ptr<Snake> EntityManager::getSnake(int index) const {
  if (index >= 0 && index < kSpritesMaxNum) {
    return snakes_[index];
  }
  return nullptr;
}

int EntityManager::snakeCount() const {
  return spritesCount_;
}

int EntityManager::playerCount() const {
  return playersCount_;
}

void EntityManager::setPlayerCount(int count) {
  playersCount_ = count;
}

void EntityManager::incrementSpriteCount() {
  ++spritesCount_;
}

int EntityManager::spriteCount() const {
  return spritesCount_;
}

void EntityManager::addBullet(const std::shared_ptr<Bullet>& bullet) {
  bullets_.push_back(bullet);
}

void EntityManager::removeBullet(const std::shared_ptr<Bullet>& bullet) {
  bullets_.remove(bullet);
}

BulletList& EntityManager::bullets() {
  return bullets_;
}

const BulletList& EntityManager::bullets() const {
  return bullets_;
}

std::array<std::shared_ptr<Snake>, kSpritesMaxNum>& EntityManager::snakes() {
  return snakes_;
}

const std::array<std::shared_ptr<Snake>, kSpritesMaxNum>& EntityManager::snakes() const {
  return snakes_;
}

void EntityManager::clear() {
  for (int i = 0; i < kSpritesMaxNum; ++i) {
    snakes_[i] = nullptr;
  }
  bullets_.clear();
  spritesCount_ = 0;
  playersCount_ = 0;
}

}  // namespace entities
}  // namespace snake
