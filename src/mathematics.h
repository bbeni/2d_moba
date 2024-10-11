// Copyright (C) 2024 Benjamin Froelich
// This file is part of https://github.com/bbeni/2d_moba
// For conditions of distribution and use, see copyright notice in project root.

#ifndef _MATHEMATICS_H
#define _MATHEMATICS_H

#include <math.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi from math.h */
#endif

#define DEG_TO_RAD M_PI / 180

// Vector2 stuff

typedef struct Vec2 {
	float x, y;
} Vec2;

void clamp(float* v, float lower, float upper);
void clampi(int* v, int lower, int upper);
float lerp(float lower, float upper, float t);
void move_towards(float* x, float target, float speed, float dt);
void move_towards_on_circle(float* angle, float target,  float speed, float dt); // move towards the closer angle mapped from -pi to pi

float dot(const Vec2* a, const Vec2* b);
float length(const Vec2* vec);
float angle_between(const Vec2* a, const Vec2* b); // -pi to pi
void normalize_or_y_axis(Vec2* v);
void normalize_or_zero(Vec2* v);

Vec2 add(const Vec2* v, const Vec2* other);
Vec2 sub(const Vec2* v, const Vec2* other);
Vec2 scale(const Vec2* v, float f);
bool not_equal(const Vec2* v1, const Vec2* other);
bool equal(const Vec2* v1, const Vec2* other);

#endif // _MATHEMATICS_H
