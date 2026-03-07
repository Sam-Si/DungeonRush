#ifndef SNAKE_WEAPON_H_
#define SNAKE_WEAPON_H_

#include "adt.h"
#include "types.h"

#include <array>
#include <functional>
#include <memory>

#define WEAPONS_SIZE 128
#define WEAPON_SWORD 0
#define WEAPON_MONSTER_CLAW 1
#define WEAPON_FIREBALL 2
#define WEAPON_THUNDER 3
#define WEAPON_ARROW 4
#define WEAPON_MONSTER_CLAW2 5
#define WEAPON_THROW_AXE 6
#define WEAPON_MANY_AXES 7
#define WEAPON_SOLID 8
#define WEAPON_SOLID_GREEN 9
#define WEAPON_ICEPICK 10
#define WEAPON_FIRE_SWORD 11
#define WEAPON_ICE_SWORD 12
#define WEAPON_HOLY_SWORD 13
#define WEAPON_PURPLE_BALL 14
#define WEAPON_PURPLE_STAFF 15
#define WEAPON_THUNDER_STAFF 16
#define WEAPON_SOLID_CLAW 17
#define WEAPON_POWERFUL_BOW 18

enum class WeaponType {
  SwordPoint,
  SwordRange,
  GunRange,
  GunPoint,
  GunPointMulti,
};

struct WeaponBuff {
  double chance = 0.0;
  int duration = 0;
};

class Bullet;
class Snake;
class Weapon;
class WeaponBehavior;

struct WeaponAttackContext {
  std::shared_ptr<Snake> attacker;
  std::shared_ptr<Snake> target;
  std::shared_ptr<Sprite> targetSprite;
  std::shared_ptr<Sprite> attackerSprite;
};

struct WeaponAttackResult {
  bool attacked = false;
};

class WeaponBehavior {
 public:
  virtual ~WeaponBehavior() = default;
  virtual WeaponAttackResult attack(const Weapon& weapon,
                                    const WeaponAttackContext& context) const = 0;
  virtual void onAttack(const Weapon& weapon,
                        const std::shared_ptr<Sprite>& sprite) const = 0;
  virtual bool allowMultiTarget() const = 0;
  virtual bool allowAreaImpact() const = 0;
  virtual void applyImpact(const Weapon& weapon,
                           const std::shared_ptr<Bullet>& bullet,
                           const std::shared_ptr<Snake>& hitSnake,
                           const std::shared_ptr<Sprite>& hitSprite) const = 0;
  virtual void applyAreaImpact(const Weapon& weapon,
                               const std::shared_ptr<Bullet>& bullet,
                               const std::shared_ptr<Snake>& hitSnake,
                               const std::shared_ptr<Sprite>& hitSprite) const = 0;
};

class Weapon {
 public:
  Weapon() = default;
  Weapon(WeaponType type, int shootRange, int effectRange, int damage, int gap,
         int bulletSpeed, const std::shared_ptr<Animation>& birthAnimation,
         const std::shared_ptr<Animation>& deathAnimation,
         const std::shared_ptr<Animation>& flyAnimation, int birthAudio,
         int deathAudio);
  Weapon(const Weapon&) = default;
  Weapon& operator=(const Weapon&) = default;
  Weapon(Weapon&&) noexcept = default;
  Weapon& operator=(Weapon&&) noexcept = default;
  ~Weapon() = default;

  WeaponType type() const;
  int shootRange() const;
  int effectRange() const;
  int damage() const;
  int gap() const;
  int bulletSpeed() const;
  const std::shared_ptr<Animation>& birthAnimation() const;
  const std::shared_ptr<Animation>& deathAnimation() const;
  const std::shared_ptr<Animation>& flyAnimation() const;
  int birthAudio() const;
  int deathAudio() const;
  const std::array<WeaponBuff, BUFF_END>& effects() const;
  std::array<WeaponBuff, BUFF_END>& effects();
  const WeaponBehavior& behavior() const;

  void setType(WeaponType type);
  void setShootRange(int shootRange);
  void setEffectRange(int effectRange);
  void setDamage(int damage);
  void setGap(int gap);
  void setBulletSpeed(int bulletSpeed);
  void setBirthAnimation(const std::shared_ptr<Animation>& animation);
  void setDeathAnimation(const std::shared_ptr<Animation>& animation);
  void setFlyAnimation(const std::shared_ptr<Animation>& animation);
  void setBirthAudio(int audio);
  void setDeathAudio(int audio);
  void setBehavior(const std::shared_ptr<WeaponBehavior>& behavior);

 private:
  WeaponType type_ = WeaponType::SwordPoint;
  int shootRange_ = 0;
  int effectRange_ = 0;
  int damage_ = 0;
  int gap_ = 0;
  int bulletSpeed_ = 0;
  std::shared_ptr<Animation> birthAnimation_{};
  std::shared_ptr<Animation> deathAnimation_{};
  std::shared_ptr<Animation> flyAnimation_{};
  int birthAudio_ = -1;
  int deathAudio_ = -1;
  std::array<WeaponBuff, BUFF_END> effects_{};
  std::shared_ptr<WeaponBehavior> behavior_{};
};

void initWeapons();
std::shared_ptr<WeaponBehavior> makeWeaponBehavior(WeaponType type);

#endif
