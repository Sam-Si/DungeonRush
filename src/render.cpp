#include "render.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "ai.h"
#include "game.h"
#include "helper.h"
#include "res.h"
#include "types.h"

#ifdef DBG
#include <cassert>
#endif

namespace {
std::shared_ptr<Text> stageText;
std::shared_ptr<Text> taskText;
std::array<std::shared_ptr<Text>, MAX_PALYERS_NUM> scoresText;
std::shared_ptr<Animation> countDownBar;

std::shared_ptr<Text> makeText(const std::string& text,
                               SDL_Renderer* sdlRenderer, TTF_Font* ttfFont,
                               SDL_Color color) {
  if (!sdlRenderer || !ttfFont) {
    return nullptr;
  }
  return std::make_shared<Text>(text, color, sdlRenderer, ttfFont);
}
}  // namespace

// Sprite
extern std::array<std::shared_ptr<Snake>, SPRITES_MAX_NUM> spriteSnake;
extern int spritesCount;
extern int playersCount;
extern Effect effects[];
extern SDL_Color BLACK;
extern SDL_Color WHITE;
extern int gameLevel;

extern const int SCALE_FACTOR = 2;
extern int texturesCount;
extern Texture textures[TEXTURES_SIZE];
extern int textsCount;
extern Text texts[TEXTSET_SIZE];

std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;

void blacken(int duration) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!sdlRenderer) {
    return;
  }
  SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
  SDL_Rect rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
  SDL_SetRenderDrawColor(sdlRenderer, RENDER_BG_COLOR, 85);
  for (int i = 0; i < duration; i++) {
    SDL_RenderFillRect(sdlRenderer, &rect);
    SDL_RenderPresent(sdlRenderer);
  }
}
void blackout() { blacken(RENDER_BLACKOUT_DURATION); }
void dim() { blacken(RENDER_DIM_DURATION); }
void initCountDownBar() {
  createAndPushAnimation(
      animationsList[RENDER_LIST_UI_ID], &textures[RES_SLIDER], nullptr,
      LoopType::Infinite, 1, SCREEN_WIDTH / 2 - 128, 10, SDL_FLIP_NONE, 0.0,
      At::TopLeft);
  countDownBar = createAndPushAnimation(
      animationsList[RENDER_LIST_UI_ID], &textures[RES_BAR_BLUE], nullptr,
      LoopType::Infinite, 1, SCREEN_WIDTH / 2 - 128, 10, SDL_FLIP_NONE, 0.0,
      At::TopLeft);
}
void initInfo() {
  extern int stage;
  SDL_Renderer* sdlRenderer = renderer();
  if (!sdlRenderer) {
    return;
  }
  char buf[1 << 8];
  sprintf(buf, "Stage:%3d", stage);
  TTF_Font* ttfFont = font();
  if (stageText) {
    stageText->setText(buf, sdlRenderer, ttfFont);
  } else {
    stageText = makeText(buf, sdlRenderer, ttfFont, WHITE);
  }
  for (int i = 0; i < playersCount; i++) {
    if (!scoresText[i]) {
      scoresText[i] = makeText("placeholder", sdlRenderer, ttfFont, WHITE);
    }
  }
  if (!taskText) {
    taskText = makeText("placeholder", sdlRenderer, ttfFont, WHITE);
  }
}
void initRenderer() {
  rendererSystem().reset();
  for (auto& list : animationsList) {
    list.clear();
  }
}
void clearInfo() {
  stageText.reset();
  taskText.reset();
  for (auto& score : scoresText) {
    score.reset();
  }
}
void clearRenderer() {
  SDL_Renderer* sdlRenderer = renderer();
  for (auto& list : animationsList) {
    list.clear();
  }
  if (sdlRenderer) {
    SDL_RenderClear(sdlRenderer);
  }
}
void renderText(const Text* text, int x, int y, double scale) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!text || !sdlRenderer) {
    return;
  }
  SDL_Rect dst = {x, y, static_cast<int>(text->width() * scale + 0.5),
                  static_cast<int>(text->height() * scale + 0.5)};
  SDL_RenderCopy(sdlRenderer, text->origin(), nullptr, &dst);
}
SDL_Point renderCenteredText(const Text* text, int x, int y, double scale) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!text || !sdlRenderer) {
    return SDL_Point{};
  }
  const int width = static_cast<int>(text->width() * scale + 0.5);
  const int height = static_cast<int>(text->height() * scale + 0.5);
  SDL_Rect dst = {x - width / 2, y - height / 2, width, height};
  SDL_RenderCopy(sdlRenderer, text->origin(), nullptr, &dst);
  return SDL_Point{x - width / 2, y - height / 2};
}
void unsetEffect(Texture* texture) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!texture || !sdlRenderer) {
    return;
  }
  SDL_SetTextureBlendMode(texture->origin(), SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(texture->origin(), 255, 255, 255);
  SDL_SetTextureAlphaMod(texture->origin(), 255);
}
void setEffect(Texture* texture, Effect* effect) {
  if (!texture || !effect) {
    return;
  }
  SDL_SetTextureBlendMode(texture->origin(), effect->mode());

  const double interval = static_cast<double>(effect->duration()) /
                          static_cast<double>(effect->length() - 1);
  double progress = effect->currentFrame();
  const int stage = static_cast<int>(progress / interval);
  progress -= stage * interval;
  progress /= interval;

  const SDL_Color prev = effect->keys()[stage];
  const SDL_Color next =
      effect->keys()[MIN(stage + 1, effect->length() - 1)];
  SDL_Color mixed{};
  mixed.a = static_cast<Uint8>(prev.a * (1 - progress) + next.a * progress);
  mixed.r = static_cast<Uint8>(prev.r * (1 - progress) + next.r * progress);
  mixed.g = static_cast<Uint8>(prev.g * (1 - progress) + next.g * progress);
  mixed.b = static_cast<Uint8>(prev.b * (1 - progress) + next.b * progress);

  SDL_SetTextureColorMod(texture->origin(), mixed.r, mixed.g, mixed.b);
  SDL_SetTextureAlphaMod(texture->origin(), mixed.a);
}
void updateAnimationOfSprite(const std::shared_ptr<Sprite>& self) {
  if (!self) {
    return;
  }
  const auto animation = self->animation();
  if (!animation) {
    return;
  }
  animation->setPosition(self->x(), self->y());
  animation->setFlip(self->face() == Direction::Right ? SDL_FLIP_NONE
                                                      : SDL_FLIP_HORIZONTAL);
}
void updateAnimationOfSnake(const std::shared_ptr<Snake>& snake) {
  if (!snake) {
    return;
  }
  for (const auto& sprite : snake->sprites()) {
    updateAnimationOfSprite(sprite);
  }
}
void updateAnimationOfBlock(Block* self) {
  if (!self) {
    return;
  }
  auto animation = self->animation();
  if (!animation) {
    return;
  }
  animation->setPosition(self->x(), self->y());
  if (self->type() == BlockType::Trap) {
    animation->setOrigin(&textures[self->enabled() ? RES_FLOOR_SPIKE_ENABLED
                                                   : RES_FLOOR_SPIKE_DISABLED]);
  } else if (self->type() == BlockType::Exit) {
    if (self->enabled() && animation->origin() != &textures[RES_FLOOR_EXIT]) {
      animation->setOrigin(&textures[RES_FLOOR_EXIT]);
      createAndPushAnimation(animationsList[RENDER_LIST_MAP_SPECIAL_ID],
                             &textures[RES_FLOOR_EXIT], &effects[EFFECT_BLINK],
                             LoopType::Infinite, 30, self->x(), self->y(),
                             SDL_FLIP_NONE, 0.0, At::TopLeft);
    }
  }
}
void clearBindInAnimationsList(const std::shared_ptr<Sprite>& sprite, int id) {
  if (!sprite) {
    return;
  }
  auto& list = animationsList[id];
  for (auto it = list.begin(); it != list.end();) {
    auto& animation = *it;
    if (animation && animation->bind() == sprite.get()) {
      animation->setBind(nullptr, false);
      if (animation->dieWithBind()) {
        it = list.erase(it);
        continue;
      }
    }
    ++it;
  }
}
void bindAnimationToSprite(const std::shared_ptr<Animation>& animation,
                           const std::shared_ptr<Sprite>& sprite,
                           bool isStrong) {
  if (!animation) {
    return;
  }
  animation->setBind(sprite.get(), isStrong);
  updateAnimationFromBind(animation.get());
}
void updateAnimationFromBind(Animation* animation) {
  if (!animation || !animation->bind()) {
    return;
  }
  const Sprite* sprite = animation->bind();
  animation->setPosition(sprite->x(), sprite->y());
  if (const auto spriteAnimation = sprite->animation()) {
    animation->setFlip(spriteAnimation->flip());
  }
}
void renderAnimation(const std::shared_ptr<Animation>& animation) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!animation || !sdlRenderer) {
    return;
  }
  updateAnimationFromBind(animation.get());
  int width = animation->origin()->width();
  int height = animation->origin()->height();
  SDL_Point poi = {animation->origin()->width(),
                   animation->origin()->height() / 2};
  if (animation->scaled()) {
    width *= SCALE_FACTOR;
    height *= SCALE_FACTOR;
  }
  SDL_Rect dst = {animation->x() - width / 2, animation->y() - height, width,
                  height};
  if (animation->at() == At::TopLeft) {
    dst.x = animation->x();
    dst.y = animation->y();
  } else if (animation->at() == At::Center) {
    dst.x = animation->x() - width / 2;
    dst.y = animation->y() - height / 2;
    poi.x = animation->origin()->width() / 2;
  } else if (animation->at() == At::BottomLeft) {
    dst.x = animation->x();
    dst.y = animation->y() + UNIT - height - 3;
  }
  if (animation->effect()) {
    setEffect(animation->origin(), animation->effect());
    animation->effect()->setCurrentFrame(
        animation->effect()->currentFrame() % animation->effect()->duration());
  }
#ifdef DBG
  assert(animation->duration() >= animation->origin()->frames());
#endif
  int stage = 0;
  if (animation->origin()->frames() > 1) {
    const double interval =
        static_cast<double>(animation->duration()) /
        static_cast<double>(animation->origin()->frames());
    stage = static_cast<int>(animation->currentFrame() / interval);
  }
  SDL_RenderCopyEx(sdlRenderer, animation->origin()->origin(),
                   &(animation->origin()->crops()[stage]), &dst,
                   animation->angle(), &poi, animation->flip());
  if (animation->effect()) {
    unsetEffect(animation->origin());
  }
}
void pushAnimationToRender(int id, const std::shared_ptr<Animation>& animation) {
  if (!animation) {
    return;
  }
  animationsList[id].push_back(animation);
}
std::shared_ptr<Animation> createAndPushAnimation(AnimationList& list,
                                                  Texture* texture,
                                                  const Effect* effect,
                                                  LoopType loopType,
                                                  int duration, int x, int y,
                                                  SDL_RendererFlip flip,
                                                  double angle, At at) {
  auto animation = std::make_shared<Animation>(texture, effect, loopType,
                                               duration, x, y, flip, angle, at);
  list.push_back(animation);
  return animation;
}
void updateAnimationLinkList(AnimationList& list) {
  for (auto it = list.begin(); it != list.end();) {
    auto& animation = *it;
    if (!animation) {
      it = list.erase(it);
      continue;
    }
    animation->setCurrentFrame(animation->currentFrame() + 1);
    animation->setLifeSpan(animation->lifeSpan() - 1);
    if (animation->effect()) {
      auto* effect = animation->effect();
      effect->setCurrentFrame(effect->currentFrame() + 1);
      effect->setCurrentFrame(effect->currentFrame() % effect->duration());
    }
    if (animation->loopType() == LoopType::Once) {
      if (animation->currentFrame() == animation->duration()) {
        it = list.erase(it);
        continue;
      }
    } else {
      if (animation->loopType() == LoopType::Lifespan &&
          animation->lifeSpan() == 0) {
        it = list.erase(it);
        continue;
      }
      animation->setCurrentFrame(animation->currentFrame() %
                                 animation->duration());
    }
    ++it;
  }
}
void renderAnimationLinkList(const AnimationList& list) {
  for (const auto& animation : list) {
    renderAnimation(animation);
  }
}
void renderAnimationLinkListWithSort(const AnimationList& list) {
  std::vector<std::shared_ptr<Animation>> buffer;
  buffer.reserve(list.size());
  for (const auto& animation : list) {
    buffer.push_back(animation);
  }
  std::sort(buffer.begin(), buffer.end(), [](const auto& a, const auto& b) {
    return a && b ? b->y() < a->y() : static_cast<bool>(a);
  });
  for (const auto& animation : buffer) {
    renderAnimation(animation);
  }
}
void renderSnakeHp(const std::shared_ptr<Snake>& snake) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!snake || !sdlRenderer) {
    return;
  }
  for (const auto& sprite : snake->sprites()) {
    if (!sprite || sprite->hp() == sprite->totalHp()) {
      continue;
    }
    double percent = static_cast<double>(sprite->hp()) / sprite->totalHp();
    for (int i = 0; percent > 1e-8; i++, percent -= 1) {
      int r = 0, g = 0, b = 0;
      if (i == 0) {
        if (percent < 1) {
          r = MIN((1 - percent) * 2 * 255, 255),
          g = MAX(0, 255 - (MAX(0.5 - percent, 0)) * 2 * 255);
        } else {
          g = 255;
        }
      } else {
        r = g = 0, b = 255;
      }
      SDL_SetRenderDrawColor(sdlRenderer, r, g, b, 255);
      const int width = RENDER_HP_BAR_WIDTH;
      const int spriteHeight =
          sprite->animation()->origin()->height() * SCALE_FACTOR;
      SDL_Rect bar = {sprite->x() - UNIT / 2 + (UNIT - width) / 2,
                      sprite->y() - spriteHeight - RENDER_HP_BAR_HEIGHT * (i + 1),
                      static_cast<int>(width * MIN(1, percent)),
                      RENDER_HP_BAR_HEIGHT};
      SDL_RenderDrawRect(sdlRenderer, &bar);
    }
  }
}
void renderHp() {
  for (int i = 0; i < spritesCount; i++) {
    renderSnakeHp(spriteSnake[i]);
  }
}
void renderCenteredTextBackground(const Text* text, int x, int y, double scale) {
  SDL_Renderer* sdlRenderer = renderer();
  if (!text || !sdlRenderer) {
    return;
  }
  const int width = static_cast<int>(text->width() * scale + 0.5);
  const int height = static_cast<int>(text->height() * scale + 0.5);
  SDL_Rect dst = {x - width / 2, y - height / 2, width, height};
  SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 200);
  SDL_RenderFillRect(sdlRenderer, &dst);
}
void renderId() {
  const int powerful = getPowerfulPlayer();
  for (int i = 0; i < playersCount; i++) {
    const auto& snake = spriteSnake[i];
    if (!snake || snake->sprites().empty()) {
      continue;
    }
    const auto snakeHead = snake->sprites().front();
    if (!snakeHead) {
      continue;
    }
    if (i == powerful) {
      renderCenteredTextBackground(&texts[4 + i], snakeHead->x(), snakeHead->y(),
                                   0.5);
    }
    renderCenteredText(&texts[4 + i], snakeHead->x(), snakeHead->y(), 0.5);
  }
}
void renderCountDown() {
  const double percent = static_cast<double>(renderFrames() %
                                             GAME_MAP_RELOAD_PERIOD) /
                         GAME_MAP_RELOAD_PERIOD;
  const int width = static_cast<int>(percent * UI_COUNTDOWN_BAR_WIDTH);
  if (countDownBar && countDownBar->origin()) {
    countDownBar->origin()->crops()[0].w = width;
    countDownBar->origin()->setWidth(width);
  }
}
void renderInfo() {
  SDL_Renderer* sdlRenderer = renderer();
  if (!sdlRenderer) {
    return;
  }
  int startY = 0;
  const int startX = 10;
  const int lineGap = FONT_SIZE;
  if (stageText) {
    renderText(stageText.get(), startX, startY, 1);
  }
  startY += lineGap;
  for (int i = 0; i < playersCount; i++) {
    char buf[1 << 8];
    const auto& snake = spriteSnake[i];
    if (!snake) {
      continue;
    }
    snake->score()->calcScore(gameLevel);
    sprintf(buf, "Player%d:%5d", i + 1,
            static_cast<int>(snake->score()->rank() + 0.5));
    if (scoresText[i]) {
      scoresText[i]->setText(buf, sdlRenderer, font());
      renderText(scoresText[i].get(), startX, startY, 1);
    }
    startY += lineGap;
  }
  if (playersCount == 1) {
    extern int GAME_WIN_NUM;
    char buf[1 << 8];
    const int remaining = spriteSnake[0]
                              ? GAME_WIN_NUM - spriteSnake[0]->num()
                              : GAME_WIN_NUM;
    sprintf(buf, "Find %d more heros!", remaining > 0 ? remaining : 0);
    if (taskText) {
      taskText->setText(buf, sdlRenderer, font());
      renderText(taskText.get(), startX, startY, 1);
    }
    startY += lineGap;
  }
}
void render() {
  SDL_Renderer* sdlRenderer = renderer();
  if (!sdlRenderer) {
    return;
  }
  SDL_SetRenderDrawColor(sdlRenderer, 25, 17, 23, 255);
  SDL_RenderClear(sdlRenderer);

  for (int i = 0; i < ANIMATION_LINK_LIST_NUM; i++) {
    updateAnimationLinkList(animationsList[i]);
    if (i == RENDER_LIST_SPRITE_ID) {
      renderAnimationLinkListWithSort(animationsList[i]);
    } else {
      renderAnimationLinkList(animationsList[i]);
    }
  }
  renderHp();
  renderCountDown();
  renderInfo();
  renderId();
  // Update Screen
  SDL_RenderPresent(sdlRenderer);
  incrementRenderFrames();
}
void renderUi() {
  SDL_Renderer* sdlRenderer = renderer();
  if (!sdlRenderer) {
    return;
  }
  SDL_SetRenderDrawColor(sdlRenderer, RENDER_BG_COLOR, 255);
  SDL_RenderClear(sdlRenderer);

  for (int i = 0; i < ANIMATION_LINK_LIST_NUM; i++) {
    updateAnimationLinkList(animationsList[i]);
    if (i == RENDER_LIST_SPRITE_ID) {
      renderAnimationLinkListWithSort(animationsList[i]);
    } else {
      renderAnimationLinkList(animationsList[i]);
    }
  }
}
