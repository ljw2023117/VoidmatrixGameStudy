#pragma once
#include "util.h"
#include "camera.h"

extern bool is_debug;

class Platform
{
public:
	struct CollisionShape
	{
		float y;
		float left, right;
	};
public:
	CollisionShape shape;
	IMAGE* img = nullptr;
	POINT render_position = { 0 };
public:
	Platform() = default;
	~Platform() = default;

	void on_draw(const Camera& camera) const
	{
		putimage_alpha(camera, render_position.x, render_position.y, img);
		if (is_debug)
		{
			setlinecolor(RGB(255, 0, 0));
			line(camera, (int)shape.left, (int)shape.y, (int)shape.right, (int)shape.y);
		}
	}

};