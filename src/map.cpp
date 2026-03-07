#include "map.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <array>

#include "helper.h"
#include "render.h"
#include "res.h"
#include "types.h"
#ifdef DBG
#include <assert.h>
#endif
namespace {
constexpr double kMapHowOld = 0.05;
constexpr double kMapWallHowDecorated = 0.1;
}
extern const int n, m;
extern Texture textures[];
extern std::array<std::array<Block, MAP_SIZE>, MAP_SIZE> map;
extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;


bool isTrap[MAP_SIZE][MAP_SIZE];
bool primMap[MAP_SIZE][MAP_SIZE];
bool hasMap[MAP_SIZE][MAP_SIZE];
int exitX, exitY;
void clearMapGenerator() {
  exitX = exitY = -1;
  memset(hasMap, 0, sizeof(hasMap));
  memset(isTrap, 0, sizeof(isTrap));
  memset(primMap, 0, sizeof(primMap));
}
int count(int x, int y) {
  int ret = 0;
  for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++) {
      if (dx || dy) {
        int xx = x + dx, yy = y + dy;
        if (inr(xx, 0, n - 1) && inr(yy, 0, m - 1)) {
          ret += primMap[xx][yy];
        }
      }
    }
  return ret;
}
void cellularAutomata() {
  static bool tmp[MAP_SIZE][MAP_SIZE];
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) {
      int c = count(i, j);
      if (c <= 3)
        tmp[i][j] = 0;
      else if (c >= 6)
        tmp[i][j] = 1;
      else
        tmp[i][j] = primMap[i][j];
    }
  memcpy(primMap, tmp, sizeof tmp);
}
#ifdef DBG
void printMap() {
  putchar('\n');
  for (int i = 0; i < n; i++) {
    for (int t = 0; t < 2; t++) {
      for (int j = 0; j < m; j++) {
        char ch = primMap[i][j] ? '#' : '.';
        printf("%c%c", ch, ch);
      }
      putchar('\n');
    }
  }
}
void phasMap() {
  for (int i = 0; i < SCREEN_WIDTH / UNIT; i++) {
    for (int j = 0; j < SCREEN_HEIGHT / UNIT; j++) {
      char ch = hasMap[i][j] ? '#' : '.';
      putchar(ch);
    }
    putchar('\n');
  }
}
#endif
void initPrimMap(double floorPercent, int smoothTimes) {
  memset(primMap, 0, sizeof(primMap));
  int nn = SCREEN_WIDTH / UNIT, mm = SCREEN_HEIGHT / UNIT;
  int halfN = nn / 2;
  int halfM = mm / 2;
  int floors = halfN * halfM * floorPercent;
  for (int i = 0; i < floors; i++) {
    int x, y;
    do {
      x = randInt(0, halfN - 1), y = randInt(0, halfM - 1);
    } while (primMap[x][y]);
    primMap[x][y] = 1;
  }
  int ltx = halfN / 4, lty = halfM / 4;
  int w = halfN / 2, h = halfM / 2;
  for (int i = ltx; i < ltx + w; i++)
    for (int j = lty; j < lty + h; j++) primMap[i][j] = 1;
  for (int i = 0; i < halfN; i++) primMap[i][0] = primMap[i][halfM - 1] = 0;
  for (int j = 0; j < halfM; j++) primMap[0][j] = primMap[halfN - 1][j] = 0;
  while (smoothTimes--) {
    cellularAutomata();
#ifdef DBG
// printMap(n, m);
#endif
  }
}
void initBlock(Block* self, BlockType type, int x, int y, int bid,
               bool enable) {
  if (!self) {
    return;
  }
  self->setPosition(x, y);
  self->setType(type);
  self->setId(bid);
  self->setEnabled(enable);
  std::shared_ptr<Animation> animation;
  if (type == BlockType::Trap) {
    animation = createAndPushAnimation(
        animationsList[RENDER_LIST_MAP_ID],
        &textures[enable ? RES_FLOOR_SPIKE_ENABLED : RES_FLOOR_SPIKE_DISABLED],
        nullptr, LoopType::Infinite, 1, x, y, SDL_FLIP_NONE, 0, At::TopLeft);
  } else if (type == BlockType::Exit) {
    animation = createAndPushAnimation(
        animationsList[RENDER_LIST_MAP_ID],
        &textures[enable ? RES_FLOOR_EXIT : RES_FLOOR_2], nullptr,
        LoopType::Infinite, 1, x, y, SDL_FLIP_NONE, 0, At::TopLeft);
  } else {
    animation = createAndPushAnimation(
        animationsList[RENDER_LIST_MAP_ID], &textures[bid], nullptr,
        LoopType::Infinite, 1, x, y, SDL_FLIP_NONE, 0, At::TopLeft);
  }
  self->setAnimation(animation);
}
void initMap() {
  // this will actually generate the map for game
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) {
      if ((i != exitX || j != exitY) && hasMap[i][j]) {
        if (isTrap[i][j])
          initBlock(&map[i][j], BlockType::Trap, i * UNIT, j * UNIT,
                    RES_FLOOR_SPIKE_DISABLED,
                    false);  // textureID does not matter
        else
          initBlock(&map[i][j], BlockType::Floor, i * UNIT, j * UNIT, RES_FLOOR_1,
                    true);
      }
    }
}
void decorateMap() {
  // this will add details to boring floors
  for (int i = 0, lim = n * m * kMapHowOld, x, y; i < lim; i++) {
    do {
      x = randInt(0, n - 2), y = randInt(0, m - 2);
    } while ((hasMap[x][y] && !isTrap[x][y]) +
                 (hasMap[x + 1][y] && !isTrap[x + 1][y]) +
                 (hasMap[x][y + 1] && !isTrap[x][y + 1]) +
                 (hasMap[x + 1][y + 1] && !isTrap[x + 1][y + 1]) <
             4);
    if (randDouble() < kMapHowOld) {
      if (auto animation = map[x][y].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_6]);
      }
      if (auto animation = map[x + 1][y].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_4]);
      }
      if (auto animation = map[x + 1][y + 1].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_8]);
      }
      if (auto animation = map[x][y + 1].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_7]);
      }
    } else {
      if (auto animation = map[x][y].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_2]);
      }
      if (auto animation = map[x][y + 1].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_5]);
      }
      if (auto animation = map[x + 1][y].animation()) {
        animation->setOrigin(&textures[RES_FLOOR_3]);
      }
    }
  }
}
void initBlankMap(int w, int h) {
  clearMapGenerator();
  int si = n/2 - w/2, sj = m/2 - h/2;
  for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) {
    int ii = si + i, jj = sj + j;
    hasMap[ii][jj] = 1;
    initBlock(&map[ii][jj], BlockType::Floor, ii * UNIT, jj * UNIT, RES_FLOOR_1,
              false);
  }
}
void initRandomMap(double floorPercent, int smoothTimes, double trapRate) {
  clearMapGenerator();
  initPrimMap(floorPercent, smoothTimes);
  // this will create a good-looking cave map in primMap[][]
  int halfN = n / 2, halfM = m / 2;
  for (int i = 0; i < halfN; i++) {
    for (int j = 0; j < halfM; j++) {
      if (primMap[i][j]) {
        hasMap[i * 2][j * 2] = hasMap[i * 2 + 1][j * 2] =
            hasMap[i * 2][j * 2 + 1] =
                hasMap[2 * i + 1][2 * j + 1] = 1;
      }
    }
  }
#ifdef DBG
  phasMap();
#endif
  for (int t = n * m * trapRate; t > 0; t--) {
    int x, y;
    do {
      x = randInt(0, n - 2), y = randInt(0, m - 2);
    } while ((hasMap[x][y] + hasMap[x + 1][y] + hasMap[x][y + 1] +
              hasMap[x + 1][y + 1]) <= 1);
    isTrap[x][y] = 1;
    if (hasMap[x + 1][y]) isTrap[x + 1][y] = 1;
    if (hasMap[x][y + 1]) isTrap[x][y + 1] = 1;
    if (hasMap[x + 1][y + 1]) isTrap[x + 1][y + 1] = 1;
  }
  do {
    exitX = randInt(0, n - 1), exitY = randInt(0, m - 1);
  } while (!(hasMap[exitX][exitY] && !isTrap[exitX][exitY]));
  initBlock(&map[exitX][exitY], BlockType::Exit, exitX * UNIT, exitY * UNIT,
            RES_FLOOR_EXIT, false);
#ifdef DBG
  printf("exit: %d %d\n", exitX, exitY);
#endif
  initMap();
  decorateMap();
}
void pushMapToRender() {
  const double mapWallDecorated = kMapWallHowDecorated;
  const double mapHowOld = kMapHowOld;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      if (!hasMap[i][j]) {
        if (inr(j + 1, 0, m - 1) && hasMap[i][j + 1]) {
          if (inr(i + 1, 0, n - 1) && hasMap[i + 1][j]) {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_CORNER_FRONT_RIGHT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, j * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_CORNER_BOTTOM_RIGHT],
                                   nullptr, LoopType::Infinite, 1, i * UNIT,
                                   (j - 1) * UNIT, SDL_FLIP_NONE, 0, At::TopLeft);
          } else if (inr(i - 1, 0, n - 1) && hasMap[i - 1][j]) {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_CORNER_FRONT_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, j * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_CORNER_BOTTOM_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          } else {
            int bid = randDouble() < mapHowOld * 5
                          ? (RES_WALL_HOLE_1 + randInt(0, 1))
                          : RES_WALL_MID;
            if (randDouble() < mapWallDecorated) {
              bid = RES_WALL_BANNER_RED + randInt(0, 3);
            }
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[bid], nullptr, LoopType::Infinite, 1, i * UNIT,
                                   j * UNIT, SDL_FLIP_NONE, 0, At::TopLeft);
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_TOP_MID], nullptr,
                                   LoopType::Infinite, 1, i * UNIT,
                                   (j - 1) * UNIT, SDL_FLIP_NONE, 0,
                                   At::TopLeft);
          }
        }
        if (inr(j - 1, 0, m - 1) && hasMap[i][j - 1]) {
          int bid = randDouble() < mapHowOld * 2
                        ? (RES_WALL_HOLE_1 + randInt(0, 1))
                        : RES_WALL_MID;
          createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                 &textures[bid], nullptr, LoopType::Infinite, 1, i * UNIT,
                                 j * UNIT, SDL_FLIP_NONE, 0, At::TopLeft);
          if (hasMap[i - 1][j]) {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_FOREWALL],
                                   &textures[RES_WALL_CORNER_TOP_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          } else if (hasMap[i + 1][j]) {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_FOREWALL],
                                   &textures[RES_WALL_CORNER_TOP_RIGHT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          } else {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_FOREWALL],
                                   &textures[RES_WALL_TOP_MID], nullptr,
                                   LoopType::Infinite, 1, i * UNIT,
                                   (j - 1) * UNIT, SDL_FLIP_NONE, 0,
                                   At::TopLeft);
          }
        }
        if (inr(i + 1, 0, n - 1) && hasMap[i + 1][j]) {
          if (inr(j + 1, 0, m - 1) && hasMap[i][j + 1])
            ;  // just do not render
          else
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_MID_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, j * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          if (!hasMap[i + 1][j + 1])
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_FRONT_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j + 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          if (!hasMap[i + 1][j - 1]) {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_MID_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_TOP_LEFT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 2) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          }
        }
        if (inr(i - 1, 0, n - 1) && hasMap[i - 1][j]) {
          if (inr(j + 1, 0, m - 1) && hasMap[i][j + 1])
            ;  // do not render
          else
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_MID_RIGHT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, j * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          if (!hasMap[i - 1][j + 1])
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_FRONT_RIGHT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j + 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          if (!hasMap[i - 1][j - 1]) {
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_MID_RIGHT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 1) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
            createAndPushAnimation(animationsList[RENDER_LIST_MAP_ID],
                                   &textures[RES_WALL_SIDE_TOP_RIGHT], nullptr,
                                   LoopType::Infinite, 1, i * UNIT, (j - 2) * UNIT,
                                   SDL_FLIP_NONE, 0, At::TopLeft);
          }
        }
      }
    }
  }
}
