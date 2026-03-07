#include "ui.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "audio.h"
#include "game.h"
#include "helper.h"
#include "map.h"
#include "net.h"
#include "render.h"
#include "res.h"
#include "storage.h"
#include "text.h"
#include "types.h"

extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;
extern bool hasMap[MAP_SIZE][MAP_SIZE];
extern Text texts[TEXTSET_SIZE];
extern SDL_Color WHITE;
extern Texture textures[];
extern Effect effects[];

int cursorPos;

namespace {
std::unique_ptr<Text> createUiText(const std::string& text, SDL_Color color) {
  SDL_Renderer* sdlRenderer = renderer();
  TTF_Font* ttfFont = font();
  if (!sdlRenderer || !ttfFont) {
    return nullptr;
  }
  return std::make_unique<Text>(text, color, sdlRenderer, ttfFont);
}

class GameStateManager;

class GameState {
 public:
  GameState() = default;
  GameState(const GameState&) = delete;
  GameState& operator=(const GameState&) = delete;
  GameState(GameState&&) = default;
  GameState& operator=(GameState&&) = default;
  virtual ~GameState() = default;

  virtual void handleInput(GameStateManager& manager) = 0;
  virtual void update(GameStateManager& manager) = 0;
  virtual void render(GameStateManager& manager) = 0;
};

class GameStateManager {
 public:
  GameStateManager() = default;
  GameStateManager(const GameStateManager&) = delete;
  GameStateManager& operator=(const GameStateManager&) = delete;
  GameStateManager(GameStateManager&&) = default;
  GameStateManager& operator=(GameStateManager&&) = default;
  ~GameStateManager() = default;

  void setState(std::unique_ptr<GameState> state) {
    currentState_ = std::move(state);
  }

  void clearState() {
    currentState_.reset();
    nextState_.reset();
  }

  void setNextState(std::unique_ptr<GameState> state) {
    nextState_ = std::move(state);
  }

  void run() {
    while (currentState_) {
      currentState_->handleInput(*this);
      currentState_->update(*this);
      currentState_->render(*this);
      if (nextState_) {
        currentState_ = std::move(nextState_);
      }
    }
  }

 private:
  std::unique_ptr<GameState> currentState_{};
  std::unique_ptr<GameState> nextState_{};
};

class ExitState final : public GameState {
 public:
  void handleInput(GameStateManager& manager) override {
    manager.clearState();
  }
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}
};

class MainMenuState final : public GameState {
 public:
  void handleInput(GameStateManager& manager) override;
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}
};

class LocalRankListState final : public GameState {
 public:
  void handleInput(GameStateManager& manager) override {
    localRankListUi();
    manager.setNextState(std::make_unique<MainMenuState>());
  }
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}
};

class LanGameState final : public GameState {
 public:
  void handleInput(GameStateManager& manager) override {
    launchLanGame();
    manager.setNextState(std::make_unique<MainMenuState>());
  }
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}
};

class LocalGameState final : public GameState {
 public:
  explicit LocalGameState(int localPlayers) : localPlayers_(localPlayers) {}

  void handleInput(GameStateManager& manager) override {
    launchLocalGame(localPlayers_);
    manager.setNextState(std::make_unique<MainMenuState>());
  }
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}

 private:
  int localPlayers_ = 1;
};

class ChooseLevelState final : public GameState {
 public:
  explicit ChooseLevelState(int localPlayers) : localPlayers_(localPlayers) {}

  void handleInput(GameStateManager& manager) override {
    if (!chooseLevelUi()) {
      manager.setNextState(std::make_unique<MainMenuState>());
      return;
    }
    manager.setNextState(std::make_unique<LocalGameState>(localPlayers_));
  }
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}

 private:
  int localPlayers_ = 1;
};

class ChooseModeState final : public GameState {
 public:
  void handleInput(GameStateManager& manager) override {
    int mode = chooseOnLanUi();
    if (mode == 0) {
      manager.setNextState(std::make_unique<ChooseLevelState>(2));
      return;
    }
    manager.setNextState(std::make_unique<LanGameState>());
  }
  void update(GameStateManager&) override {}
  void render(GameStateManager&) override {}
};

void MainMenuState::handleInput(GameStateManager& manager) {
  baseUi(30, 12);
  playBgm(0);
  const int startY = SCREEN_HEIGHT / 2 - 70;
  const int startX = SCREEN_WIDTH / 5 + 32;
  createAndPushAnimation(animationsList[RENDER_LIST_UI_ID],
                         &textures[RES_TITLE], nullptr, LoopType::Infinite, 80,
                         SCREEN_WIDTH / 2, 280, SDL_FLIP_NONE, 0, At::Center);
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_KNIGHT_M], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, startX, startY,
                         SDL_FLIP_NONE, 0, At::BottomCenter);
  auto swordFx = createAndPushAnimation(
      animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_SwordFx], nullptr,
      LoopType::Infinite, SPRITE_ANIMATION_DURATION,
      startX + UI_MAIN_GAP_ALT * 2, startY - 32, SDL_FLIP_NONE, 0,
      At::BottomCenter);
  if (swordFx) {
    swordFx->setScaled(false);
  }
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_CHORT], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION,
                         startX + UI_MAIN_GAP_ALT * 2, startY - 32,
                         SDL_FLIP_HORIZONTAL, 0, At::BottomCenter);

  const int elfX = startX + UI_MAIN_GAP_ALT * (6 + 2 * randDouble());
  const int elfY = startY + UI_MAIN_GAP * (1 + randDouble());
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_ELF_M], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, elfX, elfY,
                         SDL_FLIP_HORIZONTAL, 0, At::BottomCenter);
  createAndPushAnimation(animationsList[RENDER_LIST_EFFECT_ID],
                         &textures[RES_HALO_EXPLOSION2], nullptr,
                         LoopType::Infinite, SPRITE_ANIMATION_DURATION,
                         elfX - UI_MAIN_GAP * 1.5, elfY, SDL_FLIP_NONE, 0,
                         At::BottomCenter);
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_ZOMBIE], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, elfX - UI_MAIN_GAP * 1.5,
                         elfY, SDL_FLIP_NONE, 0, At::BottomCenter);

  const int wizardX = elfX - UI_MAIN_GAP_ALT * (1 + 2 * randDouble());
  const int wizardY = elfY + UI_MAIN_GAP * (2 + randDouble());
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_WIZZARD_M], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, wizardX, wizardY,
                         SDL_FLIP_NONE, 0, At::BottomCenter);
  createAndPushAnimation(animationsList[RENDER_LIST_EFFECT_ID],
                         &textures[RES_FIREBALL], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, wizardX + UI_MAIN_GAP,
                         wizardY, SDL_FLIP_NONE, 0, At::BottomCenter);

  const int lizardX = wizardX + UI_MAIN_GAP_ALT * (18 + 4 * randDouble());
  const int lizardY = wizardY - UI_MAIN_GAP * (1 + 3 * randDouble());
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_LIZARD_M], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, lizardX, lizardY,
                         SDL_FLIP_NONE, 0, At::BottomCenter);
  createAndPushAnimation(
      animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_CLAWFX2], nullptr,
      LoopType::Infinite, SPRITE_ANIMATION_DURATION, lizardX,
      lizardY - UI_MAIN_GAP + 16, SDL_FLIP_NONE, 0, At::BottomCenter);
  createAndPushAnimation(
      animationsList[RENDER_LIST_SPRITE_ID], &textures[RES_MUDDY], nullptr,
      LoopType::Infinite, SPRITE_ANIMATION_DURATION, lizardX,
      lizardY - UI_MAIN_GAP, SDL_FLIP_HORIZONTAL, 0, At::BottomCenter);

  createAndPushAnimation(
      animationsList[RENDER_LIST_EFFECT_ID], &textures[RES_CLAWFX2], nullptr,
      LoopType::Infinite, SPRITE_ANIMATION_DURATION, lizardX + UI_MAIN_GAP,
      lizardY - UI_MAIN_GAP + 16, SDL_FLIP_NONE, 0, At::BottomCenter);
  createAndPushAnimation(
      animationsList[RENDER_LIST_SPRITE_ID], &textures[RES_SWAMPY], nullptr,
      LoopType::Infinite, SPRITE_ANIMATION_DURATION, lizardX + UI_MAIN_GAP,
      lizardY - UI_MAIN_GAP, SDL_FLIP_HORIZONTAL, 0, At::BottomCenter);

  createAndPushAnimation(animationsList[RENDER_LIST_EFFECT_ID],
                         &textures[RES_CLAWFX2], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, lizardX + UI_MAIN_GAP,
                         lizardY + 16, SDL_FLIP_NONE, 0, At::BottomCenter);
  createAndPushAnimation(animationsList[RENDER_LIST_SPRITE_ID],
                         &textures[RES_SWAMPY], nullptr, LoopType::Infinite,
                         SPRITE_ANIMATION_DURATION, lizardX + UI_MAIN_GAP,
                         lizardY, SDL_FLIP_HORIZONTAL, 0, At::BottomCenter);

  const int optsNum = 4;
  std::vector<Text*> opts;
  opts.reserve(optsNum);
  for (int i = 0; i < optsNum; i++) {
    opts.push_back(texts + i + 6);
  }
  const int opt = chooseOptions(optsNum, opts.data());

  blackout();
  clearRenderer();
  switch (opt) {
    case 0:
      manager.setNextState(std::make_unique<ChooseLevelState>(1));
      break;
    case 1:
      manager.setNextState(std::make_unique<ChooseModeState>());
      break;
    case 2:
      manager.setNextState(std::make_unique<LocalRankListState>());
      break;
    case 3:
    default:
      manager.setNextState(std::make_unique<ExitState>());
      break;
  }
}
}  // namespace

bool moveCursor(int optsNum) {
  SDL_Event e;
  bool quit = false;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      quit = true;
      cursorPos = optsNum;
      return quit;
    } else if (e.type == SDL_KEYDOWN) {
      int keyValue = e.key.keysym.sym;
      switch (keyValue) {
        case SDLK_UP:
          cursorPos--;
          playAudio(AUDIO_INTER1);
          break;
        case SDLK_DOWN:
          cursorPos++;
          playAudio(AUDIO_INTER1);
          break;
        case SDLK_RETURN:
          quit = true;
          break;
        case SDLK_ESCAPE:
          quit = true;
          cursorPos = optsNum;
          playAudio(AUDIO_BUTTON1);
          return quit;
          break;
      }
    }
  }
  cursorPos += optsNum;
  cursorPos %= optsNum;
  return quit;
}
int chooseOptions(int optionsNum, Text** options) {
  cursorPos = 0;
  auto player = std::make_shared<Snake>(2, 0, PlayerType::Local);
  appendSpriteToSnake(player, SPRITE_KNIGHT, SCREEN_WIDTH / 2,
                      SCREEN_HEIGHT / 2, Direction::Up);
  const int lineGap = FONT_SIZE + FONT_SIZE / 2;
  const int totalHeight = lineGap * (optionsNum - 1);
  const int startY = (SCREEN_HEIGHT - totalHeight) / 2;
  while (!moveCursor(optionsNum)) {
    auto head = player->sprites().empty() ? nullptr : player->sprites().front();
    if (head && head->animation()) {
      head->animation()->setAt(At::Center);
      head->setPosition(SCREEN_WIDTH / 2 - options[cursorPos]->width() / 2 - UNIT / 2,
                        startY + cursorPos * lineGap);
      updateAnimationOfSprite(head);
    }
    renderUi();
    for (int i = 0; i < optionsNum; i++) {
      renderCenteredText(options[i], SCREEN_WIDTH / 2, startY + i * lineGap, 1);
    }
    // Update Screen
    if (SDL_Renderer* sdlRenderer = renderer()) {
      SDL_RenderPresent(sdlRenderer);
    }
    incrementRenderFrames();
  }
  playAudio(AUDIO_BUTTON1);
  destroySnake(player);
  animationsList[RENDER_LIST_SPRITE_ID].clear();
  return cursorPos;
}
void baseUi(int w, int h) {
  initRenderer();
  initBlankMap(w, h);
  pushMapToRender();
}
bool chooseLevelUi() {
  baseUi(30, 12);
  const int optsNum = 3;
  std::vector<Text*> opts;
  opts.reserve(optsNum);
  for (int i = 0; i < optsNum; i++) {
    opts.push_back(texts + i + 10);
  }
  const int opt = chooseOptions(optsNum, opts.data());
  if (opt != optsNum) {
    setLevel(opt);
  }
  clearRenderer();
  return opt != optsNum;
}

void launchLocalGame(int localPlayerNum) {
  auto scores = startGame(localPlayerNum, 0, true);
  std::vector<Score*> rawScores;
  rawScores.reserve(scores.size());
  for (auto& score : scores) {
    rawScores.push_back(score.get());
  }
  rankListUi(localPlayerNum, rawScores.data());
  for (int i = 0; i < localPlayerNum; i++) updateLocalRanklist(rawScores[i]);
}
int rangeOptions(int start, int end) {
  const int optsNum = end - start + 1;
  std::vector<Text*> opts;
  opts.reserve(optsNum);
  for (int i = 0; i < optsNum; i++) {
    opts.push_back(texts + i + start);
  }
  const int opt = chooseOptions(optsNum, opts.data());
  return opt;
}

std::optional<std::string> inputUi() {
  constexpr int kMaxLen = 30;

  baseUi(20, 10);

  std::string ret;
  ret.reserve(kMaxLen);

  extern SDL_Color WHITE;
  std::unique_ptr<Text> text(nullptr);
  std::unique_ptr<Text> placeholder(createUiText("Enter IP", WHITE));

  SDL_StartTextInput();
  SDL_Event e;
  bool quit = false;
  bool finished = false;
  while (!quit && !finished) {
    const Text* displayText = nullptr;
    if (!ret.empty()) {
      if (text) {
        text->setText(ret, renderer(), font());
      } else {
        text = createUiText(ret, WHITE);
      }
      displayText = text.get();
    } else {
      displayText = placeholder.get();
    }
    renderCenteredText(displayText, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2);
    if (SDL_Renderer* sdlRenderer = renderer()) {
      SDL_RenderPresent(sdlRenderer);
    }
    clearRenderer();

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT ||
          (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
        quit = true;
        break;
      } else if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_BACKSPACE) {
          if (!ret.empty()) {
            ret.pop_back();
          }
        } else if (e.key.keysym.sym == SDLK_RETURN) {
          finished = true;
          break;
        }
      } else if (e.type == SDL_TEXTINPUT) {
        if (ret.size() < kMaxLen) {
          ret.append(e.text.text);
        }
      }
    }
  }

  SDL_StopTextInput();

  if (quit) {
    return std::nullopt;
  }

  return ret;
}

void launchLanGame() {
  baseUi(10, 10);
  int opt = rangeOptions(LAN_HOSTGAME, LAN_JOINGAME);
  blackout();
  clearRenderer();
  if (opt == 0) {
    hostGame();
  } else {
    const auto ip = inputUi();
    if (!ip.has_value()) {
      return;
    }
    joinGame(ip->c_str(), LAN_LISTEN_PORT);
  }
}

int chooseOnLanUi() {
  baseUi(10, 10);
  int opt = rangeOptions(MULTIPLAYER_LOCAL, MULTIPLAYER_LAN);
  clearRenderer();
  return opt;
}

namespace {
void runUiStateMachine() {
  GameStateManager manager;
  manager.setState(std::make_unique<MainMenuState>());
  manager.run();
}
}  // namespace

void mainUi() {
  runUiStateMachine();
}
void rankListUi(int count, Score** scores) {
  baseUi(30, 12 + MAX(0, count - 4));
  playBgm(0);
  std::vector<std::unique_ptr<Text>> ownedTexts;
  ownedTexts.reserve(count);
  std::vector<Text*> opts;
  opts.reserve(count);
  char buf[1 << 8];
  for (int i = 0; i < count; i++) {
    sprintf(buf, "Score: %-6.0lf Got: %-6d Kill: %-6d Damage: %-6d Stand: %-6d",
            scores[i]->rank(), scores[i]->got(), scores[i]->killed(),
            scores[i]->damage(), scores[i]->stand());
    ownedTexts.push_back(createUiText(buf, WHITE));
    opts.push_back(ownedTexts.back().get());
  }

  chooseOptions(count, opts.data());

  blackout();
  clearRenderer();
}
void localRankListUi() {
  int count;
  Score** scores = readRanklist(STORAGE_PATH, &count);
  rankListUi(count, scores);
  destroyRanklist(count, scores);
}
