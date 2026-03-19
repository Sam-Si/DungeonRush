#include "helper.h"

#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "prng.h"
#include "res.h"
#include "types.h"

extern const int SCALE_FACTOR;
extern Texture textures[];
bool inr(int x, int l, int r) { return x <= r && l <= x; }
int randInt(int l, int r) { return PRNG::rand() % (r - l + 1) + l; }
double randDouble() { return static_cast<double>(PRNG::rand()) / PRNG::MAX; }
int IntervalCalc(int l1, int r1, int l2, int r2) {
  return MAX(-MAX(l1, l2) + MIN(r1, r2), 0);
}
int RectRectCalc(const SDL_Rect* a, const SDL_Rect* b) {
  return IntervalCalc(a->x, a->x + a->w, b->x, b->x + b->w) *
         IntervalCalc(a->y, a->y + a->h, b->y, b->y + b->h);
}
bool IntervalCross(int l1, int r1, int l2, int r2) {
  return MAX(l1, l2) < MIN(r1, r2);
}
bool RectRectCross(const SDL_Rect* a, const SDL_Rect* b) {
  return RectRectCalc(a, b) >= HELPER_RECT_CROSS_LIMIT;
  /*
  return IntervalCross(a->x, a->x + a->w, b->x, b->x + b->w) &&
         IntervalCross(a->y, a->y + a->h, b->y, b->y + b->h);
         */
}
bool RectCirCross(const SDL_Rect* a, int x, int y, int r) {
  if (inr(x, a->x, a->x + a->w) && inr(y, a->y, a->y + a->h)) {
    return true;
  }
  if (abs(x - a->x) <= r && inr(y, a->y, a->y + a->h)) {
    return true;
  }
  if (abs(x - a->x - a->w) <= r && inr(y, a->y, a->y + a->h)) {
    return true;
  }
  if (abs(y - a->y) <= r && inr(x, a->x, a->x + a->w)) {
    return true;
  }
  if (abs(y - a->y - a->h) <= r && inr(x, a->x, a->x + a->w)) {
    return true;
  }
  return false;
}
SDL_Rect getSpriteAnimationBox(const Sprite* sprite) {
  if (!sprite) {
    return {0, 0, 0, 0};
  }
  const auto animation = sprite->animation();
  if (!animation || !animation->origin()) {
    return {0, 0, 0, 0};
  }
  SDL_Rect dst = {animation->x() - animation->origin()->width() * SCALE_FACTOR / 2,
                  animation->y() - animation->origin()->height() * SCALE_FACTOR,
                  animation->origin()->width() * SCALE_FACTOR,
                  animation->origin()->height() * SCALE_FACTOR};
  return dst;
}
SDL_Rect getSpriteBoundBox(const Sprite* sprite) {
  if (!sprite) {
    return {0, 0, 0, 0};
  }
  const auto animation = sprite->animation();
  if (!animation || !animation->origin()) {
    return {0, 0, 0, 0};
  }
  SDL_Rect dst = {animation->x() - animation->origin()->width() * SCALE_FACTOR / 2,
                  animation->y() - animation->origin()->height() * SCALE_FACTOR,
                  animation->origin()->width() * SCALE_FACTOR,
                  animation->origin()->height() * SCALE_FACTOR};
  bool big = false;
  if (animation->origin() == &textures[RES_BIG_DEMON]) {
    big = true;
  } else if (animation->origin() == &textures[RES_BIG_ZOMBIE]) {
    big = true;
  } else if (animation->origin() == &textures[RES_ORGRE]) {
    big = true;
  }
  if (big) {
    dst.w -= BIG_SPRITE_EFFECT_DELTA;
    dst.x += BIG_SPRITE_EFFECT_DELTA / 2;
    dst.y += SPRITE_EFFECT_VERTICAL_DELTA;
    dst.h -= SPRITE_EFFECT_VERTICAL_DELTA;
  } else {
    dst.w -= SPRITE_EFFECT_DELTA;
    dst.x += SPRITE_EFFECT_DELTA / 2;
    dst.y += SPRITE_EFFECT_VERTICAL_DELTA;
    dst.h -= SPRITE_EFFECT_VERTICAL_DELTA;
  }
  return dst;
}
SDL_Rect getSpriteFeetBox(const Sprite* sprite) {
  if (!sprite) {
    return {0, 0, 0, 0};
  }
  const auto animation = sprite->animation();
  if (!animation) {
    return {0, 0, 0, 0};
  }
  SDL_Rect dst = getSpriteBoundBox(sprite);
  dst.y = animation->y() - SPRITE_EFFECT_FEET;
  dst.h = SPRITE_EFFECT_FEET;
  return dst;
}
SDL_Rect getMapRect(int x, int y) {
  SDL_Rect ret = {x * UNIT, y * UNIT, UNIT, UNIT};
  return ret;
}
double distance(Point a, Point b) {
  const double dx = static_cast<double>(a.x - b.x);
  const double dy = static_cast<double>(a.y - b.y);
  return sqrt(dx * dx + dy * dy);
}
