#include "models/game_event.h"

namespace snake {
namespace models {

void EventBus::subscribe(Listener listener) {
  listeners_.push_back(std::move(listener));
}

void EventBus::emit(const GameEvent& event) const {
  for (const auto& listener : listeners_) {
    listener(event);
  }
}

void EventBus::clear() {
  listeners_.clear();
}

}  // namespace models
}  // namespace snake
