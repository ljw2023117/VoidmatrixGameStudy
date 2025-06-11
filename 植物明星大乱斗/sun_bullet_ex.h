#pragma once

#include "bullet.h"
#include "animation.h"

extern Atlas atlas_sun_ex;
extern Atlas atlas_sun_ex_explode;

extern Camera main_camera;

class SunBulletEx : public Bullet
{
public:
	SunBulletEx()
	{
		size.x = 288, size.y = 288;

		damage = 20;

		animation_idle.set_atlas(&atlas_sun_ex);
		animation_idle.set_intreval(50);

		animation_explode.set_atlas(&atlas_sun_ex_explode);
		animation_explode.set_intreval(50);
		animation_explode.set_loop(false);
		animation_explode.set_callback([&]() { can_remove = true; });

		IMAGE* frame_idle = animation_idle.get_frame();
		IMAGE* frame_explode = animation_explode.get_frame();
		explode_render_offset.x = (frame_idle->getwidth() - frame_explode->getwidth()) / 2.0f;
		explode_render_offset.y = (frame_idle->getheight() - frame_explode->getheight()) / 2.0f;

	}
	~SunBulletEx() = default;

	void on_collide()
	{
		Bullet::on_collide(); // ���ø������ײ������
		main_camera.shake(20, 350); // ���������
		mciSendString(_T("play sun_explode_ex from 0"), NULL, 0, NULL); // ���ű�ը��Ч
	}

	void on_update(int delta)
	{
		if (!valid)
			animation_explode.on_update(delta); // �����Ч�����±�ը����
		else
		{
			position += velocity * delta;
			animation_idle.on_update(delta); // �������Ĭ�϶���
		}
		if (check_if_exceeds_screen())
			can_remove = true; // ���������Ļ��Χ��������Ƴ�
	}

	void on_draw(const Camera& camera) const
	{
		if (valid)
			animation_idle.on_draw(camera, (int)position.x, (int)position.y); // ����Ĭ�϶���
		else
		{
			animation_explode.on_draw(camera, (int)(position.x + explode_render_offset.x),
				(int)(position.y + explode_render_offset.y)); // ���Ʊ�ը����
		}
		Bullet::on_draw(camera);
	}

private:
	Animation animation_idle;
	Animation animation_explode;
	Vector2 explode_render_offset; // ��ը��������Ⱦƫ����
};
