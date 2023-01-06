#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "v3math.h"
#include "raycast.h"


Object objects[128];
int numObjects;
int closestObjIndex = 0;


float clamp(float v) {
  if (v > 1) return 1;
  if (v < 0) return 0;
  return v;
}

/*
Function for displaying error messages; Error codes are as follows:
- 0: Incorrect command line input for the program
- 1: Invalid input file
- 2: Invalid output file
*/
void help(int errno) {

   fprintf(stderr, "ERROR: ");

   switch(errno) {
      case 0:
         fprintf(stderr, "Command Format: ./raycast <[width] [height] [input.json] [output.ppm]>");
         break;
      case 1:
         fprintf(stderr, "Input file is invalid");
         break;
      case 2:
         fprintf(stderr, "Output file is invalid");
         break;
   }

   exit(1);
}

void displayObjects(Object *image, int arrSize) {

   Object *obj = NULL;

   for(int i = 0; i < arrSize; i++) {

      obj = &image[i];

      // Camera
      if(obj->kind == 1) {
         printf("%d) CAMERA:\n", i + 1);
         printf("   Width: %f\n   Height: %f\n\n", obj->width, obj->height);
      }
      // Sphere
      else if(obj->kind == 2) {
         printf("%d) SPHERE:\n", i + 1);
         printf("   Position: [%f, %f, %f]\n", obj->position[0], obj->position[1], obj->position[2]);
         printf("   Diffuse Color: [%f, %f, %f]\n", obj->diffuseColor[0], obj->diffuseColor[1], obj->diffuseColor[2]);
         printf("   Specular Color: [%f, %f, %f]\n", obj->specularColor[0], obj->specularColor[1], obj->specularColor[2]);
         printf("   Radius: %f\n\n", obj->radius);
      }
      // Plane
      else if(obj->kind == 3) {
         printf("%d) PLANE:\n", i + 1);
         printf("   Position: [%f, %f, %f]\n", obj->position[0], obj->position[1], obj->position[2]);
         printf("   Diffuse Color: [%f, %f, %f]\n", obj->diffuseColor[0], obj->diffuseColor[1], obj->diffuseColor[2]);
         printf("   Specular Color: [%f, %f, %f]\n", obj->specularColor[0], obj->specularColor[1], obj->specularColor[2]);
         printf("   Normal: [%f, %f, %f]\n\n", obj->normal[0], obj->normal[1], obj->normal[2]);
      }
      // Light
      else if(obj->kind == 4) {
         printf("%d) LIGHT:\n", i + 1);
         printf("   Position: [%f, %f, %f]\n", obj->position[0], obj->position[1], obj->position[2]);
         printf("   Color: [%f, %f, %f]\n", obj->color[0], obj->color[1], obj->color[2]);
         printf("   Direction: [%f, %f, %f]\n", obj->direction[0], obj->direction[1], obj->direction[2]);
         printf("   Spot Direction: [%f, %f, %f]\n", obj->spotDirection[0], obj->spotDirection[1], obj->spotDirection[2]);
         printf("   Radial-a0: %f\n", obj->radialA0);
         printf("   Radial-a1: %f\n", obj->radialA1);
         printf("   Radial-a2: %f\n", obj->radialA2);
         printf("   Angular-a0: %f\n", obj->angularA0);
         printf("   Theta: %f\n\n", obj->theta);
      }
   }
}

// Given an origin and a direction vector, find if any intersections occur with a plane
float getPlaneIntersection(float *origin, float *directionVector, Object *plane) {

   float planeNormal[3] = {plane->normal[0], plane->normal[1], plane->normal[2]};
   float rayD[3];
   float temp[3];
   v3_subtract(temp, origin, plane->position);
   float d = v3_dot_product(planeNormal, temp);

   float closestT = - ((v3_dot_product(planeNormal, origin) + d) \
                    / ( v3_dot_product(planeNormal, directionVector ) ) );
   


   // Intersection point behind origin (camera)
   if (closestT < 0){
      return -1;
   }

   return closestT;
}

// Given an origin and a direction vector, find if any intersections occur with a sphere
float getSphereIntersection(float *origin, float *directionVector, Object *sphere) {

   float a = 0;
   float b = 0;
   float c = 0;
   float t0 = 1000;
   float t1 = 1000;

   // Calculating A, B, and, C for t value
   float dVectorX = directionVector[0];
   float dVectorY = directionVector[1];
   float dVectorZ = directionVector[2];
   a = 1;
   b = 2 * (dVectorX * (origin[0] - sphere->position[0]) + \
            dVectorY * (origin[1] - sphere->position[1]) + \
            dVectorZ * (origin[2] - sphere->position[2]));
   c = pow((origin[0] - sphere->position[0]), 2) + \
       pow((origin[1] - sphere->position[1]), 2) + \
       pow((origin[2] - sphere->position[2]), 2) - \
       pow(sphere->radius, 2);

   float discriminant = (pow(b, 2) - 4 * a * c);
   if(discriminant < 0) {
      return -1;
   }

   // Calulating t values which are a magnitude, but this reduces down to a single float
   t0 = ((-b - sqrt(discriminant)) / (2 * a));
   t1 = ((-b + sqrt(discriminant)) / (2 * a));

   float closestT = t0;

   // Both t0 and t1 are behind the camera, not rendering either
   if ( t0 <= 0 && t1 <= 0 ){
      return -1;
   }
   // t0 behind the camera, t1 is closest
   else if (t0 <= 0 && t1 > 0){
      closestT = t1;
   }
   // t1 behind the camera, t0 is closest
   else if ( t1 <= 0 && t0 > 0){
      closestT = t0;
   }
   // Both t0 and t1 are in front of the camera
   else {
      if(t0 < t1) {
         closestT = t0;
      }
      else {
         closestT = t1;
      }
   }

   return closestT;
}


void illuminate(float *color, float *R0, int closestObj) {

   float illuminationColor[3] = {0, 0, 0};

   // Loop through light array
   // For each light: create light vector L, test to see if objects are in shadow or not -> determines pixel color
   for(int lightInd = 0; lightInd < numObjects; lightInd += 1) {

      float cosTheta = cos(objects[lightInd].theta);

      if(objects[lightInd].kind != 4) continue;

     
      // Calculate new direction vector
      float L[3];
      v3_subtract(L, objects[lightInd].position, R0);

      float lightDistance = v3_length(L);

      v3_normalize(L, L);


      // Call shoot, find intersection point with light position and point
      float t = 0;
      {
	      int newHitObject = 0;
         t = shoot(R0, L, closestObj, &newHitObject);
      }

      // Check if intersection point is lit
      // we have an intersection, so its in the shadow
      if(t > 0 && t < lightDistance) {
         continue;
      }
      
      // Calculate normal vectors
      float normal[3] = {0,0,0};
      if(objects[closestObj].kind == 3) {
         normal[0] = objects[closestObj].normal[0];
         normal[1] = objects[closestObj].normal[1]; // Plane normal
         normal[2] = objects[closestObj].normal[2];
      }
      else if(objects[closestObj].kind == 2) {
         v3_subtract(normal, R0, objects[closestObj].position ); // Sphere normal
      }
      v3_normalize(normal, normal);

      // Calculate diffuse color
      float nDotL = v3_dot_product(normal, L);

      float diffuse[3] = {0,0,0};
      
      if (nDotL > 0){
         diffuse[0] = (objects[closestObj].diffuseColor[0] * objects[lightInd].color[0] * nDotL);
         diffuse[1] = (objects[closestObj].diffuseColor[1] * objects[lightInd].color[1] * nDotL);
         diffuse[2] = (objects[closestObj].diffuseColor[2] * objects[lightInd].color[2] * nDotL);
      }
      else {
         diffuse[0] = 0;
         diffuse[1] = 0;
         diffuse[2] = 0;
      }

      // Calculate specular color
      float V[3] = {R0[0], R0[1], R0[2]};

      float R[3] = {0,0,0};
      v3_reflect(R, L, normal);

      float vDotr = v3_dot_product(V, R);
      float specular[3] = {0,0,0};
      
      if ( vDotr > 0 && nDotL > 0 ){
         specular[0] = (objects[closestObj].specularColor[0] * objects[lightInd].color[0] * pow(vDotr, 20));
         specular[1] = (objects[closestObj].specularColor[1] * objects[lightInd].color[1] * pow(vDotr, 20));
         specular[2] = (objects[closestObj].specularColor[2] * objects[lightInd].color[2] * pow(vDotr, 20));
      }
      else{
         specular[0] = 0;
         specular[1] = 0;
         specular[2] = 0;
      }


      // Calculate radial attenuation
      float radatt = 1 / (objects[lightInd].radialA0 + \
                         (objects[lightInd].radialA1 * lightDistance) + \
                         (pow(objects[lightInd].radialA2 * lightDistance, 2)));


      /*

      // Calculate angular attenuation
      float angatt = 0;
      float v0[3] = {0,0,0};
      float vL[3] =  {objects[lightInd].spotDirection[0], \
                      objects[lightInd].spotDirection[1], \
                      objects[lightInd].spotDirection[2]};

      v3_subtract(v0, R0, objects[lightInd].position);
      v3_normalize(v0, v0);


      float v0dotvL = v3_dot_product(v0, vL);
      float cosAlpha = v0dotvL;
      if (objects[lightInd].theta != 0){
         angatt = pow(v0dotvL, objects[lightInd].angularA0);
      }
      else if (cosAlpha < cosTheta){
         angatt = 0; 
      }
      else{
         angatt = 1;
      }
      */

      // Calculate return color by combining diffuse and specular color
      illuminationColor[0] +=  radatt  * ( diffuse[0]  );
      illuminationColor[1] +=  radatt  * ( diffuse[1]  );
      illuminationColor[2] +=  radatt  * ( diffuse[2]  );

   }

   // Assign color value to return color
   color[0] = illuminationColor[0]; // illuminationColor[0];
   color[1] = illuminationColor[1]; // illuminationColor[1];
   color[2] = illuminationColor[2]; // illuminationColor[2];
}

// Shoots a ray fmakerom origin using dirVector, stores any intersection into T-value
// This function should work correctly, assuming the correct values are handed into is
float shoot(float *origin, float *dirVector, int currentObject, int *hitObject) {

   float closestT = INFINITY;
   *hitObject = -1;

   //printf("%d\n", numObjects);
  
   for(int objIndex = 0; objIndex < numObjects; objIndex++) {
      
      Object *workingObj = &objects[objIndex];

      if (objects[objIndex].kind == 1) continue;
      
      // Skip current object
      if(objIndex == currentObject ) {
         continue;
      }

      // Sphere found
      if(workingObj->kind == 2) {
        
         // The t values are a magnitude, this reduces down to a single float
         float t = getSphereIntersection(origin, dirVector, workingObj);
         
         // Compare the current T found and closest T found to see which point is closer 
         // If it is smaller than our current smallest, reassign
         if (t > 0 && t < closestT){
            
            closestT = t;
            *hitObject = objIndex;
            //printf("obj index in shoot: %d\n", objIndex);
         }
      }
      // Plane found
      else if(workingObj->kind == 3) {      
         float t = getPlaneIntersection(origin, dirVector, workingObj);  
         if (t > 0 && t < closestT){

            closestT = t;
            *hitObject = objIndex;
            //printf("obj index in shoot: %d\n", objIndex);
         }
      }

   }

   // No intersection found
   if (closestT == INFINITY) return -1;

   // intersection found, return the t value
   return closestT;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {

   int imgWidth = atoi(argv[1]);
   
	int imgHeight = atoi(argv[2]);
   uint8_t *image = malloc(sizeof(uint8_t) * imgWidth * imgHeight * 3);

	char *inputFile = argv[3];
   char *outputFile = argv[4];

   char temp1[100];
   char temp2[100];
   char temp3[100];

   FILE *inputFH = fopen(inputFile, "r");
   FILE *outputFH = fopen(outputFile, "w");

   char buf[1000];
   

   printf("\n--------------------------\n");
   printf("Project 4 - Illumination\n");
	printf("--------------------------\n\n");

   // Check for too many or not enough arguments
   if(argc != 5) {
      help(0);
   }
   // Check for starting "./raycast"
   if(strcmp(argv[0], "./raycast") != 0) {
      help(0);
   }

   //fgets(asd, 100, inputFH);
   // Check for valid input.json/input.cvs file
   if(!inputFH) {
      help(1);
   }
   // Check for valid output.ppm file
   if(!outputFH) {
      help(2);
   }


   float camWidth, camHeight;

   float spherePos[3];
   float radius;
   float diffuseColor[3];
   float specularColor[3];

   float planePos[3];
   float planeColor[3];
   float planeDiffuse[3];
   float planeSpecular[3] = {0, 0, 0};
   float normal[3];

   float lightPos[3];
   float lightColor[3];
   float theta, radialA0, radialA1, radialA2;
   float spotDirection[3];
   float angularA0;

   char character;

   char *firstline;

   char str[1000];

   int objIndex = 0;
   Object *camera;
   Object *sphere;
   Object *plane;
   Object *light;
   Object *temp;
   
   // Temporary parsing variables
   char line[1000];
   char objectKind[10];
   char attr1[10];
   char attr2[10];
   char attr3[10];
   char attr4[10];
   char attr5[10];
   char attr6[10];
   char comma[2];
   char bracket[2];
   float tempfloat1 = 0;
   float tempfloat2 = 0;
   float tempfloat3 = 0;
   float *tempV1;
   float *tempV2;
   float *tempV3;
   char *tempPtr;
   char charTemp[2];

   char delim[3] = ", ";

   while(fgets(line, 1000, inputFH)) {

      sscanf(line, "%s", objectKind);
      tempPtr = line;

      // Camera found
      if (strcmp(objectKind, "camera,") == 0){

         //camera = malloc(sizeof(Object));
         camera = &objects[objIndex];
         numObjects += 1;

         sscanf(line, "%s%s%f%s%s%f", objectKind, attr1, &tempfloat1, attr2, comma, &tempfloat2);

         if (strcmp(attr1, "width:") == 0){
            camWidth = tempfloat1;
            camHeight = tempfloat2;
         }

         else{
            camWidth = tempfloat2;
            camHeight = tempfloat1;
         }
         
         camera->kind = 1;
         camera->width = camWidth;
         camera->height = camHeight;
         //objects[objIndex] = *camera;
      }

      // Sphere found
      else if (strcmp(objectKind, "sphere,") == 0){

         //sphere = malloc(sizeof(Object));
         sphere = &objects[objIndex];
         numObjects += 1;

         // splits the line up into an array
         
         tempPtr = strtok(line, delim);
         // loop through the attributes and values from this line
         while (tempPtr != NULL ){
            
            if (strcmp(tempPtr, "radius:") == 0){
               tempPtr = strtok(NULL, delim);
               radius = atof(tempPtr);
            }

            if (strcmp(tempPtr, "diffuse_color:") == 0){
 
               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               diffuseColor[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               diffuseColor[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               diffuseColor[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "specular_color:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               specularColor[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               specularColor[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               specularColor[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "position:") == 0){
  
               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               spherePos[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               spherePos[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               spherePos[2] = atof(tempPtr);
            }
            
            sphere->kind = 2;
            sphere->radius = radius;
            sphere->diffuseColor[0] = diffuseColor[0];
            sphere->diffuseColor[1] = diffuseColor[1];
            sphere->diffuseColor[2] = diffuseColor[2];
            sphere->specularColor[0] = specularColor[0];
            sphere->specularColor[1] = specularColor[1];
            sphere->specularColor[2] = specularColor[2];
            sphere->position[0] = spherePos[0];
            sphere->position[1] = spherePos[1];
            sphere->position[2] = spherePos[2];
            //objects[objIndex] = *sphere;
            
            tempPtr = strtok(NULL, delim);
         }
      }
      // Plane found
      else if (strcmp(objectKind, "plane,") == 0){

         //plane = malloc(sizeof(Object));
         plane = &objects[objIndex];
         numObjects += 1;

         // splits the line up into an array
         char delim[3] = ", ";
         tempPtr = strtok(line, delim);
         // loop through the attributes and values from this line
         while (tempPtr != NULL ){
            
            if (strcmp(tempPtr, "color:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               planeColor[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               planeColor[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               planeColor[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "diffuse_color:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               planeDiffuse[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               planeDiffuse[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               planeDiffuse[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "specular_color:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               planeSpecular[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               planeSpecular[1] = atof(tempPtr);
	       printf("ps: %f\n", planeSpecular[1]);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               planeSpecular[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "position:") == 0){
    
               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               planePos[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               planePos[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               planePos[2] = atof(tempPtr);

            }

            if (strcmp(tempPtr, "normal:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               normal[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               normal[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               normal[2] = atof(tempPtr);

            }

            plane->kind = 3;
            plane->position[0] = planePos[0];
            plane->position[1] = planePos[1];
            plane->position[2] = planePos[2];
            plane->normal[0] = normal[0];
            plane->normal[1] = normal[1];
            plane->normal[2] = normal[2];
            plane->diffuseColor[0] = planeDiffuse[0];
            plane->diffuseColor[1] = planeDiffuse[1];
            plane->diffuseColor[2] = planeDiffuse[2];
            plane->specularColor[0] = planeSpecular[0];
            plane->specularColor[1] = planeSpecular[1];
            plane->specularColor[2] = planeSpecular[2];
            //objects[objIndex] = *plane;

            tempPtr = strtok(NULL, delim);
         }
      }
      // Light found
      else if (strcmp(objectKind, "light,") == 0) {

         //light = malloc(sizeof(Object));
         light = &objects[objIndex];
         numObjects += 1;

         // splits the line up into an array
         char delim[3] = ", ";
         tempPtr = strtok(line, delim);
         // loop through the attributes and values from this line
         while (tempPtr != NULL ){

            if (strcmp(tempPtr, "color:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               lightColor[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               lightColor[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               lightColor[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "theta:") == 0){
               //printf("%s\n", tempPtr);
               tempPtr = strtok(NULL, delim);
               theta = atof(tempPtr);
            }

            if (strcmp(tempPtr, "radial-a2:") == 0){
               tempPtr = strtok(NULL, delim);
               radialA2 = atof(tempPtr);
            }

            if (strcmp(tempPtr, "radial-a1:") == 0){
               //printf("%s\n", tempPtr);
               tempPtr = strtok(NULL, delim);
               radialA1 = atof(tempPtr);
            }

            if (strcmp(tempPtr, "radial-a0:") == 0){
               //printf("%s\n", tempPtr);
               tempPtr = strtok(NULL, delim);
               radialA0 = atof(tempPtr);
            }

            if (strcmp(tempPtr, "position:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               lightPos[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               lightPos[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               lightPos[2] = atof(tempPtr);
            }

            if (strcmp(tempPtr, "angular-a0:") == 0){
               //printf("%s\n", tempPtr);
               tempPtr = strtok(NULL, delim);
               angularA0 = atof(tempPtr);
            }

            if (strcmp(tempPtr, "direction:") == 0){

               // first value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr += 1;
               spotDirection[0] = atof(tempPtr);

               // second value at tempPtr
               tempPtr = strtok(NULL, delim);
               spotDirection[1] = atof(tempPtr);
      
               // third value at tempPtr
               tempPtr = strtok(NULL, delim);
               tempPtr[strlen(tempPtr) - 1] = '\0';
               spotDirection[2] = atof(tempPtr);
            }

            light->kind = 4;
            light->position[0] = lightPos[0];
            light->position[1] = lightPos[1];
            light->position[2] = lightPos[2];
            light->spotDirection[0] = spotDirection[0];
            light->spotDirection[1] = spotDirection[1];
            light->spotDirection[2] = spotDirection[2];
            light->color[0] = lightColor[0];
            light->color[1] = lightColor[1];
            light->color[2] = lightColor[2];
            light->theta = theta;
            light->radialA0 = radialA0;
            light->radialA1 = radialA1;
            light->radialA2 = radialA2;
            light->angularA0 = angularA0;
            //objects[objIndex] = *light;
            tempPtr = strtok(NULL, delim);
         }
      }
      
      tempPtr = strtok(line, delim);
      objIndex += 1;
   }

   displayObjects(objects, numObjects);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


   int imgX, imgY;

   float pixelWidth = camWidth / imgWidth;
   float pixelHeight = camHeight / imgHeight;

   // objIndex = 0;
   Object *workingObj = NULL;
   float rayOrigin[3] = {0, 0, 0};
   float directionVector[3];
   float closestT = 1000;
   float currentT = 1000;
   float p[3];
   int imageArrInd = 0;
   
   // Goes throughout each pixel, checking for intersections
   // Once intersection is found, color pixel with respective color
   // printf("Closest object index: %d\n\n", closestObjIndex);
   
   for(imgY = 0; imgY < imgHeight; imgY += 1) {

      for(imgX = imgWidth; imgX > 0; imgX -= 1) {

         p[0] = 0 - (camWidth / 2) + pixelWidth * (imgX + 0.5);
         p[1] = 0 - (camHeight / 2) + pixelHeight * (imgY + 0.5);
         p[2] = -1;

         float tempColor[3];

         // Calculating Direction Vector
         v3_subtract(directionVector, p, rayOrigin);
         v3_normalize(directionVector, directionVector);
         
         //closestObjIndex = 0;
         // Finding which intersection point is closest to camera for every pixel
         int hitObject;
         closestT = shoot(rayOrigin, directionVector, closestObjIndex, &hitObject);


         //printf("closest T: %f\n", closestT);
         float illumColor[3] = {0,0,0};
         // Plane or Sphere found
         if(closestT > 0) {
	         float intersectCoords[3] = { rayOrigin[0] + (directionVector[0] * closestT), \
                                         rayOrigin[1] + (directionVector[1] * closestT), \
                                         rayOrigin[2] + (directionVector[2] * closestT) };
            
            // Calculating new color after illuminating with light
            illuminate(illumColor, intersectCoords, hitObject);

            // For Illuminate, change diffuseColor -> illumColor
            tempColor[0] = illumColor[0];
            tempColor[1] = illumColor[1];
            tempColor[2] = illumColor[2];
         }
         // None Type found
         else {
            tempColor[0] = 0;
            tempColor[1] = 0;
            tempColor[2] = 0;
         }

         // Placing the color values into the image array to be written to PPM
         image[imageArrInd + 0] = clamp(tempColor[0]) * 255;
         image[imageArrInd + 1] = clamp(tempColor[1]) * 255;
         image[imageArrInd + 2] = clamp(tempColor[2]) * 255;

         imageArrInd += 3;
      }
   }
   
   // Writing to the output.ppm file from the image array
   int ppmWidth = imgWidth;
   int ppmHeight = imgHeight;
   fprintf(outputFH, "%s\n", "P3");
   fprintf(outputFH, "%d %d\n", ppmWidth, ppmHeight);
   fprintf(outputFH, "%d\n", 255);

   for (int i = ppmWidth * ppmHeight * 3; i > 0 ; i -=3){
      fprintf(outputFH, "%d %d %d\n", image[i + 0], image[i + 1], image[i + 2]);
   }

   free(image);
   fclose(inputFH);
   fclose(outputFH);

   return 0;
}
