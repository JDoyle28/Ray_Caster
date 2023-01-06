#ifndef RAYCAST_H
#define RAYCAST_H


#define NONE 0
#define CAMERA 1
#define SPHERE 2
#define PLANE 3
#define LIGHT 4

/*
Kinds of Objects:
- 0: None
- 1: Camera
- 2: Sphere
- 3: Plane
- 4: Light
*/
typedef struct Object {
   
   int kind;
   float color[3];
   float position[3];
   float diffuseColor[3];
   float specularColor[3];

   union {
      // Camera properties
      struct {
         float width;
         float height;
      };
      // Sphere properties
      struct {
         float radius;
      };
      // Plane properties
      struct {
         float normal[3];
      };
      // Light properties
      struct {
         float lightColor[3];
         float theta;
         float radialA0;
         float radialA1;
         float radialA2;
         float direction[3];
         float angularA0;
         float spotDirection[3];
      };
   };

   } Object;


void help(int errno);
void displayObjects(Object *image, int arrSize);
float getPlaneIntersection(float *origin, float *directionVector, Object *plane);
float getSphereIntersection(float *origin, float *directionVector, Object *sphere);
// void illuminate(float *color, float *dirVector, float *point, int closestIndex);
void illuminate(float *color, float *point, int closestIndex);
float shoot(float *origin, float *dirVector, int currentObject, int *hitObject);

#endif
