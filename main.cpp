#include <vector>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255,   0,   255);
const int width = 800;
const int height = 800;
Model* model = NULL;

void line(Vec2i t0, Vec2i t1, TGAImage &image, TGAColor color)
{
	bool steep = false;

	// checks if the slope is steep and swaps xs and ys to make it less steep
	if (std::abs(t0.x - t1.x) < std::abs(t0.y - t1.y)) {
		std::swap(t0.x, t0.y);
		std::swap(t1.x, t1.y);
		steep = true;
	}
	// needed to make x0 smaller than x1
	if (t0.x > t1.x) {
		std::swap(t0.x, t1.x);
		std::swap(t0.y, t1.y);
	}

	int dx = t1.x - t0.x;
	int dy = t1.y - t0.y;
	float derror2 = std::abs(dy) * 2;
	float error2 = 0;
	int y = t0.y;

	for (int x = t0.x; x <= t1.x; x++) {
		if (steep)
			image.set(y, x, color);
		else
			image.set(x, y, color);
		
		error2 += derror2;
		
		if (error2 > dx) {
			y += t1.y > t0.y? 1 : -1;
			error2 -= 2*dx;
		}
	}
}

Vec3f barycentric(Vec2i *pts, Vec2i P)
{
	// LEGEND
	// AB = pts[1].x - pts[0].x
	// AC = pts[2].x - pts[0].x
	// PA = pts[0].x - P[0].x
	Vec3f u = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x) ^ Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);

	if (std::abs(u.z) < 1) return Vec3f(-1, 1, -1);
	return Vec3f(1.f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

void triangle(Vec2i *pts, TGAImage& image, TGAColor color)
{
	Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
	Vec2i bboxmax(0, 0);
	Vec2i clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
		bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

		bboxmax.x = std::max(clamp.x, std::min(bboxmax.x, pts[i].x));
		bboxmax.y = std::max(clamp.y, std::min(bboxmax.y, pts[i].y));
	}

	Vec2i P;

	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
			image.set(P.x, P.y, color);
		}
	}
}

int main(int argc, char** argv) {
	model = new Model("african_head.obj");
		
	TGAImage image(width, height, TGAImage::RGB);
	
	// rendering triangles
	Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
	triangle(pts, image, red);

	image.flip_vertically(); 
	image.write_tga_file("output.tga");
	return 0;
}
