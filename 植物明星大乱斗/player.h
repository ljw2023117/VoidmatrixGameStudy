#pragma once

#include "camera.h"
#include "vector2.h"
#include "animation.h"
#include "player_id.h"
#include "platform.h"
#include "bullet.h"
#include "particle.h"

#include <graphics.h>
#include <iostream>

extern bool is_debug;
extern std::vector<Platform> platforms_list;
extern std::vector<Bullet*> bullet_list;

extern	IMAGE img_1P_cursor;
extern	IMAGE img_2P_cursor;

extern Atlas atlas_run_effect;
extern Atlas atlas_jump_effect;
extern Atlas atlas_land_effect;

class Player
{
public:
	Player(bool facing_right = true) : is_facing_right(facing_right)
	{
		current_animation = is_facing_right ? &animation_idle_right : &animation_idle_left; // 默认动画为朝向右的静止动画

		animation_jump_effect.set_atlas(&atlas_jump_effect);
		animation_jump_effect.set_intreval(25);
		animation_jump_effect.set_loop(false);
		animation_jump_effect.set_callback([&]() {
			is_jump_effect_visible = false;
			});

		animation_land_effect.set_atlas(&atlas_land_effect);
		animation_land_effect.set_intreval(50);
		animation_land_effect.set_loop(false);
		animation_land_effect.set_callback([&]() {
			is_land_effect_visible = false;
			});
		timer_attack_cd.set_wait_time(attack_cd); // 设置普通攻击冷却时间
		timer_attack_cd.set_one_shot(true); // 设置为单次触发
		timer_attack_cd.set_callback([&]() {
			can_attack = true; // 冷却结束后可以攻击
			});

		timer_invulnerable.set_wait_time(750);
		timer_invulnerable.set_one_shot(true);
		timer_invulnerable.set_callback([&]() {
			is_invulnerable = false;
			});

		timer_invulnerable_blink.set_wait_time(75);
		timer_invulnerable_blink.set_callback([&]() {
			is_showing_sketch_frame = !is_showing_sketch_frame;
			});

		timer_run_effect_generation.set_wait_time(75);
		timer_run_effect_generation.set_callback([&]() {
			Vector2 particle_position;
			IMAGE* frame = atlas_run_effect.get_image(0);
			particle_position.x = position.x + (size.x - frame->getwidth()) / 2;
			particle_position.y = position.y + size.y - frame->getheight();
			particle_list.emplace_back(particle_position, &atlas_run_effect, 45);
			});

			
		timer_die_effect_generation.set_wait_time(35);
		timer_die_effect_generation.set_callback([&]() {
			Vector2 particle_position;
			IMAGE* frame = atlas_run_effect.get_image(0);
			particle_position.x = position.x + (size.x - frame->getwidth()) / 2;
			particle_position.y = position.y + size.y - frame->getheight();
			particle_list.emplace_back(particle_position, &atlas_run_effect, 150);
			});

		timer_cursor_visibility.set_wait_time(2500);
		timer_cursor_visibility.set_one_shot(true);
		timer_cursor_visibility.set_callback([&]() {
			is_cursor_visible = false;
			});
	}
	~Player() = default;

	virtual void on_update(int delta)
	{
		int diraction = is_right_key_down - is_left_key_down; // 1:向右, -1:向左, 0:不动
		if (diraction != 0)
		{
			if (!is_attacking_ex)
				is_facing_right = diraction > 0; // 更新朝向
			current_animation = is_facing_right ? &animation_run_right : &animation_run_left; // 切换到奔跑动画
			float distance = diraction * run_velocity * delta;
			on_run(distance);
		}
		else
		{
			current_animation = is_facing_right ? &animation_idle_right : &animation_idle_left; // 切换到静止动画
			timer_run_effect_generation.pause();
		}

		if (is_attacking_ex)
			current_animation = is_facing_right ? &animation_attack_ex_right : &animation_attack_ex_left;

		if (hp <= 0) current_animation = last_hurt_direction.x < 0 ? &animation_die_left : &animation_die_right;

		current_animation->on_update(delta); // 更新当前动画
		animation_jump_effect.on_update(delta);
		animation_land_effect.on_update(delta);

		timer_attack_cd.on_update(delta); // 更新攻击冷却定时器
		timer_invulnerable.on_update(delta);
		timer_invulnerable_blink.on_update(delta);
		timer_run_effect_generation.on_update(delta);
		timer_cursor_visibility.on_update(delta);

		if (hp <= 0) timer_die_effect_generation.on_update(delta);

		particle_list.erase(std::remove_if(particle_list.begin(), particle_list.end(),
			[](const Particle& particle) {
				return !particle.check_valid();
			}), particle_list.end());

		for (auto& particle : particle_list)
			particle.on_update(delta);

		if (is_showing_sketch_frame)
			sketch_image(current_animation->get_frame(), &img_sketch);

		move_and_collide(delta);
	}

	virtual void on_draw(const Camera& camera)
	{
		if (is_jump_effect_visible)
			animation_jump_effect.on_draw(camera, (int)position_jump_effect.x, (int)position_jump_effect.y);
		if (is_land_effect_visible)
			animation_land_effect.on_draw(camera, (int)position_land_effect.x, (int)position_land_effect.y);

		for (auto& particle : particle_list)
			particle.on_draw(camera);

		if (hp > 0 && is_invulnerable && is_showing_sketch_frame)
			putimage_alpha(camera, (int)position.x, (int)position.y, &img_sketch);
		else
			current_animation->on_draw(camera, (int)position.x, (int)position.y); // 绘制当前动画

		if (is_cursor_visible)
		{
			switch (id)
			{
			case PlayerID::P1:
				putimage_alpha(camera, (int)(position.x + (size.x - img_1P_cursor.getheight()) / 2),
					(int)(position.y - img_1P_cursor.getheight()), &img_1P_cursor);
				break;
			case PlayerID::P2:
				putimage_alpha(camera, (int)(position.x + (size.x - img_2P_cursor.getheight()) / 2),
					(int)(position.y - img_2P_cursor.getheight()), &img_2P_cursor);
				break;
			}
		}

		if (is_debug)
		{
			setlinecolor(RGB(0, 125, 255));
			rectangle((int)position.x, (int)position.y, (int)position.x + size.x, (int)position.y + size.y);
		}
	}

	virtual void on_input(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (id)
			{
			case PlayerID::P1:
				switch (msg.vkcode)
				{
				case 0x41: // A key
					is_left_key_down = true;
					break;
				case 0x44: // D key
					is_right_key_down = true;
					break;
				case 0x57:	// W key
					on_jump();
					break;
				case 0x46:	// F key
					if (can_attack)
					{
						on_attack();
						can_attack = false; // 设置为不可攻击状态
						timer_attack_cd.restart(); // 启动攻击冷却定时器
					}
					break;
				case 0x47: // G key
					if (mp >= 100)
					{
						on_attack_ex();
						mp = 0;
					}
					break;
				}
				break;
			case PlayerID::P2:
				switch (msg.vkcode)
				{
				case VK_LEFT:
					is_left_key_down = true;
					break;
				case VK_RIGHT:
					is_right_key_down = true;
					break;
				case VK_UP:
					on_jump();
					break;
				case VK_OEM_PERIOD: // . key
					if (can_attack)
					{
						on_attack();
						can_attack = false; // 设置为不可攻击状态
						timer_attack_cd.restart(); // 启动攻击冷却定时器
					}
					break;
				case VK_OEM_2:		// / key
					if (mp >= 100)
					{
						on_attack_ex();
						mp = 0;
					}
					break;
				}
				break;
			default:
				break;
			}
			break;
		case WM_KEYUP:
			switch (id)
			{
			case PlayerID::P1:
				switch (msg.vkcode)
				{
				case 0x41: // A key
					is_left_key_down = false;
					break;
				case 0x44: // D key
					is_right_key_down = false;
					break;
				}
				break;
			case PlayerID::P2:
				switch (msg.vkcode)
				{
				case VK_LEFT:
					is_left_key_down = false;
					break;
				case VK_RIGHT:
					is_right_key_down = false;
					break;
				}
				break;
			}
			break;
		}

	}

	void set_id(PlayerID playerid)
	{
		id = playerid;
	}

	void set_position(float x, float y)
	{
		position.x = x, position.y = y;
	}

	const Vector2& get_position() const
	{
		return position;
	}

	const Vector2& get_size() const
	{
		return size;
	}

	void set_hp(int val)
	{
		hp = val;
	}

	int get_hp()
	{
		return hp;
	}

	int get_mp()
	{
		return mp;
	}

	virtual void on_run(float distance)
	{
		if (is_attacking_ex) return;
		position.x += distance;
		timer_run_effect_generation.resume();
	}

	virtual void on_jump()
	{
		if (velocity.y != 0 || is_attacking_ex) // 如果当前已经在跳跃中，则不允许再次跳跃
			return;
		velocity.y += jump_velocity;
		is_jump_effect_visible = true;
		animation_jump_effect.reset();

		IMAGE* effect_frame = animation_jump_effect.get_frame();
		position_jump_effect.x = position.x + (size.x - effect_frame->getwidth()) / 2;
		position_jump_effect.y = position.y + size.y - effect_frame->getheight();

	}

	virtual void on_land()
	{
		is_land_effect_visible = true;
		animation_land_effect.reset();

		IMAGE* effect_frame = animation_land_effect.get_frame();
		position_land_effect.x = position.x + (size.x - effect_frame->getwidth()) / 2;
		position_land_effect.y = position.y + size.y - effect_frame->getheight();
	}

	void make_invulnerable()
	{
		is_invulnerable = true;
		timer_invulnerable.restart();
	}

	virtual void on_attack() {}
	virtual void on_attack_ex() {}
protected:
	void move_and_collide(int delta)
	{
		float last_velocity_y = velocity.y;
		// 模拟重力作用
		velocity.y += gravity * delta;
		position += velocity * (float)delta;

		if (hp <= 0) return;
		// 检测与平台的碰撞
		if (velocity.y > 0)
		{
			for (const Platform& platform : platforms_list)
			{
				const Platform::CollisionShape& shape = platform.shape;
				bool is_collided_x = (max(position.x + size.x, shape.right) - min(position.x, shape.left)) <= size.x + shape.right - shape.left;
				bool is_collided_y = (shape.y >= position.y && shape.y <= position.y + size.y);
				if (is_collided_x && is_collided_y)
				{
					float last_tick_foot_pos_y = position.y + size.y - velocity.y * (float)delta;
					if (last_tick_foot_pos_y <= shape.y)
					{
						position.y = shape.y - size.y; // 碰撞后将角色位置调整到平台上方
						velocity.y = 0; // 重置垂直速度

						if (last_velocity_y != 0)
							on_land();
						break;
					}
				}
			}
		}

		if (!is_invulnerable)
		{
			for (auto bullet : bullet_list)
			{
				if (!bullet->get_valid() || bullet->get_collide_target() != id)
					continue;

				if (bullet->check_collision(position, size))
				{
					make_invulnerable();
					bullet->on_collide();
					bullet->set_valid(false);
					hp -= bullet->get_damage();
					last_hurt_direction = bullet->get_position() - position;
					if (hp <= 0)
					{
						velocity.x = last_hurt_direction.x < 0 ? 0.35f : -0.35f;
						velocity.y = -1.0f;
					}
				}
			}
		}
	}

protected:
	Vector2 size;							// 角色尺寸
	Vector2 position;						// 角色位置
	Vector2 velocity;						// 角色速度

	int mp = 0;								// 角色能量
	int hp = 100;							// 角色生命值

	const float gravity = 1.6e-3f;			// 玩家重力
	const float run_velocity = 0.55f;		// 跑动速度
	const float jump_velocity = -0.85f;		// 跳跃速度

	Animation animation_idle_left;			// 朝向左的默认动画
	Animation animation_idle_right;			// 朝向右的默认动画
	Animation animation_run_left;			// 朝向左的奔跑动画
	Animation animation_run_right;			// 朝向右的奔跑动画
	Animation animation_attack_ex_left;		// 朝向左的特殊攻击动画
	Animation animation_attack_ex_right;	// 朝向右的特殊攻击动画
	Animation animation_jump_effect;		// 跳跃特效动画
	Animation animation_land_effect;		// 落地特效动画
	Animation animation_die_left;			// 朝向左的死亡动画
	Animation animation_die_right;			// 朝向右的死亡动画

	bool is_jump_effect_visible = false;	// 跳跃动画是否可见
	bool is_land_effect_visible = false;	// 落地动画是否可见

	Vector2 position_jump_effect;			// 跳跃动画播放位置
	Vector2 position_land_effect;			// 落地动画播放位置

	Animation* current_animation = nullptr; // 当前正在播放的动画

	PlayerID id = PlayerID::P1;				// 玩家序号 ID

	bool is_left_key_down = false;			// 左键是否按下
	bool is_right_key_down = false;			// 右键是否按下

	bool is_facing_right = true;			// 是否面向右侧

	int attack_cd = 500;					// 普通攻击冷却时间
	bool can_attack = true;					// 是否可以普通攻击
	Timer timer_attack_cd;					// 普通攻击冷却定时器

	bool is_attacking_ex = false;			// 是否正在特殊攻击

	IMAGE img_sketch;						// 动画帧剪影图片
	bool is_invulnerable = false;			// 角色是否处于无敌状态
	bool is_showing_sketch_frame = false;	// 当前帧是否应该显示剪影
	Timer timer_invulnerable;				// 无敌状态定时器
	Timer timer_invulnerable_blink;			// 无敌状态闪烁定时器

	std::vector<Particle> particle_list;	// 粒子对象列表

	Timer timer_run_effect_generation;		// 跑动特效粒子发射定时器
	Timer timer_die_effect_generation;		// 死亡特效粒子发射定时器

	bool is_cursor_visible = true;			// 玩家光标指示器是否显示
	Timer timer_cursor_visibility;			// 玩家光标指示器可见性定时器

	Vector2 last_hurt_direction;			// 最后一次受击的方向
};

