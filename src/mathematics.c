// Copyright (C) 2024 Benjamin Froelich
// This file is part of https://github.com/bbeni/2d_moba
// For conditions of distribution and use, see copyright notice in project root.

#include "mathematics.h"
#include <math.h>
#include <assert.h>
#include <float.h>

void clamp(float* v, float lower, float upper) {
	if (*v < lower) {
		*v = lower;
		return;
	}

	if (*v > upper) {
		*v = upper;
	}
}

void clampi(int* v, int lower, int upper) {
	if (*v < lower) {
		*v = lower;
		return;
	}

	if (*v > upper) {
		*v = upper;
	}
}


float lerp(float lower, float upper, float t) {
	return lower * (1.0f - t) + upper * t;
}


void move_towards(float* x, float target, float speed, float dt) {
	if (*x == target) {
		return;
	}
	if (*x > target) {
		*x -= speed * dt;
		if (*x < target) {
			*x = target;
		}
	}
	else {
		*x += speed * dt;
		if (*x > target) {
			*x = target;
		}
	}
}

// move an angle to a range [-pi, pi) closer to the mapped target angle the shorter way around the circle
// @Bug somehow there is and edge case where the target is PI and we go to -PI and never set to PI but it should flip it here
// for now just assume the user of this function is not dependent on the wrap around behaviour
void move_towards_on_circle(float* angle, float target,  float speed, float dt) {

	//*angle = fmod(*angle + M_PI, 2 * M_PI) - M_PI; // map to [-pi, pi)
	// we should use fmod.. but for now thats ok what is done here
	// maybe I am using fmod the wrong way.. the asserts fired using fmod..
	while (*angle >= M_PI) {
		*angle -= 2 * M_PI;
	}
	
	while (*angle < -M_PI) {
		*angle += 2 * M_PI;
	}

	assert(*angle < M_PI);
	assert(*angle >= -M_PI);
	
	if (*angle + M_PI < target) {
		move_towards(angle, target - 2*M_PI, speed, dt);
	} else if (*angle - M_PI > target) {
		move_towards(angle, target + 2*M_PI, speed, dt);
	}
	else {
		move_towards(angle, target, speed, dt);
	}
} 


float angle_between(const Vec2* a, const Vec2* b) {
	float det = a->x * b->y - a->y * b->x;
	float dot = a->x * b->x + a->y * b->y;
	return atan2f(det, dot);
}

void normalize_or_y_axis(Vec2* v) {
	float denom = sqrtf(v->x * v->x + v->y * v->y);
	if (denom == 0.0f) {
		*v = (Vec2){ 0.0f, 1.0f };
		return;
	}
	float factor = 1 / denom;
	v->x *= factor;
	v->y *= factor;
}

void normalize_or_zero(Vec2* v) {
	float denom = sqrtf(v->x * v->x + v->y * v->y);
	if (denom == 0.0f) {
		*v = (Vec2){ 0.0f, 0.0f };
		return;
	}
	float factor = 1 / denom;
	v->x *= factor;
	v->y *= factor;
}

float dot(const Vec2* a, const Vec2* b) {
	return a->x * b->x + a->y * b->y;
}

float length(const Vec2* vec) {
	return sqrtf(dot(vec, vec));}


Vec2 add(const Vec2* v, const Vec2* other) {
    return (Vec2){
	v->x + other->x,
	v->y + other->y,
    };
}
    
Vec2 sub(const Vec2* v, const Vec2* other) {
    return (Vec2){
	v->x - other->x,
	v->y - other->y,
    };
}

Vec2 scale(const Vec2* v, float f) {
    return (Vec2){
        f*v->x,
        f*v->y,
    };
}

bool not_equal(const Vec2* v1, const Vec2* v2) {
    return v1->x != v2->x || v1->y != v2->y;
}

bool equal(const Vec2* v1, const Vec2* v2) {
    return v1->x == v2->x && v1->y == v2->y;
}
