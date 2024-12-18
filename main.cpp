﻿#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255,   0,   255);
const int width = 800;
const int height = 800;
Vec3f light_dir(0, 0, -1);
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

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P)
{
	Vec3f s[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	Vec3f u = cross(s[0], s[1]);
	if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(Vec3f *pts, float *zbuffer, TGAImage& image, TGAColor color)
{
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
		}
	}

	Vec3f P;

	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

			P.z = 0;
			for (int i = 0; i < 3; i++) P.z += pts[i][2] * bc_screen[i];
			if (zbuffer[int(P.x + P.y * width)] < P.z) {
				zbuffer[int(P.x + P.y * width)] = P.z;
				image.set(P.x, P.y, color);
			}
		}
	}
}
Vec3f world2screen(Vec3f v) {
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}
int main(int argc, char** argv) {
	model = new Model("african_head.obj");
		
	TGAImage image(width, height, TGAImage::RGB);
	
	float* zbuffer = new float[width * height];

	for (int i = 0; i < model->nfaces(); i++) {
		// Add debug output here
		std::cout << "Painting face: " << i << std::endl;
		std::vector<int> face = model->face(i);

		Vec3f pts[3];
		Vec3f world_coords[3];

		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			pts[j] = world2screen(v);
			world_coords[j] = v;
		}

		Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
		n.normalize();

		float intensity = n * light_dir;

		triangle(pts, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));	}

	image.flip_vertically(); 
	image.write_tga_file("output.tga");

	return 0;
}
