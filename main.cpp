#include <vector>
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
const int depth = 255;

Vec3f light_dir(0, 0, -1);
Model* model = NULL;
Vec3f camera(0, 0, 3);

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

Vec3f m2v(Matrix m)
{
	return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

Matrix v2m(Vec3f v)
{
	Matrix m(4, 1);

	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.f;

	return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P)
{
	Vec3f s[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	Vec3f u = s[0]^s[1];
	if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizer
}

void triangle(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, TGAImage &image, float intensity, int *zbuffer) {
	if (t0.y==t1.y && t0.y==t2.y) return; // i dont care about degenerate triangles
    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
    if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
    if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here
        Vec3i A = t0  + Vec3f(t2-t0  )*alpha;
        Vec3i B = second_half ? t1  + Vec3f(t2-t1  )*beta : t0  + Vec3f(t1-t0  ) * beta;
        Vec2i uvA = uv0 + (uv2-uv0)*alpha;
        Vec2i uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0) * beta;
        if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }
        for (int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            Vec3i   P = Vec3f(A) + Vec3f(B-A)*phi;
            Vec2i uvP =     uvA +   (uvB-uvA)*phi;
            int idx = P.x+P.y*width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = model->diffuse(uvP);
                image.set(P.x, P.y, TGAColor(color.r*intensity, color.g*intensity, color.b*intensity));
            }
        }
    }
}

int main(int argc, char** argv) {
	model = new Model("obj/model/african_head.obj");
	TGAImage image(width, height, TGAImage::RGB);
	
	int* zbuffer = new int[width * height];
	std::fill(zbuffer, zbuffer + width * height, std::numeric_limits<int>::min());

	Matrix Projection = Matrix::identity(4);
	Matrix ViewPort   = viewport(width/8, height/8, width*3/4, height*3/4);

	Projection[3][2] = -1.f/camera.z;

	// draw the model
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);

		Vec3f screen_coords[3];
		Vec3f world_coords[3];
		
		// looping vertices in face
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			screen_coords[j] = m2v(ViewPort*Projection*v2m(v));
			world_coords[j] = v;
		}

		Vec3f n = (world_coords[2] - world_coords[0])^(world_coords[1] - world_coords[0]);
		n.normalize();

		float intensity = n * light_dir;
		
		if (intensity > 0) {
			Vec2i uv[3];
			
			for (int k = 0; k < 3; k++)
				uv[k] = model->uv(i, k);
			
			triangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
		}
	}	
	
	image.flip_vertically(); 
	image.write_tga_file("output.tga");

	return 0;
}
