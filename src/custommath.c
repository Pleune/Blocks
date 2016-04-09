#include "custommath.h"

#include <string.h>
#include <math.h>

int
imin(int a, int b)
{
	if(a < b)
		return a;
	else
		return b;
}

int
imax(int a, int b)
{
	if(a > b)
		return a;
	else
		return b;
}

long double *
distlong3(long double *out, long3_t *a, long3_t *b)
{
	long3_t a_ = *a;
	a_.x -= b->x;
	a_.y -= b->y;
	a_.z -= b->z;
	*out = sqrtl(a_.x*a_.x + a_.y*a_.y + a_.z*a_.z);
	return out;
}


vec3_t *
normalvec3(vec3_t *out, vec3_t *in)
{
	float mag = sqrt( (in->x)*(in->x) + (in->y)*(in->y) + (in->z)*(in->z));
	out->x = in->x / mag;
	out->y = in->y / mag;
	out->z = in->z / mag;
	return out;
}

vec3_t *
crossvec3(vec3_t *out, vec3_t *a, vec3_t *b)
{
	//incase out is a or b
	vec3_t intermediary;
	intermediary.x = (a->y)*(b->z) - (a->z)*(b->y);
	intermediary.y = (a->z)*(b->x) - (a->x)*(b->z);
	intermediary.z = (a->x)*(b->y) - (a->y)*(b->x);

	out->x = intermediary.x;
	out->y = intermediary.y;
	out->z = intermediary.z;

	return out;
}

vec3_t *
subtractvec3(vec3_t *out, vec3_t *a, vec3_t *b)
{
	out->x = a->x - b->x;
	out->y = a->y - b->y;
	out->z = a->z - b->z;
	return out;
}

mat4_t *
dotmat4mat4(mat4_t *out, mat4_t *a, mat4_t *b)
{
	float mat[16];

	const float *m1 = a->mat, *m2 = b->mat;

	mat[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
	mat[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
	mat[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
	mat[3] = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];
	mat[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
	mat[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
	mat[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
	mat[7] = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];
	mat[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
	mat[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
	mat[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
	mat[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];
	mat[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
	mat[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
	mat[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
	mat[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];

	memcpy(out->mat, mat, sizeof(float)*16);

	return out;
}

mat4_t
getprojectionmatrix(float fov, float aspect, float far, float near)
{
	float top = near * tan(0.00872664625/*(pi/180)/2*/ * fov);

	float right = top * aspect;

	mat4_t mat;

	//all of these are always 0
	mat.mat[1] = 0;
	mat.mat[2] = 0;
	mat.mat[3] = 0;
	mat.mat[4] = 0;
	mat.mat[6] = 0;
	mat.mat[7] = 0;
	mat.mat[12] = 0;
	mat.mat[13] = 0;
	mat.mat[15] = 0;

	mat.mat[0] = near/right;//(2*near)/(right-left)
	mat.mat[5] = near/top;//(2*near)/(top-bottom)
	mat.mat[8] = 0;//(right+left)/(right-left)
	mat.mat[9] = 0;//(top+bottom)/(top-bototm)
	mat.mat[10] = -(far+near)/(far-near);
	mat.mat[11] = -1;
	mat.mat[14] = -(2*far*near)/(far-near);

	return mat;
}

mat4_t
getviewmatrix(vec3_t eye, vec3_t target, vec3_t up)
{
	vec3_t xaxis, yaxis, zaxis;

	normalvec3(&zaxis, subtractvec3(&zaxis, &eye, &target));
	normalvec3(&xaxis, crossvec3(&xaxis, &up, &zaxis));
	crossvec3(&yaxis, &zaxis, &xaxis);

	mat4_t orientation;
	orientation.mat[0] = xaxis.x;
	orientation.mat[1] = yaxis.x;
	orientation.mat[2] = zaxis.x;
	orientation.mat[3] = 0;
	orientation.mat[4] = xaxis.y;
	orientation.mat[5] = yaxis.y;
	orientation.mat[6] = zaxis.y;
	orientation.mat[7] = 0;
	orientation.mat[8] = xaxis.z;
	orientation.mat[9] = yaxis.z;
	orientation.mat[10] = zaxis.z;
	orientation.mat[11] = 0;
	orientation.mat[12] = 0;
	orientation.mat[13] = 0;
	orientation.mat[14] = 0;
	orientation.mat[15] = 1;

	mat4_t translation;
	translation.mat[0] = 1;
	translation.mat[1] = 0;
	translation.mat[2] = 0;
	translation.mat[3] = 0;
	translation.mat[4] = 0;
	translation.mat[5] = 1;
	translation.mat[6] = 0;
	translation.mat[7] = 0;
	translation.mat[8] = 0;
	translation.mat[9] = 0;
	translation.mat[10] = 1;
	translation.mat[11] = 0;
	translation.mat[12] = -eye.x;
	translation.mat[13] = -eye.y;
	translation.mat[14] = -eye.z;
	translation.mat[15] = 1;

	dotmat4mat4(&orientation, &orientation, &translation);
	return orientation;
}
