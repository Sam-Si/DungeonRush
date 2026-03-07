#ifndef SNAKE_ADT_H_
#define SNAKE_ADT_H_

#include <list>
#include <memory>

class Animation;
class Bullet;
class Sprite;

using AnimationList = std::list<std::shared_ptr<Animation>>;
using BulletList = std::list<std::shared_ptr<Bullet>>;
using SpriteList = std::list<std::shared_ptr<Sprite>>;

#endif
