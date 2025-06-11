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
		Bullet::on_collide(); // 调用父类的碰撞处理方法
		main_camera.shake(20, 350); // 主摄像机震动
		mciSendString(_T("play sun_explode_ex from 0"), NULL, 0, NULL); // 播放爆炸音效
	}

	void on_update(int delta)
	{
		if (!valid)
			animation_explode.on_update(delta); // 如果无效，更新爆炸动画
		else
		{
			position += velocity * delta;
			animation_idle.on_update(delta); // 否则更新默认动画
		}
		if (check_if_exceeds_screen())
			can_remove = true; // 如果超出屏幕范围，则可以移除
	}

	void on_draw(const Camera& camera) const
	{
		if (valid)
			animation_idle.on_draw(camera, (int)position.x, (int)position.y); // 绘制默认动画
		else
		{
			animation_explode.on_draw(camera, (int)(position.x + explode_render_offset.x),
				(int)(position.y + explode_render_offset.y)); // 绘制爆炸动画
		}
		Bullet::on_draw(camera);
	}

private:
	Animation animation_idle;
	Animation animation_explode;
	Vector2 explode_render_offset; // 爆炸动画的渲染偏移量
};
