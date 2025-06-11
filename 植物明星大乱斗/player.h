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
		current_animation = is_facing_right ? &animation_idle_right : &animation_idle_left; // Ĭ�϶���Ϊ�����ҵľ�ֹ����

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
		timer_attack_cd.set_wait_time(attack_cd); // ������ͨ������ȴʱ��
		timer_attack_cd.set_one_shot(true); // ����Ϊ���δ���
		timer_attack_cd.set_callback([&]() {
			can_attack = true; // ��ȴ��������Թ���
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
		int diraction = is_right_key_down - is_left_key_down; // 1:����, -1:����, 0:����
		if (diraction != 0)
		{
			if (!is_attacking_ex)
				is_facing_right = diraction > 0; // ���³���
			current_animation = is_facing_right ? &animation_run_right : &animation_run_left; // �л������ܶ���
			float distance = diraction * run_velocity * delta;
			on_run(distance);
		}
		else
		{
			current_animation = is_facing_right ? &animation_idle_right : &animation_idle_left; // �л�����ֹ����
			timer_run_effect_generation.pause();
		}

		if (is_attacking_ex)
			current_animation = is_facing_right ? &animation_attack_ex_right : &animation_attack_ex_left;

		if (hp <= 0) current_animation = last_hurt_direction.x < 0 ? &animation_die_left : &animation_die_right;

		current_animation->on_update(delta); // ���µ�ǰ����
		animation_jump_effect.on_update(delta);
		animation_land_effect.on_update(delta);

		timer_attack_cd.on_update(delta); // ���¹�����ȴ��ʱ��
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
			current_animation->on_draw(camera, (int)position.x, (int)position.y); // ���Ƶ�ǰ����

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
						can_attack = false; // ����Ϊ���ɹ���״̬
						timer_attack_cd.restart(); // ����������ȴ��ʱ��
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
						can_attack = false; // ����Ϊ���ɹ���״̬
						timer_attack_cd.restart(); // ����������ȴ��ʱ��
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
		if (velocity.y != 0 || is_attacking_ex) // �����ǰ�Ѿ�����Ծ�У��������ٴ���Ծ
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
		// ģ����������
		velocity.y += gravity * delta;
		position += velocity * (float)delta;

		if (hp <= 0) return;
		// �����ƽ̨����ײ
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
						position.y = shape.y - size.y; // ��ײ�󽫽�ɫλ�õ�����ƽ̨�Ϸ�
						velocity.y = 0; // ���ô�ֱ�ٶ�

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
	Vector2 size;							// ��ɫ�ߴ�
	Vector2 position;						// ��ɫλ��
	Vector2 velocity;						// ��ɫ�ٶ�

	int mp = 0;								// ��ɫ����
	int hp = 100;							// ��ɫ����ֵ

	const float gravity = 1.6e-3f;			// �������
	const float run_velocity = 0.55f;		// �ܶ��ٶ�
	const float jump_velocity = -0.85f;		// ��Ծ�ٶ�

	Animation animation_idle_left;			// �������Ĭ�϶���
	Animation animation_idle_right;			// �����ҵ�Ĭ�϶���
	Animation animation_run_left;			// ������ı��ܶ���
	Animation animation_run_right;			// �����ҵı��ܶ���
	Animation animation_attack_ex_left;		// ����������⹥������
	Animation animation_attack_ex_right;	// �����ҵ����⹥������
	Animation animation_jump_effect;		// ��Ծ��Ч����
	Animation animation_land_effect;		// �����Ч����
	Animation animation_die_left;			// ���������������
	Animation animation_die_right;			// �����ҵ���������

	bool is_jump_effect_visible = false;	// ��Ծ�����Ƿ�ɼ�
	bool is_land_effect_visible = false;	// ��ض����Ƿ�ɼ�

	Vector2 position_jump_effect;			// ��Ծ��������λ��
	Vector2 position_land_effect;			// ��ض�������λ��

	Animation* current_animation = nullptr; // ��ǰ���ڲ��ŵĶ���

	PlayerID id = PlayerID::P1;				// ������ ID

	bool is_left_key_down = false;			// ����Ƿ���
	bool is_right_key_down = false;			// �Ҽ��Ƿ���

	bool is_facing_right = true;			// �Ƿ������Ҳ�

	int attack_cd = 500;					// ��ͨ������ȴʱ��
	bool can_attack = true;					// �Ƿ������ͨ����
	Timer timer_attack_cd;					// ��ͨ������ȴ��ʱ��

	bool is_attacking_ex = false;			// �Ƿ��������⹥��

	IMAGE img_sketch;						// ����֡��ӰͼƬ
	bool is_invulnerable = false;			// ��ɫ�Ƿ����޵�״̬
	bool is_showing_sketch_frame = false;	// ��ǰ֡�Ƿ�Ӧ����ʾ��Ӱ
	Timer timer_invulnerable;				// �޵�״̬��ʱ��
	Timer timer_invulnerable_blink;			// �޵�״̬��˸��ʱ��

	std::vector<Particle> particle_list;	// ���Ӷ����б�

	Timer timer_run_effect_generation;		// �ܶ���Ч���ӷ��䶨ʱ��
	Timer timer_die_effect_generation;		// ������Ч���ӷ��䶨ʱ��

	bool is_cursor_visible = true;			// ��ҹ��ָʾ���Ƿ���ʾ
	Timer timer_cursor_visibility;			// ��ҹ��ָʾ���ɼ��Զ�ʱ��

	Vector2 last_hurt_direction;			// ���һ���ܻ��ķ���
};

