#include "types.h"

#include <SDL_ttf.h>

#include <cmath>
#include <memory>
#include <utility>

SDL_Color BLACK = {0, 0, 0, 255};
SDL_Color WHITE = {255, 255, 255, 255};

Texture::Texture(SDL_Texture* origin, int width, int height, int frames)
    : origin_(origin),
      height_(height),
      width_(width),
      frames_(frames),
      crops_(std::make_unique<SDL_Rect[]>(frames)) {}

Texture::~Texture() = default;

SDL_Texture* Texture::origin() const { return origin_; }
int Texture::height() const { return height_; }
int Texture::width() const { return width_; }
int Texture::frames() const { return frames_; }
SDL_Rect* Texture::crops() const { return crops_.get(); }
void Texture::setWidth(int width) { width_ = width; }

Text::Text(const std::string& text, SDL_Color color, SDL_Renderer* renderer,
           TTF_Font* font)
    : color_(color) {
  setText(text, renderer, font);
}

Text::Text(Text&& other) noexcept
    : text_(std::move(other.text_)),
      height_(other.height_),
      width_(other.width_),
      origin_(other.origin_),
      color_(other.color_) {
  other.origin_ = nullptr;
}

Text& Text::operator=(Text&& other) noexcept {
  if (this != &other) {
    if (origin_) {
      SDL_DestroyTexture(origin_);
    }
    text_ = std::move(other.text_);
    height_ = other.height_;
    width_ = other.width_;
    origin_ = other.origin_;
    color_ = other.color_;
    other.origin_ = nullptr;
  }
  return *this;
}

Text::~Text() {
  if (origin_) {
    SDL_DestroyTexture(origin_);
    origin_ = nullptr;
  }
}

const std::string& Text::value() const { return text_; }
int Text::height() const { return height_; }
int Text::width() const { return width_; }
SDL_Texture* Text::origin() const { return origin_; }
SDL_Color Text::color() const { return color_; }

void Text::setText(const std::string& text, SDL_Renderer* renderer,
                   TTF_Font* font) {
  if (text == text_) {
    return;
  }
  text_ = text;
  if (origin_) {
    SDL_DestroyTexture(origin_);
    origin_ = nullptr;
  }

  SDL_Surface* textSurface = TTF_RenderText_Solid(font, text_.c_str(), color_);
  if (!textSurface) {
    return;
  }

  origin_ = SDL_CreateTextureFromSurface(renderer, textSurface);
  width_ = textSurface->w;
  height_ = textSurface->h;
  SDL_FreeSurface(textSurface);
}

Effect::Effect(int duration, int length, SDL_BlendMode mode)
    : duration_(duration), length_(length), mode_(mode) {
  if (length_ > 0) {
    keys_ = std::make_unique<SDL_Color[]>(length_);
  }
}

Effect::Effect(const Effect& other)
    : duration_(other.duration_),
      currentFrame_(other.currentFrame_),
      length_(other.length_),
      mode_(other.mode_) {
  if (length_ > 0) {
    keys_ = std::make_unique<SDL_Color[]>(length_);
    std::copy(other.keys_.get(), other.keys_.get() + length_, keys_.get());
  }
}

Effect& Effect::operator=(const Effect& other) {
  if (this == &other) {
    return *this;
  }
  duration_ = other.duration_;
  currentFrame_ = other.currentFrame_;
  length_ = other.length_;
  mode_ = other.mode_;
  if (length_ > 0) {
    keys_ = std::make_unique<SDL_Color[]>(length_);
    std::copy(other.keys_.get(), other.keys_.get() + length_, keys_.get());
  } else {
    keys_.reset();
  }
  return *this;
}

int Effect::duration() const { return duration_; }
int Effect::currentFrame() const { return currentFrame_; }
int Effect::length() const { return length_; }
SDL_Color* Effect::keys() const { return keys_.get(); }
SDL_BlendMode Effect::mode() const { return mode_; }
void Effect::setCurrentFrame(int frame) { currentFrame_ = frame; }

Animation::Animation(Texture* origin, const Effect* effect, LoopType loopType,
                     int duration, int x, int y, SDL_RendererFlip flip,
                     double angle, At at)
    : loopType_(loopType),
      origin_(origin),
      duration_(duration),
      currentFrame_(0),
      x_(x),
      y_(y),
      angle_(angle),
      flip_(flip),
      at_(at),
      lifeSpan_(duration) {
  if (effect) {
    effect_ = std::make_shared<Effect>(*effect);
  }
}

Animation::Animation(const Animation& other)
    : loopType_(other.loopType_),
      origin_(other.origin_),
      duration_(other.duration_),
      currentFrame_(other.currentFrame_),
      x_(other.x_),
      y_(other.y_),
      angle_(other.angle_),
      flip_(other.flip_),
      at_(other.at_),
      bind_(other.bind_),
      scaled_(other.scaled_),
      dieWithBind_(other.dieWithBind_),
      lifeSpan_(other.lifeSpan_) {
  if (other.effect_) {
    effect_ = std::make_shared<Effect>(*other.effect_);
  }
}

Animation& Animation::operator=(const Animation& other) {
  if (this == &other) {
    return *this;
  }
  loopType_ = other.loopType_;
  origin_ = other.origin_;
  duration_ = other.duration_;
  currentFrame_ = other.currentFrame_;
  x_ = other.x_;
  y_ = other.y_;
  angle_ = other.angle_;
  flip_ = other.flip_;
  at_ = other.at_;
  bind_ = other.bind_;
  scaled_ = other.scaled_;
  dieWithBind_ = other.dieWithBind_;
  lifeSpan_ = other.lifeSpan_;
  if (other.effect_) {
    effect_ = std::make_shared<Effect>(*other.effect_);
  } else {
    effect_.reset();
  }
  return *this;
}

LoopType Animation::loopType() const { return loopType_; }
Texture* Animation::origin() const { return origin_; }
Effect* Animation::effect() const { return effect_.get(); }
int Animation::duration() const { return duration_; }
int Animation::currentFrame() const { return currentFrame_; }
int Animation::x() const { return x_; }
int Animation::y() const { return y_; }
double Animation::angle() const { return angle_; }
SDL_RendererFlip Animation::flip() const { return flip_; }
At Animation::at() const { return at_; }
Sprite* Animation::bind() const { return bind_; }
bool Animation::scaled() const { return scaled_; }
bool Animation::dieWithBind() const { return dieWithBind_; }
int Animation::lifeSpan() const { return lifeSpan_; }

void Animation::setOrigin(Texture* origin) { origin_ = origin; }
void Animation::setEffect(const Effect* effect) {
  if (effect) {
    effect_ = std::make_shared<Effect>(*effect);
  } else {
    effect_.reset();
  }
}
void Animation::setDuration(int duration) { duration_ = duration; }
void Animation::setCurrentFrame(int frame) { currentFrame_ = frame; }
void Animation::setPosition(int x, int y) {
  x_ = x;
  y_ = y;
}
void Animation::setAngle(double angle) { angle_ = angle; }
void Animation::setFlip(SDL_RendererFlip flip) { flip_ = flip; }
void Animation::setAt(At at) { at_ = at; }
void Animation::setBind(Sprite* sprite, bool dieWithBind) {
  bind_ = sprite;
  dieWithBind_ = dieWithBind;
}
void Animation::setScaled(bool scaled) { scaled_ = scaled; }
void Animation::setLifeSpan(int lifeSpan) { lifeSpan_ = lifeSpan; }
void Animation::setLoopType(LoopType loopType) { loopType_ = loopType; }

int Score::damage() const { return damage_; }
int Score::stand() const { return stand_; }
int Score::killed() const { return killed_; }
int Score::got() const { return got_; }
double Score::rank() const { return rank_; }

void Score::addScore(const Score& other) {
  got_ += other.got_;
  damage_ += other.damage_;
  killed_ += other.killed_;
  stand_ += other.stand_;
}

void Score::calcScore(int gameLevel) {
  if (got_ == 0) {
    rank_ = 0.0;
    return;
  }
  rank_ = static_cast<double>(damage_) / got_ +
          static_cast<double>(stand_) / got_ + got_ * 50 + killed_ * 100;
  rank_ *= gameLevel + 1;
}

void Score::addDamage(int value) { damage_ += value; }
void Score::addStand(int value) { stand_ += value; }
void Score::addKilled(int value) { killed_ += value; }
void Score::addGot(int value) { got_ += value; }

void Score::setDamage(int value) { damage_ = value; }
void Score::setStand(int value) { stand_ = value; }
void Score::setKilled(int value) { killed_ = value; }
void Score::setGot(int value) { got_ = value; }

Block::Block(BlockType type, int x, int y, int bid,
             const std::shared_ptr<Animation>& animation)
    : type_(type), x_(x), y_(y), id_(bid), animation_(animation) {}

BlockType Block::type() const { return type_; }
int Block::x() const { return x_; }
int Block::y() const { return y_; }
int Block::id() const { return id_; }
bool Block::enabled() const { return enabled_; }
std::shared_ptr<Animation> Block::animation() const { return animation_; }

void Block::setEnabled(bool enabled) { enabled_ = enabled; }
void Block::setAnimation(const std::shared_ptr<Animation>& animation) {
  animation_ = animation;
}
void Block::setPosition(int x, int y) {
  x_ = x;
  y_ = y;
}
void Block::setType(BlockType type) { type_ = type; }
void Block::setId(int id) { id_ = id; }

Item::Item(ItemType type, int id, int belong,
           const std::shared_ptr<Animation>& animation)
    : type_(type), id_(id), belong_(belong), animation_(animation) {}

ItemType Item::type() const { return type_; }
int Item::id() const { return id_; }
int Item::belong() const { return belong_; }
std::shared_ptr<Animation> Item::animation() const { return animation_; }

void Item::setAnimation(const std::shared_ptr<Animation>& animation) {
  animation_ = animation;
}
void Item::setBelong(int belong) { belong_ = belong; }
void Item::setId(int id) { id_ = id; }
void Item::setType(ItemType type) { type_ = type; }
