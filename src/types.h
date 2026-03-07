#ifndef SNAKE_TYPES_
#define SNAKE_TYPES_

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <array>
#include <memory>
#include <string>

#define TEXT_LEN 1024
#define POSITION_BUFFER_SIZE 256
#define BUFF_BEGIN 0
#define BUFF_FROZEN 0
#define BUFF_SLOWDOWN 1
#define BUFF_DEFFENCE 2
#define BUFF_ATTACK 3
#define BUFF_END 4
// Renderer Types
enum class Direction { Left, Right, Up, Down };
enum class At { TopLeft, BottomLeft, BottomCenter, Center };
enum class LoopType { Once, Infinite, Lifespan };

constexpr Direction LEFT = Direction::Left;
constexpr Direction RIGHT = Direction::Right;
constexpr Direction UP = Direction::Up;
constexpr Direction DOWN = Direction::Down;

constexpr At AT_TOP_LEFT = At::TopLeft;
constexpr At AT_BOTTOM_LEFT = At::BottomLeft;
constexpr At AT_BOTTOM_CENTER = At::BottomCenter;
constexpr At AT_CENTER = At::Center;

constexpr LoopType LOOP_ONCE = LoopType::Once;
constexpr LoopType LOOP_INFI = LoopType::Infinite;
constexpr LoopType LOOP_LIFESPAN = LoopType::Lifespan;

class Effect;
class Sprite;

class Texture {
 public:
  Texture() = default;
  Texture(SDL_Texture* origin, int width, int height, int frames);
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&&) noexcept = default;
  Texture& operator=(Texture&&) noexcept = default;
  ~Texture();

  SDL_Texture* origin() const;
  int height() const;
  int width() const;
  int frames() const;
  SDL_Rect* crops() const;

  void setWidth(int width);

 private:
  SDL_Texture* origin_ = nullptr;
  int height_ = 0;
  int width_ = 0;
  int frames_ = 0;
  std::unique_ptr<SDL_Rect[]> crops_{};
};

class Text {
 public:
  Text() = default;
  Text(const std::string& text, SDL_Color color, SDL_Renderer* renderer,
       TTF_Font* font);
  Text(const Text&) = delete;
  Text& operator=(const Text&) = delete;
  Text(Text&& other) noexcept;
  Text& operator=(Text&& other) noexcept;
  ~Text();

  const std::string& value() const;
  int height() const;
  int width() const;
  SDL_Texture* origin() const;
  SDL_Color color() const;

  void setText(const std::string& text, SDL_Renderer* renderer, TTF_Font* font);

 private:
  std::string text_{};
  int height_ = 0;
  int width_ = 0;
  SDL_Texture* origin_ = nullptr;
  SDL_Color color_{};
};

class Effect {
 public:
  Effect() = default;
  Effect(int duration, int length, SDL_BlendMode mode);
  Effect(const Effect& other);
  Effect& operator=(const Effect& other);
  Effect(Effect&&) noexcept = default;
  Effect& operator=(Effect&&) noexcept = default;
  ~Effect() = default;

  int duration() const;
  int currentFrame() const;
  int length() const;
  SDL_Color* keys() const;
  SDL_BlendMode mode() const;

  void setCurrentFrame(int frame);

 private:
  int duration_ = 0;
  int currentFrame_ = 0;
  int length_ = 0;
  std::unique_ptr<SDL_Color[]> keys_{};
  SDL_BlendMode mode_ = SDL_BLENDMODE_NONE;
};

class Animation {
 public:
  Animation() = default;
  Animation(Texture* origin, const Effect* effect, LoopType loopType,
            int duration, int x, int y, SDL_RendererFlip flip, double angle,
            At at);
  Animation(const Animation& other);
  Animation& operator=(const Animation& other);
  Animation(Animation&&) noexcept = default;
  Animation& operator=(Animation&&) noexcept = default;
  ~Animation() = default;

  LoopType loopType() const;
  Texture* origin() const;
  Effect* effect() const;
  int duration() const;
  int currentFrame() const;
  int x() const;
  int y() const;
  double angle() const;
  SDL_RendererFlip flip() const;
  At at() const;
  Sprite* bind() const;
  bool scaled() const;
  bool dieWithBind() const;
  int lifeSpan() const;

  void setOrigin(Texture* origin);
  void setEffect(const Effect* effect);
  void setDuration(int duration);
  void setCurrentFrame(int frame);
  void setPosition(int x, int y);
  void setAngle(double angle);
  void setFlip(SDL_RendererFlip flip);
  void setAt(At at);
  void setBind(Sprite* sprite, bool dieWithBind);
  void setScaled(bool scaled);
  void setLifeSpan(int lifeSpan);

  void setLoopType(LoopType loopType);

 private:
  LoopType loopType_ = LoopType::Once;
  Texture* origin_ = nullptr;
  std::shared_ptr<Effect> effect_{};
  int duration_ = 0;
  int currentFrame_ = 0;
  int x_ = 0;
  int y_ = 0;
  double angle_ = 0.0;
  SDL_RendererFlip flip_ = SDL_FLIP_NONE;
  At at_ = At::TopLeft;
  Sprite* bind_ = nullptr;
  bool scaled_ = true;
  bool dieWithBind_ = false;
  int lifeSpan_ = 0;
};

// Game Logic Types
struct Point {
  int x = 0;
  int y = 0;
};

class Score {
 public:
  Score() = default;
  Score(const Score&) = delete;
  Score& operator=(const Score&) = delete;
  Score(Score&&) noexcept = default;
  Score& operator=(Score&&) noexcept = default;
  ~Score() = default;

  int damage() const;
  int stand() const;
  int killed() const;
  int got() const;
  double rank() const;

  void addScore(const Score& other);
  void calcScore(int gameLevel);
  void addDamage(int value);
  void addStand(int value);
  void addKilled(int value);
  void addGot(int value);

  void setDamage(int value);
  void setStand(int value);
  void setKilled(int value);
  void setGot(int value);

 private:
  int damage_ = 0;
  int stand_ = 0;
  int killed_ = 0;
  int got_ = 0;
  double rank_ = 0.0;
};

enum class BlockType { Trap, Wall, Floor, Exit };

class Block {
 public:
  Block() = default;
  Block(BlockType type, int x, int y, int bid,
        const std::shared_ptr<Animation>& animation);
  Block(const Block&) = default;
  Block& operator=(const Block&) = default;
  Block(Block&&) noexcept = default;
  Block& operator=(Block&&) noexcept = default;
  ~Block() = default;

  BlockType type() const;
  int x() const;
  int y() const;
  int id() const;
  bool enabled() const;
  std::shared_ptr<Animation> animation() const;

  void setEnabled(bool enabled);
  void setAnimation(const std::shared_ptr<Animation>& animation);
  void setPosition(int x, int y);
  void setType(BlockType type);
  void setId(int id);

 private:
  BlockType type_ = BlockType::Floor;
  int x_ = 0;
  int y_ = 0;
  int id_ = 0;
  bool enabled_ = false;
  std::shared_ptr<Animation> animation_{};
};

enum class ItemType { None, Hero, HpMedicine, HpExtraMedicine, Weapon };

class Item {
 public:
  Item() = default;
  Item(ItemType type, int id, int belong,
       const std::shared_ptr<Animation>& animation);
  Item(const Item&) = default;
  Item& operator=(const Item&) = default;
  Item(Item&&) noexcept = default;
  Item& operator=(Item&&) noexcept = default;
  ~Item() = default;

  ItemType type() const;
  int id() const;
  int belong() const;
  std::shared_ptr<Animation> animation() const;

  void setAnimation(const std::shared_ptr<Animation>& animation);
  void setBelong(int belong);
  void setId(int id);
  void setType(ItemType type);

 private:
  ItemType type_ = ItemType::None;
  int id_ = 0;
  int belong_ = 0;
  std::shared_ptr<Animation> animation_{};
};

#endif
