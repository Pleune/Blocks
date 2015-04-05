#ifndef CUSTOMMATH_H
#define CUSTOMMATH_H

typedef struct {
	int x;
	int y;
	int z;
} int3_t;

typedef struct {
	long x;
	long y;
	long z;
} long3_t;

typedef struct vec2_s {
	float x;
	float y;
} vec2_t;

typedef struct vec3_s {
	float x;
	float y;
	float z;
} vec3_t;

typedef struct vec4_s {
       float x;
       float y;
       float z;
       float w;
} vec4_t;

typedef struct mat3_s {
       float mat[9];
} mat3_t;

typedef struct mat4_s {
	float mat[16];
} mat4_t;

//many of these are not actually coded in yet

//vector stuff
vec3_t *normalvec3(vec3_t *out, vec3_t *in);
vec4_t *normalvec4(vec4_t *out, vec4_t *in);

vec3_t *crossvec3(vec3_t *out, vec3_t *a, vec3_t *b);

//addition functions
vec3_t *addvec3(vec3_t *out, vec3_t *a, vec3_t *b);
vec4_t *addvec4(vec4_t *out, vec4_t *a, vec4_t *b);

mat3_t *addmat3(mat3_t *out, mat3_t *a, mat3_t *b);
mat4_t *addmat4(mat4_t *out, mat4_t *a, mat4_t *b);

//subtraction functions
vec3_t *subtractvec3(vec3_t *out, vec3_t *a, vec3_t *b);
vec4_t *subtractvec4(vec4_t *out, vec4_t *a, vec4_t *b);

mat3_t *subtractmat3(mat3_t *out, mat3_t *a, mat3_t *b);
mat4_t *subtractmat4(mat4_t *out, mat4_t *a, mat4_t *b);

//scaling functions
vec3_t *scalevec3(vec3_t *out, vec3_t *in, float scale);
vec4_t *scalevec4(vec4_t *out, vec4_t *in, float scale);

mat3_t *scalemat3(mat3_t *out, mat3_t *in, float scale);
mat4_t *scalemat4(mat4_t *out, mat4_t *in, float scale);

//dot products
mat4_t *dotmat4mat4(mat4_t *out, mat4_t *a, mat4_t *b);

//more specific transformation functions for use with opengl (mat4 / vec4 only)
mat4_t gettranslatematrix(float x, float y, float z);

mat4_t getrotmatrix(float rx, float ry, float rz);
mat4_t getprojectionmatrix(float fov, float aspect, float far, float near);
mat4_t getviewmatrix(vec3_t eye, vec3_t target, vec3_t up);//lookat right-handed

#endif //CUSTOMMATH_H
