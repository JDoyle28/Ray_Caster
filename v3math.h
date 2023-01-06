#ifndef V3MATH_H
#define V3MATH_H

#include "v3math.h"
#include "raycast.h"
#include <stdbool.h>

void v3_add(float *dst, float *a, float *b);
float v3_angle(float *a, float *b); // angle between a and b
float v3_angle_quick(float *a, float *b); // angle between a and b; no cos-1
void v3_cross_product(float *dst, float *a, float *b);
float v3_dot_product(float *a, float *b);
bool v3_equals(float *a, float *b, float tolerance);
void v3_from_points(float *dst, float *head, float *tail);
float v3_length(float *a);
void v3_normalize(float *dst, float *a);
void v3_reflect(float *dst, float *v, float *n);
void v3_scale(float *dst, float s);
void v3_subtract(float *dst, float *a, float *b);

#endif
