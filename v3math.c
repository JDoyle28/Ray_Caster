#include "v3math.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>


void v3_add(float *dst, float *a, float *b) {
   dst[0] = a[0] + b[0];
   dst[1] = a[1] + b[1];
   dst[2] = a[2] + b[2];
}

float v3_angle(float *a, float *b) {
   float angle = ( acosf(v3_dot_product(a,b) / ( v3_length(b) * v3_length(a) ) )  )* 180/M_PI;
   return angle;
}

float v3_angle_quick(float *a, float *b) {
   float dot = v3_dot_product(a, b);
   float result = (v3_length(a) * v3_length(b));
   printf("Dot prod: %f\n", dot);      // These two lines are for testing purposes
   printf("||a|| * ||b||: %f\n", result);
   float angle = ( dot / result ) * 180/M_PI;

   return angle;
}

void v3_cross_product(float *dst, float *a, float *b) {
   dst[0] = (a[1] * b[2]) - (a[2] * b[1]); 
   dst[1] = (a[2] * b[0]) - (a[0] * b[2]); 
   dst[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

float v3_dot_product(float *a, float *b) {  
   float product = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
   return product;
}

// Compares two vectors, testing each coordinate +- a given tolerance
// Returns bool value of whether they are equal or not
bool v3_equals(float *a, float *b, float tolerance) {
    float min_a[] = {a[0] - tolerance, a[1] - tolerance, a[2] - tolerance};
    float max_a[] = {a[0] + tolerance, a[1] + tolerance, a[2] + tolerance};  
    for(int i = 0; i < 3; i++) {
        if(b[i] < min_a[i] || b[i] > max_a[i]) {
            return false;
        }
    }
    return true;
}

void v3_from_points(float *dst, float *head, float *tail) {
   dst[0] = tail[0] - head[0];
   dst[1] = tail[1] - head[1];
   dst[2] = tail[2] - head[2];
}

float v3_length(float *a) {
   float length = sqrt(pow(a[0], 2) + pow(a[1], 2) + pow(a[2], 2));
   return length;
}

void v3_normalize(float *dst, float *a) {
   dst[0] = a[0] / v3_length(a);
   dst[1] = a[1] / v3_length(a);
   dst[2] = a[2] / v3_length(a);
}

void v3_reflect(float *dst, float *v, float *n) {
   float temp = 2 * v3_dot_product(v, n);
   float *temp2 = n;
   v3_scale(temp2, temp);
   v3_subtract(dst, v, temp2);
}

void v3_scale(float *dst, float s) {
   dst[0] = dst[0] * s;
   dst[1] = dst[1] * s;
   dst[2] = dst[2] * s;
}

void v3_subtract(float *dst, float *a, float *b) {
   dst[0] = a[0] - b[0];
   dst[1] = a[1] - b[1];
   dst[2] = a[2] - b[2];
}
