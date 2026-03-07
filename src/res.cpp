#include "res.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_net.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "audio.h"
#include "render.h"
#include "sprite.h"
#include "types.h"
#include "weapon.h"

// Constants
extern const int n = SCREEN_WIDTH / UNIT;
extern const int m = SCREEN_HEIGHT / UNIT;

ResourceManager::~ResourceManager() = default;

SDL_Window* ResourceManager::window() const { return window_.get(); }

TTF_Font* ResourceManager::font() const { return font_.get(); }

const std::vector<std::shared_ptr<SDL_Texture>>&
ResourceManager::tilesetTextures() const {
  return tilesetTextures_;
}

void ResourceManager::resetWindow(SDL_Window* window) { window_.reset(window); }

void ResourceManager::resetFont(TTF_Font* font) { font_.reset(font); }

void ResourceManager::clearTilesetTextures() { tilesetTextures_.clear(); }

void ResourceManager::addTilesetTexture(SDL_Texture* texture) {
  tilesetTextures_.push_back(
      std::shared_ptr<SDL_Texture>(texture, SDL_DestroyTexture));
}

RendererSystem::~RendererSystem() = default;

SDL_Renderer* RendererSystem::renderer() const { return renderer_.get(); }

void RendererSystem::setRenderer(SDL_Renderer* renderer) {
  renderer_.reset(renderer);
}

void RendererSystem::reset() { frames_ = 0; }

unsigned long long RendererSystem::frames() const { return frames_; }

void RendererSystem::incrementFrames() { ++frames_; }

ResourceManager& resourceManager() {
  static ResourceManager instance;
  return instance;
}

RendererSystem& rendererSystem() {
  static RendererSystem instance;
  return instance;
}

AudioSystem& audioSystem() {
  return AudioSystem::instance();
}

SDL_Renderer* renderer() {
  return rendererSystem().renderer();
}

TTF_Font* font() {
  return resourceManager().font();
}

int bgmCount() {
  return AUDIO_BGM_SIZE;
}

unsigned long long renderFrames() {
  return rendererSystem().frames();
}

void incrementRenderFrames() {
  rendererSystem().incrementFrames();
}

static char resourceBasePath[PATH_LEN] = "./";

const char tilesetPath[TILESET_SIZE][PATH_LEN] = {
    "drawable/0x72_DungeonTilesetII_v1_3",
    "drawable/fireball_explosion1",
    "drawable/halo_explosion1",
    "drawable/halo_explosion2",
    "drawable/fireball",
    "drawable/floor_spike",
    "drawable/floor_exit",
    "drawable/HpMed",
    "drawable/SwordFx",
    "drawable/ClawFx",
    "drawable/Shine",
    "drawable/Thunder",
    "drawable/BloodBound",
    "drawable/arrow",
    "drawable/explosion-2",
    "drawable/ClawFx2",
    "drawable/Axe",
    "drawable/cross_hit",
    "drawable/blood",
    "drawable/SolidFx",
    "drawable/IcePick",
    "drawable/IceShatter",
    "drawable/Ice",
    "drawable/SwordPack",
    "drawable/HolyShield",
    "drawable/golden_cross_hit",
    "drawable/ui",
    "drawable/title",
    "drawable/purple_ball",
    "drawable/purple_exp",
    "drawable/staff",
    "drawable/Thunder_Yellow",
    "drawable/attack_up",
    "drawable/powerful_bow"};
const char fontPath[] = "font/m5x7.ttf";
const char textsetPath[] = "text.txt";

const char bgmsPath[AUDIO_BGM_SIZE][PATH_LEN] = {
  "audio/main_title.ogg",
  "audio/bg1.ogg",
  "audio/bg2.ogg",
  "audio/bg3.ogg"
};
const char soundsPath[PATH_LEN] = "audio/sounds";
const char soundsPathPrefix[PATH_LEN] = "audio/";
// Gloabls
int texturesCount;
Texture textures[TEXTURES_SIZE];
int textsCount;
Text texts[TEXTSET_SIZE];

extern SDL_Color BLACK;
extern SDL_Color WHITE;
extern Weapon weapons[WEAPONS_SIZE];
extern std::array<AnimationList, ANIMATION_LINK_LIST_NUM> animationsList;

Effect effects[EFFECTS_SIZE];

Sprite commonSprites[COMMON_SPRITE_SIZE];

void setResourceBasePath(const char* path) {
  snprintf(resourceBasePath, sizeof(resourceBasePath), "%s", path);
}

static void joinResourcePath(char* out, size_t outSize, const char* relativePath) {
  snprintf(out, outSize, "%s/%s", resourceBasePath, relativePath);
}

bool init() {
  bool success = true;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  SDL_Window* window = SDL_CreateWindow("Dungeon Rush " VERSION_STRING,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                        SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  resourceManager().resetWindow(window);

#ifndef SOFTWARE_ACC
  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif
#ifdef SOFTWARE_ACC
  printf("define software acc\n");
  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
#endif
  if (!sdlRenderer) {
    printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
    return false;
  }
  rendererSystem().setRenderer(sdlRenderer);
  SDL_SetRenderDrawColor(sdlRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

  const int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    printf("SDL_image could not initialize! SDL_image Error: %s\n",
           IMG_GetError());
    success = false;
  }
  if (TTF_Init() == -1) {
    printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
    success = false;
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
           Mix_GetError());
    success = false;
  }
  if (SDLNet_Init() == -1) {
    printf("SDL_Net_Init: %s\n", SDLNet_GetError());
    success = false;
  }

  return success;
}
SDL_Texture* loadSDLTexture(const char* path) {
  // The final texture
  SDL_Renderer* sdlRenderer = renderer();
  if (!sdlRenderer) {
    return nullptr;
  }
  SDL_Texture* newTexture = nullptr;
  char fullPath[PATH_LEN * 2];
  joinResourcePath(fullPath, sizeof(fullPath), path);

  SDL_Surface* loadedSurface = IMG_Load(fullPath);
  if (loadedSurface == nullptr) {
    printf("Unable to load image %s! SDL_image Error: %s\n", fullPath,
           IMG_GetError());
  } else {
    newTexture = SDL_CreateTextureFromSurface(sdlRenderer, loadedSurface);
    if (newTexture == nullptr) {
      printf("Unable to create texture from %s! SDL Error: %s\n", fullPath,
             SDL_GetError());
    }

    SDL_FreeSurface(loadedSurface);
  }

  return newTexture;
}
bool loadTextset() {
  bool success = true;
  char fullPath[PATH_LEN * 2];
  joinResourcePath(fullPath, sizeof(fullPath), textsetPath);
  FILE* file = fopen(fullPath, "r");
  if (!file) {
    printf("Failed to open textset %s\n", fullPath);
    return false;
  }
  char str[TEXT_LEN];
  SDL_Renderer* sdlRenderer = renderer();
  TTF_Font* ttfFont = font();
  while (fgets(str, TEXT_LEN, file)) {
    int len = strlen(str);
    while (len - 1 >= 0 && !isprint(str[len - 1])) {
      str[--len] = 0;
    }
    if (!len) {
      continue;
    }
    if (!sdlRenderer || !ttfFont) {
      success = false;
      continue;
    }
    texts[textsCount++] = Text(str, WHITE, sdlRenderer, ttfFont);
#ifdef DBG
    printf("Texts #%d: %s loaded\n", textsCount - 1, str);
#endif
  }
  fclose(file);
  return success;
}
bool loadTileset(const char* path, SDL_Texture* origin) {
  char fullPath[PATH_LEN * 2];
  joinResourcePath(fullPath, sizeof(fullPath), path);
  FILE* file = fopen(fullPath, "r");
  if (!file) {
    printf("Failed to open tileset %s\n", fullPath);
    return false;
  }
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int f = 0;
  char resName[256];
  while (fscanf(file, "%s %d %d %d %d %d", resName, &x, &y, &w, &h, &f) == 6) {
    Texture* p = &textures[texturesCount++];
    *p = Texture(origin, w, h, f);
    for (int i = 0; i < f; i++) {
      p->crops()[i].x = x + i * w;
      p->crops()[i].y = y;
      p->crops()[i].h = h;
      p->crops()[i].w = w;
    }
#ifdef DBG
    printf("Resources #%d: %s %d %d %d %d %d loaded\n", texturesCount - 1,
           resName, x, y, w, h, f);
#endif
  }
  fclose(file);
  return true;
}
bool loadAudio() {
  bool success = true;
  char fullPath[PATH_LEN * 2];
  AudioSystem& audio = audioSystem();
  audio.clear();

  for (int i = 0; i < AUDIO_BGM_SIZE; i++) {
    if (bgmsPath[i][0] == '\0') {
      break;
    }
    joinResourcePath(fullPath, sizeof(fullPath), bgmsPath[i]);
    std::unique_ptr<Mix_Music, MixMusicDeleter> music(Mix_LoadMUS(fullPath));
    success &= static_cast<bool>(music);
    if (!music) {
      printf("Failed to load %s: SDL_mixer Error: %s\n", fullPath,
             Mix_GetError());
    }
#ifdef DBG
    else {
      printf("BGM %s loaded\n", fullPath);
    }
#endif
    audio.setBgm(i, std::move(music));
  }

  joinResourcePath(fullPath, sizeof(fullPath), soundsPath);
  FILE* f = fopen(fullPath, "r");
  if (!f) {
    printf("Failed to open sound list %s\n", fullPath);
    return false;
  }
  char buf[PATH_LEN];
  char path[PATH_LEN << 1];
  while (~fscanf(f, "%s", buf)) {
    snprintf(path, sizeof(path), "%s/%s", soundsPathPrefix, buf);
    joinResourcePath(fullPath, sizeof(fullPath), path);
    std::unique_ptr<Mix_Chunk, MixChunkDeleter> chunk(Mix_LoadWAV(fullPath));
    success &= static_cast<bool>(chunk);
    if (!chunk) {
      printf("Failed to load %s: SDL_mixer Error: %s\n", fullPath,
             Mix_GetError());
    }
#ifdef DBG
    else {
      printf("Sound #%d: %s\n", audio.soundsCount(), fullPath);
    }
#endif
    audio.addSound(std::move(chunk));
  }
  fclose(f);
  return success;
}
bool loadMedia() {
  // Loading success flag
  bool success = true;
  // load effects
  initCommonEffects();
  // Load tileset
  char imgPath[PATH_LEN + 4];
  resourceManager().clearTilesetTextures();
  for (int i = 0; i < TILESET_SIZE; i++) {
    if (!strlen(tilesetPath[i])) {
      break;
    }
    sprintf(imgPath, "%s.png", tilesetPath[i]);
    SDL_Texture* texture = loadSDLTexture(imgPath);
    success &= loadTileset(tilesetPath[i], texture);
    success &= texture != nullptr;
    if (texture) {
      resourceManager().addTilesetTexture(texture);
    }
  }
  // Open the font
  char fontFullPath[PATH_LEN * 2];
  joinResourcePath(fontFullPath, sizeof(fontFullPath), fontPath);
  TTF_Font* font = TTF_OpenFont(fontFullPath, FONT_SIZE);
  resourceManager().resetFont(font);
  if (!font) {
    printf("Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
    success = false;
  } else {
    if (!loadTextset()) {
      printf("Failed to load textset!\n");
      success = false;
    }
  }
  // Init common sprites
  initWeapons();
  initCommonSprites();

  if (!loadAudio()) {
    printf("Failed to load audio!\n");
    success = false;
  }

  return success;
}
void cleanup() {
  resourceManager().clearTilesetTextures();
  audioSystem().clear();
  rendererSystem().setRenderer(nullptr);
  resourceManager().resetWindow(nullptr);
  resourceManager().resetFont(nullptr);

  // Quit SDL subsystems
  TTF_Quit();
  IMG_Quit();
  Mix_CloseAudio();
  SDLNet_Quit();
  SDL_Quit();
}
void initCommonEffects() {
  // Effect #0: Death
  effects[0] = Effect(30, 4, SDL_BLENDMODE_BLEND);
  SDL_Color death = {255, 255, 255, 255};
  effects[0].keys()[0] = death;
  death.g = death.b = 0;
  death.r = 168;
  effects[0].keys()[1] = death;
  death.r = 80;
  effects[0].keys()[2] = death;
  death.r = death.a = 0;
  effects[0].keys()[3] = death;
#ifdef DBG
  puts("Effect #0: Death loaded");
#endif

  // Effect #1: Blink ( white )
  effects[1] = Effect(30, 3, SDL_BLENDMODE_ADD);
  SDL_Color blink = {0, 0, 0, 255};
  effects[1].keys()[0] = blink;
  blink.r = blink.g = blink.b = 200;
  effects[1].keys()[1] = blink;
  blink.r = blink.g = blink.b = 0;
  effects[1].keys()[2] = blink;
#ifdef DBG
  puts("Effect #1: Blink (white) loaded");
#endif
  effects[2] = Effect(30, 2, SDL_BLENDMODE_BLEND);
  SDL_Color vanish = {255, 255, 255, 255};
  effects[2].keys()[0] = vanish;
  vanish.a = 0;
  effects[2].keys()[1] = vanish;
#ifdef DBG
  puts("Effect #2: Vanish (30fm) loaded");
#endif
}
void initCommonSprite(Sprite* sprite, Weapon* weapon, int res_id, int hp) {
  if (!sprite) {
    return;
  }
  auto animation = createAndPushAnimation(
      animationsList[RENDER_LIST_SPRITE_ID], &textures[res_id], nullptr,
      LoopType::Infinite, SPRITE_ANIMATION_DURATION, 0, 0, SDL_FLIP_NONE, 0,
      At::BottomCenter);
  *sprite = Sprite();
  sprite->setHp(hp);
  sprite->setTotalHp(hp);
  sprite->setWeapon(weapon);
  sprite->setAnimation(animation);
  sprite->setFace(Direction::Right);
  sprite->setDirection(Direction::Right);
  sprite->setLastAttack(0);
  sprite->setDropRate(1);
}
void initCommonSprites() {
  initCommonSprite(&commonSprites[SPRITE_KNIGHT], &weapons[WEAPON_SWORD],
                   RES_KNIGHT_M, 150);
  initCommonSprite(&commonSprites[SPRITE_ELF], &weapons[WEAPON_ARROW], RES_ELF_M,
                   100);
  initCommonSprite(&commonSprites[SPRITE_WIZZARD], &weapons[WEAPON_FIREBALL],
                   RES_WIZZARD_M, 95);
  initCommonSprite(&commonSprites[SPRITE_LIZARD], &weapons[WEAPON_MONSTER_CLAW],
                   RES_LIZARD_M, 120);
  initCommonSprite(&commonSprites[SPRITE_TINY_ZOMBIE],
                   &weapons[WEAPON_MONSTER_CLAW2], RES_TINY_ZOMBIE, 50);
  initCommonSprite(&commonSprites[SPRITE_GOBLIN], &weapons[WEAPON_MONSTER_CLAW2],
                   RES_GOBLIN, 100);
  initCommonSprite(&commonSprites[SPRITE_IMP], &weapons[WEAPON_MONSTER_CLAW2],
                   RES_IMP, 100);
  initCommonSprite(&commonSprites[SPRITE_SKELET], &weapons[WEAPON_MONSTER_CLAW2],
                   RES_SKELET, 100);
  initCommonSprite(&commonSprites[SPRITE_MUDDY], &weapons[WEAPON_SOLID], RES_MUDDY,
                   150);
  initCommonSprite(&commonSprites[SPRITE_SWAMPY], &weapons[WEAPON_SOLID_GREEN],
                   RES_SWAMPY, 150);
  initCommonSprite(&commonSprites[SPRITE_ZOMBIE], &weapons[WEAPON_MONSTER_CLAW2],
                   RES_ZOMBIE, 120);
  initCommonSprite(&commonSprites[SPRITE_ICE_ZOMBIE], &weapons[WEAPON_ICEPICK],
                   RES_ICE_ZOMBIE, 120);
  initCommonSprite(&commonSprites[SPRITE_MASKED_ORC], &weapons[WEAPON_THROW_AXE],
                   RES_MASKED_ORC, 120);
  initCommonSprite(&commonSprites[SPRITE_ORC_WARRIOR],
                   &weapons[WEAPON_MONSTER_CLAW2], RES_ORC_WARRIOR, 200);
  initCommonSprite(&commonSprites[SPRITE_ORC_SHAMAN],
                   &weapons[WEAPON_MONSTER_CLAW2], RES_ORC_SHAMAN, 120);
  initCommonSprite(&commonSprites[SPRITE_NECROMANCER],
                   &weapons[WEAPON_PURPLE_BALL], RES_NECROMANCER, 120);
  initCommonSprite(&commonSprites[SPRITE_WOGOL], &weapons[WEAPON_MONSTER_CLAW2],
                   RES_WOGOL, 150);
  initCommonSprite(&commonSprites[SPRITE_CHROT], &weapons[WEAPON_MONSTER_CLAW2],
                   RES_CHORT, 150);
  Sprite* now = nullptr;
  initCommonSprite(now = &commonSprites[SPRITE_BIG_ZOMBIE],
                   &weapons[WEAPON_THUNDER], RES_BIG_ZOMBIE, 3000);
  now->setDropRate(100);
  initCommonSprite(now = &commonSprites[SPRITE_ORGRE], &weapons[WEAPON_MANY_AXES],
                   RES_ORGRE, 3000);
  now->setDropRate(100);
  initCommonSprite(now = &commonSprites[SPRITE_BIG_DEMON],
                   &weapons[WEAPON_THUNDER], RES_BIG_DEMON, 2500);
  now->setDropRate(100);
}
