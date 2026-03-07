#ifndef SNAKE__PLAYER_H_
#define SNAKE__PLAYER_H_

#include "adt.h"
#include "component.h"
#include "types.h"

#include <array>
#include <memory>

enum class PlayerType { Local, Remote, Computer };

// ============================================================================
// Snake - Composes BuffComponent, ScoreComponent, and AIComponent
//          to avoid massive base class. Sprite list is managed separately.
// ============================================================================
class Snake {
 public:
  Snake(int step, int team, PlayerType playerType);
  Snake(const Snake&) = delete;
  Snake& operator=(const Snake&) = delete;
  Snake(Snake&&) noexcept = default;
  Snake& operator=(Snake&&) noexcept = default;
  ~Snake() = default;

  // Identity accessors
  int moveStep() const;
  int team() const;
  int num() const;
  PlayerType playerType() const;

  void setMoveStep(int step);
  void setTeam(int team);
  void incrementNum();

  // BuffComponent accessors
  const std::array<int, BUFF_END>& buffs() const;
  std::array<int, BUFF_END>& buffs();
  bool isFrozen() const;
  bool isSlowed() const;
  bool hasDefense() const;
  bool hasAttackUp() const;

  // ScoreComponent accessors
  const std::shared_ptr<Score>& score() const;
  std::shared_ptr<Score>& score();

  // AIComponent accessors
  AIBehavior* aiBehavior() const;
  bool hasAIBehavior() const;
  void setAIBehavior(std::shared_ptr<AIBehavior> behavior);

  // Sprite management
  SpriteList& sprites();
  const SpriteList& sprites() const;

  // Direct component access for advanced usage
  BuffComponent& buffComponent();
  const BuffComponent& buffComponent() const;
  ScoreComponent& scoreComponent();
  const ScoreComponent& scoreComponent() const;
  AIComponent& aiComponent();
  const AIComponent& aiComponent() const;

 private:
  SpriteList sprites_{};
  BuffComponent buffComponent_{};
  ScoreComponent scoreComponent_{};
  AIComponent aiComponent_{};
  int moveStep_ = 0;
  int team_ = 0;
  int num_ = 0;
  PlayerType playerType_ = PlayerType::Local;
};

#endif
