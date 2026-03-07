#include "player.h"

Snake::Snake(int step, int team, PlayerType playerType)
    : moveStep_(step), team_(team), playerType_(playerType) {
  buffs_.fill(0);
  score_ = std::make_shared<Score>();
}

int Snake::moveStep() const { return moveStep_; }
int Snake::team() const { return team_; }
int Snake::num() const { return num_; }
PlayerType Snake::playerType() const { return playerType_; }

const std::array<int, BUFF_END>& Snake::buffs() const { return buffs_; }
std::array<int, BUFF_END>& Snake::buffs() { return buffs_; }
const std::shared_ptr<Score>& Snake::score() const { return score_; }

void Snake::setMoveStep(int step) { moveStep_ = step; }
void Snake::setTeam(int team) { team_ = team; }
void Snake::incrementNum() { ++num_; }

SpriteList& Snake::sprites() { return sprites_; }
const SpriteList& Snake::sprites() const { return sprites_; }
