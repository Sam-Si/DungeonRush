#include "weapon.h"

#include <memory>

#include "ai.h"
#include "audio.h"
#include "bullet.h"
#include "game.h"
#include "helper.h"
#include "render.h"
#include "res.h"
#include "types.h"

extern Texture textures[TEXTURES_SIZE];
extern BulletList bullets;

namespace {
std::shared_ptr<Animation> createWeaponAnimation(int textureId, LoopType loop,
                                                 At at) {
  if (textureId == -1) {
    return nullptr;
  }
  return std::make_shared<Animation>(&textures[textureId], nullptr, loop,
                                     SPRITE_ANIMATION_DURATION, 0, 0,
                                     SDL_FLIP_NONE, 0.0, at);
}

void initWeapon(Weapon& weapon, int birthTextureId, int deathTextureId,
                int flyTextureId) {
  auto birth = createWeaponAnimation(birthTextureId, LoopType::Once, At::Center);
  auto death =
      createWeaponAnimation(deathTextureId, LoopType::Once, At::BottomCenter);
  auto fly =
      createWeaponAnimation(flyTextureId, LoopType::Infinite, At::Center);
  weapon = Weapon{WeaponType::SwordPoint, 32 * 2, 40, 10, 60, 6, birth, death,
                  fly, -1, AUDIO_CLAW_HIT};
}
}  // namespace

namespace {
void applyWeaponDamage(const std::shared_ptr<Snake>& src,
                       const std::shared_ptr<Snake>& dest,
                       const std::shared_ptr<Sprite>& target,
                       const Weapon& weapon) {
  GameContext& ctx = getGameContext();
  if (!dest || !target) {
    return;
  }
  double calcDamage = weapon.damage();
  if (dest->buffs()[BUFF_FROZEN]) {
    calcDamage *= GAME_FROZEN_DAMAGE_K;
  }
  if (src && src->team() != GAME_MONSTERS_TEAM) {
    if (src->buffs()[BUFF_ATTACK]) {
      calcDamage *= GAME_BUFF_ATTACK_K;
    }
  }
  if (dest->team() != GAME_MONSTERS_TEAM) {
    if (dest->buffs()[BUFF_DEFFENCE]) {
      calcDamage /= GAME_BUFF_DEFENSE_K;
    }
  }
  target->setHp(target->hp() - static_cast<int>(calcDamage));
  if (src) {
    src->score()->addDamage(static_cast<int>(calcDamage));
    if (target->hp() <= 0) {
      src->score()->addKilled(1);
    }
  }
  dest->score()->addStand(static_cast<int>(calcDamage));
  ctx.buffManager.invokeWeaponBuff(src, weapon, dest, weapon.damage());
}

class SwordBehavior final : public WeaponBehavior {
 public:
  WeaponAttackResult attack(const Weapon& weapon,
                            const WeaponAttackContext& context) const override {
    if (!context.attacker || !context.target || !context.targetSprite ||
        !context.attackerSprite) {
      return {};
    }
    const double rad =
        atan2(context.targetSprite->y() - context.attackerSprite->y(),
              context.targetSprite->x() - context.attackerSprite->x());
    if (const auto& deathAnimation = weapon.deathAnimation()) {
      auto effect = std::make_shared<Animation>(*deathAnimation);
      bindAnimationToSprite(effect, context.targetSprite, false);
      if (effect->angle() != -1) {
        effect->setAngle(rad * 180 / PI);
      }
      pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
    }
    applyWeaponDamage(context.attacker, context.target, context.targetSprite,
                      weapon);
    return {.attacked = true};
  }

  void onAttack(const Weapon& weapon,
                const std::shared_ptr<Sprite>& sprite) const override {
    if (!sprite) {
      return;
    }
    if (const auto& birthAnimation = weapon.birthAnimation()) {
      auto effect = std::make_shared<Animation>(*birthAnimation);
      bindAnimationToSprite(effect, sprite, true);
      effect->setAt(At::BottomCenter);
      pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
    }
    playAudio(weapon.deathAudio());
    sprite->setLastAttack(static_cast<int>(renderFrames()));
  }

  bool allowMultiTarget() const override { return true; }
  bool allowAreaImpact() const override { return false; }

  void applyImpact(const Weapon& weapon, const std::shared_ptr<Bullet>&,
                   const std::shared_ptr<Snake>& hitSnake,
                   const std::shared_ptr<Sprite>& hitSprite) const override {
    if (!hitSnake || !hitSprite) {
      return;
    }
    applyWeaponDamage(nullptr, hitSnake, hitSprite, weapon);
  }

  void applyAreaImpact(const Weapon& weapon, const std::shared_ptr<Bullet>&,
                       const std::shared_ptr<Snake>& hitSnake,
                       const std::shared_ptr<Sprite>& hitSprite) const override {
    if (!hitSnake || !hitSprite) {
      return;
    }
    applyWeaponDamage(nullptr, hitSnake, hitSprite, weapon);
  }
};

class RangedBehavior final : public WeaponBehavior {
 public:
  explicit RangedBehavior(bool multiShot) : multiShot_(multiShot) {}

  WeaponAttackResult attack(const Weapon& weapon,
                            const WeaponAttackContext& context) const override {
    if (!context.attacker || !context.attackerSprite || !context.targetSprite) {
      return {};
    }
    const double rad =
        atan2(context.targetSprite->y() - context.attackerSprite->y(),
              context.targetSprite->x() - context.attackerSprite->x());
    auto bullet = std::make_shared<Bullet>(context.attacker,
                                           const_cast<Weapon*>(&weapon),
                                           context.attackerSprite->x(),
                                           context.attackerSprite->y(), rad,
                                           context.attacker->team(),
                                           weapon.flyAnimation());
    ::bullets.push_back(bullet);
    pushAnimationToRender(RENDER_LIST_EFFECT_ID, bullet->animation());
    return {.attacked = true};
  }

  void onAttack(const Weapon& weapon,
                const std::shared_ptr<Sprite>& sprite) const override {
    if (!sprite) {
      return;
    }
    if (const auto& birthAnimation = weapon.birthAnimation()) {
      auto effect = std::make_shared<Animation>(*birthAnimation);
      bindAnimationToSprite(effect, sprite, true);
      effect->setAt(At::BottomCenter);
      pushAnimationToRender(RENDER_LIST_EFFECT_ID, effect);
    }
    playAudio(weapon.birthAudio());
    sprite->setLastAttack(static_cast<int>(renderFrames()));
  }

  bool allowMultiTarget() const override { return multiShot_; }
  bool allowAreaImpact() const override { return true; }

  void applyImpact(const Weapon& weapon, const std::shared_ptr<Bullet>& bullet,
                   const std::shared_ptr<Snake>& hitSnake,
                   const std::shared_ptr<Sprite>& hitSprite) const override {
    if (!bullet || !hitSnake || !hitSprite) {
      return;
    }
    applyWeaponDamage(bullet->owner(), hitSnake, hitSprite, weapon);
  }

  void applyAreaImpact(const Weapon& weapon, const std::shared_ptr<Bullet>& bullet,
                       const std::shared_ptr<Snake>& hitSnake,
                       const std::shared_ptr<Sprite>& hitSprite) const override {
    if (!bullet || !hitSnake || !hitSprite) {
      return;
    }
    applyWeaponDamage(bullet->owner(), hitSnake, hitSprite, weapon);
  }

 private:
  bool multiShot_ = false;
};

class WeaponBehaviorFactory final {
 public:
  WeaponBehaviorFactory() = delete;
  static std::shared_ptr<WeaponBehavior> makeBehavior(WeaponType type) {
    if (type == WeaponType::SwordPoint || type == WeaponType::SwordRange) {
      return std::make_shared<SwordBehavior>();
    }
    if (type == WeaponType::GunPoint || type == WeaponType::GunRange) {
      return std::make_shared<RangedBehavior>(false);
    }
    if (type == WeaponType::GunPointMulti) {
      return std::make_shared<RangedBehavior>(true);
    }
    return std::make_shared<SwordBehavior>();
  }
};
}  // namespace

Weapon::Weapon(WeaponType type, int shootRange, int effectRange, int damage,
               int gap, int bulletSpeed,
               const std::shared_ptr<Animation>& birthAnimation,
               const std::shared_ptr<Animation>& deathAnimation,
               const std::shared_ptr<Animation>& flyAnimation, int birthAudio,
               int deathAudio)
    : type_(type),
      shootRange_(shootRange),
      effectRange_(effectRange),
      damage_(damage),
      gap_(gap),
      bulletSpeed_(bulletSpeed),
      birthAnimation_(birthAnimation),
      deathAnimation_(deathAnimation),
      flyAnimation_(flyAnimation),
      birthAudio_(birthAudio),
      deathAudio_(deathAudio),
      behavior_(WeaponBehaviorFactory::makeBehavior(type)) {}

WeaponType Weapon::type() const { return type_; }
int Weapon::shootRange() const { return shootRange_; }
int Weapon::effectRange() const { return effectRange_; }
int Weapon::damage() const { return damage_; }
int Weapon::gap() const { return gap_; }
int Weapon::bulletSpeed() const { return bulletSpeed_; }
const std::shared_ptr<Animation>& Weapon::birthAnimation() const {
  return birthAnimation_;
}
const std::shared_ptr<Animation>& Weapon::deathAnimation() const {
  return deathAnimation_;
}
const std::shared_ptr<Animation>& Weapon::flyAnimation() const {
  return flyAnimation_;
}
int Weapon::birthAudio() const { return birthAudio_; }
int Weapon::deathAudio() const { return deathAudio_; }
const std::array<WeaponBuff, BUFF_END>& Weapon::effects() const {
  return effects_;
}
std::array<WeaponBuff, BUFF_END>& Weapon::effects() { return effects_; }
const WeaponBehavior& Weapon::behavior() const {
  return behavior_ ? *behavior_ : *WeaponBehaviorFactory::makeBehavior(type_);
}

void Weapon::setType(WeaponType type) {
  type_ = type;
  behavior_ = WeaponBehaviorFactory::makeBehavior(type);
}
void Weapon::setShootRange(int shootRange) { shootRange_ = shootRange; }
void Weapon::setEffectRange(int effectRange) { effectRange_ = effectRange; }
void Weapon::setDamage(int damage) { damage_ = damage; }
void Weapon::setGap(int gap) { gap_ = gap; }
void Weapon::setBulletSpeed(int bulletSpeed) { bulletSpeed_ = bulletSpeed; }
void Weapon::setBirthAnimation(const std::shared_ptr<Animation>& animation) {
  birthAnimation_ = animation;
}
void Weapon::setDeathAnimation(const std::shared_ptr<Animation>& animation) {
  deathAnimation_ = animation;
}
void Weapon::setFlyAnimation(const std::shared_ptr<Animation>& animation) {
  flyAnimation_ = animation;
}
void Weapon::setBirthAudio(int audio) { birthAudio_ = audio; }
void Weapon::setDeathAudio(int audio) { deathAudio_ = audio; }
void Weapon::setBehavior(const std::shared_ptr<WeaponBehavior>& behavior) {
  behavior_ = behavior;
}

std::shared_ptr<WeaponBehavior> makeWeaponBehavior(WeaponType type) {
  return WeaponBehaviorFactory::makeBehavior(type);
}

Weapon weapons[WEAPONS_SIZE];

void initWeapons() {
  Weapon* now;

  initWeapon(*(&weapons[WEAPON_SWORD]), -1, RES_SwordFx, -1);
  now = &weapons[WEAPON_SWORD];
  now->setDamage(30);
  now->setShootRange(32 * 3);
  if (now->deathAnimation()) {
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAngle(-1.0);
  }
  now->setDeathAudio(AUDIO_SWORD_HIT);

  initWeapon(*(&weapons[WEAPON_MONSTER_CLAW]), -1, RES_CLAWFX2, -1);
  now = &weapons[WEAPON_MONSTER_CLAW];
  now->setType(WeaponType::SwordRange);
  now->setShootRange(32 * 3 + 16);
  now->setDamage(24);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setAt(At::Center);
  }
  now->setDeathAudio(AUDIO_CLAW_HIT_HEAVY);

  initWeapon(*(&weapons[WEAPON_FIREBALL]), RES_Shine, RES_HALO_EXPLOSION1,
             RES_FIREBALL);
  now = &weapons[WEAPON_FIREBALL];
  now->setType(WeaponType::GunRange);
  now->setDamage(45);
  now->setEffectRange(50);
  now->setShootRange(256);
  now->setGap(180);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setAt(At::Center);
  }
  if (now->birthAnimation()) {
    now->birthAnimation()->setDuration(24);
  }
  now->setBirthAudio(AUDIO_SHOOT);
  now->setDeathAudio(AUDIO_FIREBALL_EXP);

  initWeapon(*(&weapons[WEAPON_THUNDER]), RES_BLOOD_BOUND, RES_Thunder, -1);
  now = &weapons[WEAPON_THUNDER];
  now->setType(WeaponType::SwordRange);
  now->setDamage(80);
  now->setShootRange(128);
  now->setGap(120);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setScaled(false);
  }
  now->setDeathAudio(AUDIO_THUNDER);

  initWeapon(*(&weapons[WEAPON_THUNDER_STAFF]), -1, RES_THUNDER_YELLOW, -1);
  now = &weapons[WEAPON_THUNDER_STAFF];
  now->setType(WeaponType::SwordRange);
  now->setDamage(50);
  now->setShootRange(128);
  now->setGap(120);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setScaled(false);
  }
  now->setDeathAudio(AUDIO_THUNDER);

  initWeapon(*(&weapons[WEAPON_ARROW]), -1, RES_HALO_EXPLOSION2, RES_ARROW);
  now = &weapons[WEAPON_ARROW];
  now->setType(WeaponType::GunPoint);
  now->setGap(40);
  now->setDamage(10);
  now->setShootRange(200);
  now->setBulletSpeed(10);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setAt(At::Center);
  }
  if (now->flyAnimation()) {
    now->flyAnimation()->setScaled(false);
  }
  now->setBirthAudio(AUDIO_LIGHT_SHOOT);
  now->setDeathAudio(AUDIO_ARROW_HIT);

  initWeapon(*(&weapons[WEAPON_POWERFUL_BOW]), -1, RES_HALO_EXPLOSION2,
             RES_ARROW);
  now = &weapons[WEAPON_POWERFUL_BOW];
  now->setType(WeaponType::GunPoint);
  now->setGap(60);
  now->setDamage(25);
  now->setShootRange(320);
  now->setBulletSpeed(7);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setAt(At::Center);
  }
  now->setBirthAudio(AUDIO_LIGHT_SHOOT);
  now->setDeathAudio(AUDIO_ARROW_HIT);
  now->effects()[BUFF_ATTACK] = {0.5, 240};

  initWeapon(*(&weapons[WEAPON_MONSTER_CLAW2]), -1, RES_CLAWFX, -1);
  now = &weapons[WEAPON_MONSTER_CLAW2];

  initWeapon(*(&weapons[WEAPON_THROW_AXE]), -1, RES_CROSS_HIT, RES_AXE);
  now = &weapons[WEAPON_THROW_AXE];
  now->setType(WeaponType::GunPoint);
  now->setDamage(12);
  now->setShootRange(160);
  now->setBulletSpeed(10);
  if (now->flyAnimation()) {
    now->flyAnimation()->setDuration(24);
    now->flyAnimation()->setAngle(-1.0);
    now->flyAnimation()->setScaled(false);
  }
  if (now->deathAnimation()) {
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAt(At::Center);
  }
  now->setBirthAudio(AUDIO_LIGHT_SHOOT);
  now->setDeathAudio(AUDIO_ARROW_HIT);

  initWeapon(*(&weapons[WEAPON_MANY_AXES]), -1, RES_CROSS_HIT, RES_AXE);
  now = &weapons[WEAPON_MANY_AXES];
  now->setType(WeaponType::GunPointMulti);
  now->setShootRange(180);
  now->setGap(70);
  now->setEffectRange(50);
  now->setDamage(50);
  now->setBulletSpeed(4);
  if (now->flyAnimation()) {
    now->flyAnimation()->setDuration(24);
    now->flyAnimation()->setAngle(-1.0);
  }
  if (now->deathAnimation()) {
    now->deathAnimation()->setAt(At::Center);
  }
  now->setBirthAudio(AUDIO_LIGHT_SHOOT);
  now->setDeathAudio(AUDIO_ARROW_HIT);

  initWeapon(*(&weapons[WEAPON_SOLID]), -1, RES_SOLIDFX, -1);
  now = &weapons[WEAPON_SOLID];
  if (now->deathAnimation()) {
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAngle(-1.0);
  }
  now->effects()[BUFF_SLOWDOWN] = {0.3, 180};

  initWeapon(*(&weapons[WEAPON_SOLID_GREEN]), -1, RES_SOLID_GREENFX, -1);
  now = &weapons[WEAPON_SOLID_GREEN];
  now->setShootRange(96);
  if (now->deathAnimation()) {
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAngle(-1.0);
  }
  now->effects()[BUFF_SLOWDOWN] = {0.3, 180};

  initWeapon(*(&weapons[WEAPON_SOLID_CLAW]), -1, RES_SOLID_GREENFX, -1);
  now = &weapons[WEAPON_SOLID_CLAW];
  now->setType(WeaponType::SwordRange);
  now->setShootRange(32 * 3 + 16);
  now->setDamage(35);
  if (now->deathAnimation()) {
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAngle(-1.0);
  }
  now->setDeathAudio(AUDIO_CLAW_HIT_HEAVY);
  now->effects()[BUFF_SLOWDOWN] = {0.7, 60};

  initWeapon(*(&weapons[WEAPON_ICEPICK]), -1, RES_ICESHATTER, RES_ICEPICK);
  now = &weapons[WEAPON_ICEPICK];
  now->setType(WeaponType::GunRange);
  now->setDamage(30);
  now->setEffectRange(50);
  now->setShootRange(256);
  now->setGap(180);
  now->setBulletSpeed(8);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setAt(At::Center);
  }
  if (now->flyAnimation()) {
    now->flyAnimation()->setScaled(false);
  }
  now->effects()[BUFF_FROZEN] = {0.2, 60};
  now->setBirthAudio(AUDIO_ICE_SHOOT);

  initWeapon(*(&weapons[WEAPON_PURPLE_BALL]), -1, RES_PURPLE_EXP,
             RES_PURPLE_BALL);
  now = &weapons[WEAPON_PURPLE_BALL];
  now->setType(WeaponType::GunRange);
  now->setDamage(20);
  now->setEffectRange(50);
  now->setShootRange(256);
  now->setGap(100);
  now->setBulletSpeed(6);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAt(At::Center);
  }
  if (now->flyAnimation()) {
    now->flyAnimation()->setScaled(false);
  }
  now->setBirthAudio(AUDIO_ICE_SHOOT);
  now->setDeathAudio(AUDIO_ARROW_HIT);

  initWeapon(*(&weapons[WEAPON_PURPLE_STAFF]), -1, RES_PURPLE_EXP,
             RES_PURPLE_BALL);
  now = &weapons[WEAPON_PURPLE_STAFF];
  now->setType(WeaponType::GunPointMulti);
  now->setDamage(45);
  now->setEffectRange(50);
  now->setShootRange(256);
  now->setGap(100);
  now->setBulletSpeed(7);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setScaled(false);
    now->deathAnimation()->setAt(At::Center);
  }
  if (now->flyAnimation()) {
    now->flyAnimation()->setScaled(false);
  }
  now->setBirthAudio(AUDIO_ICE_SHOOT);
  now->setDeathAudio(AUDIO_ARROW_HIT);

  initWeapon(*(&weapons[WEAPON_HOLY_SWORD]), -1, RES_GOLDEN_CROSS_HIT, -1);
  now = &weapons[WEAPON_HOLY_SWORD];
  now->setType(WeaponType::SwordRange);
  now->setDamage(30);
  now->setShootRange(32 * 4);
  now->effects()[BUFF_DEFFENCE] = {0.6, 180};

  initWeapon(*(&weapons[WEAPON_ICE_SWORD]), -1, RES_ICESHATTER, -1);
  now = &weapons[WEAPON_ICE_SWORD];
  now->setType(WeaponType::SwordRange);
  now->setShootRange(32 * 3 + 16);
  now->setDamage(80);
  now->setGap(30);
  if (now->deathAnimation()) {
    now->deathAnimation()->setAngle(-1.0);
    now->deathAnimation()->setAt(At::Center);
  }
  now->effects()[BUFF_FROZEN] = {0.6, 80};
  now->setDeathAudio(AUDIO_SWORD_HIT);

#ifdef DBG
  puts("weapon done");
#endif
}
