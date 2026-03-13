#ifndef SNAKE_MODELS_GAME_EVENT_H_
#define SNAKE_MODELS_GAME_EVENT_H_

#include "types.h"

#include <functional>
#include <vector>

namespace snake {
namespace models {

enum class GameEventType { PlayerDied, ItemPicked, SoundRequested };

struct GameEvent {
  GameEventType type = GameEventType::SoundRequested;
  int playerId = -1;
  ItemType itemType = ItemType::None;
  int audioId = -1;
};

class EventBus {
 public:
  using Listener = std::function<void(const GameEvent&)>;

  EventBus() = default;
  EventBus(const EventBus&) = delete;
  EventBus& operator=(const EventBus&) = delete;
  EventBus(EventBus&&) = default;
  EventBus& operator=(EventBus&&) = default;
  ~EventBus() = default;

  void subscribe(Listener listener);
  void emit(const GameEvent& event) const;
  void clear();

 private:
  std::vector<Listener> listeners_{};
};

}  // namespace models
}  // namespace snake

#endif  // SNAKE_MODELS_GAME_EVENT_H_
