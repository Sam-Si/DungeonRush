#ifndef SNAKE__PLAYER_H_
#define SNAKE__PLAYER_H_

#include "adt.h"
#include "types.h"

#include <array>
#include <memory>

enum class PlayerType { Local, Remote, Computer };

class Snake {
 public:
  Snake(int step, int team, PlayerType playerType);
  Snake(const Snake&) = delete;
  Snake& operator=(const Snake&) = delete;
  Snake(Snake&&) noexcept = default;
  Snake& operator=(Snake&&) noexcept = default;
  ~Snake() = default;

  int moveStep() const;
  int team() const;
  int num() const;
  PlayerType playerType() const;
  const std::array<int, BUFF_END>& buffs() const;
  std::array<int, BUFF_END>& buffs();
  const std::shared_ptr<Score>& score() const;

  void setMoveStep(int step);
  void setTeam(int team);
  void incrementNum();

  SpriteList& sprites();
  const SpriteList& sprites() const;

 private:
  SpriteList sprites_{};
  int moveStep_ = 0;
  int team_ = 0;
  int num_ = 0;
  std::array<int, BUFF_END> buffs_{};
  std::shared_ptr<Score> score_{};
  PlayerType playerType_ = PlayerType::Local;
};

#endif
