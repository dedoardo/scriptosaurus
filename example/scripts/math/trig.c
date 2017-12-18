#include <stdio.h>
#include <math.h>

#define SSR_SCRIPT
#include "../../../scriptosaurus.h"

ssr_func(float, my_sin)(float v)
{
	printf("my_sin(%f)=", v);
	return sinf(v);
}

ssr_func(float, my_cos)(float v)
{
	printf("my_cos(%f)=", v);
	return cosf(v);
}

ssr_func(float, my_tan)(float v)
{
	printf("my_tan(%f)=", v);
	return tanf(v);
}

ssr_func(float, my_tan2)(float v)
{
	return v;
}