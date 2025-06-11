#pragma once
#include "bullet.h"
#include "animation.h"

extern Atlas atlas_sun;
extern Atlas atlas_sun_explode;

extern Camera main_camera;

class SunBullet : public Bullet
{
public:
	SunBullet()
	{
		size.x = 96, size.y = 96; // 日光炸弹的大小
		damage = 20; // 日光炸弹的伤害

		animation_idle.set_atlas(&atlas_sun);
		animation_idle.set_intreval(50);

		animation_explode.set_atlas(&atlas_sun_explode);
		animation_explode.set_intreval(50);
		animation_explode.set_loop(false);
		animation_explode.set_callback([&]() {
			can_remove = true; // 爆炸动画播放完毕后可以移除
			});

		IMAGE* frame_idle = animation_idle.get_frame();
		IMAGE* frame_explode = animation_explode.get_frame();
		explode_render_offset.x = (frame_idle->getwidth() - frame_explode->getwidth()) / 2.0f;
		explode_render_offset.y = (frame_idle->getheight() - frame_explode->getheight()) / 2.0f;
	}

	~SunBullet() = default;

	void on_collide()
	{
		Bullet::on_collide(); // 调用父类的碰撞处理方法
		main_camera.shake(5, 250); // 主摄像机震动
		mciSendString(_T("play sun_explode from 0"), NULL, 0, NULL); // 播放爆炸音效
	}

	void on_update(int delta)
	{
		if (valid)
		{
			velocity.y += gravity * delta; // 应用重力加速度
			position += velocity * (float)delta; // 更新位置
		}

		if (!valid)
			animation_explode.on_update(delta); // 如果无效，更新爆炸动画
		else
			animation_idle.on_update(delta); // 否则更新默认动画
		if (check_if_exceeds_screen())
			can_remove = true; // 如果超出屏幕范围，则可以移除
	}

	void on_draw(const Camera& camera) const
	{
		if (valid)
		{
			animation_idle.on_draw(camera, (int)position.x, (int)position.y); // 绘制默认动画
		}
		else
		{
			animation_explode.on_draw(camera, (int)(position.x + explode_render_offset.x),
				(int)(position.y + explode_render_offset.y)); // 绘制爆炸动画
		}
		Bullet::on_draw(camera);
	}
private:
	const float gravity = 1e-3f; // 日光炸弹的重力加速度

private:
	Animation animation_idle; // 日光炸弹的默认动画
	Animation animation_explode; // 日光炸弹的爆炸动画
	Vector2 explode_render_offset; // 爆炸动画的渲染偏移量
};
