#include "player.h"

Snake::Snake(int step, int team, PlayerType playerType)
    : moveStep_(step), team_(team), playerType_(playerType) {
  scoreComponent_.setScore(std::make_shared<Score>());
}

// Identity accessors
int Snake::moveStep() const { return moveStep_; }
int Snake::team() const { return team_; }
int Snake::num() const { return num_; }
PlayerType Snake::playerType() const { return playerType_; }

void Snake::setMoveStep(int step) { moveStep_ = step; }
void Snake::setTeam(int team) { team_ = team; }
void Snake::incrementNum() { ++num_; }

// BuffComponent accessors
const std::array<int, BUFF_END>& Snake::buffs() const {
  return buffComponent_.buffs();
}
std::array<int, BUFF_END>& Snake::buffs() { return buffComponent_.buffs(); }
bool Snake::isFrozen() const { return buffComponent_.isFrozen(); }
bool Snake::isSlowed() const { return buffComponent_.isSlowed(); }
bool Snake::hasDefense() const { return buffComponent_.hasDefense(); }
bool Snake::hasAttackUp() const { return buffComponent_.hasAttackUp(); }

// ScoreComponent accessors
const std::shared_ptr<Score>& Snake::score() const {
  return scoreComponent_.score();
}
std::shared_ptr<Score>& Snake::score() { return scoreComponent_.score(); }

// AIComponent accessors
AIBehavior* Snake::aiBehavior() const { return aiComponent_.behavior(); }
bool Snake::hasAIBehavior() const { return aiComponent_.hasBehavior(); }
void Snake::setAIBehavior(std::shared_ptr<AIBehavior> behavior) {
  aiComponent_.setBehavior(std::move(behavior));
}

// Sprite management
SpriteList& Snake::sprites() { return sprites_; }
const SpriteList& Snake::sprites() const { return sprites_; }

// Direct component access
BuffComponent& Snake::buffComponent() { return buffComponent_; }
const BuffComponent& Snake::buffComponent() const { return buffComponent_; }
ScoreComponent& Snake::scoreComponent() { return scoreComponent_; }
const ScoreComponent& Snake::scoreComponent() const {
  return scoreComponent_;
}
AIComponent& Snake::aiComponent() { return aiComponent_; }
const AIComponent& Snake::aiComponent() const { return aiComponent_; }
