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

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color)
{
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);

	int triangle_height = t2.y - t0.y;
	
	for (int i = 0; i < triangle_height; ++i) {
		bool upper_segment = i > (t1.y - t0.y) || t1.y == t0.y;
		int segment_height = upper_segment ? t2.y - t1.y : t1.y - t0.y;

		float alpha = (float) i / triangle_height;
		float beta = (float) (i - (upper_segment ? (t1.y - t0.y) : 0 )) / segment_height;

		Vec2i A = t0 + (t2 - t0) * alpha;
		Vec2i B = upper_segment ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;

		if (A.x > B.x) std::swap(A, B);
		
		for (int j = A.x; j <= B.x; ++j) {
			image.set(j, t0.y + i, color);			
		}
	}
}

int main(int argc, char** argv) {
	model = new Model("african_head.obj");
		
	TGAImage image(width, height, TGAImage::RGB);
	
	// rendering triangles
	Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
	Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
	Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
	triangle(t0[0], t0[1], t0[2], image, red);
	triangle(t1[0], t1[1], t1[2], image, white);
	triangle(t2[0], t2[1], t2[2], image, green);

	image.flip_vertically(); 
	image.write_tga_file("output.tga");
	return 0;
}
