#ifndef SNAKE_AI_H_
#define SNAKE_AI_H_

#include "game.h"
#include "player.h"

#include <memory>

#define AI_PATH_RANDOM 0.01
#define AI_PREDICT_STEPS 38
#define AI_DECIDE_RATE 4

struct Choice {
  int value = 0;
  Direction direction = Direction::Right;
};

void AiInput(const std::shared_ptr<Snake>& snake);
int getPowerfulPlayer();

#endif
