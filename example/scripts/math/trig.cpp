#include <stdio.h>
#include <math.h>

#if defined(SSR_SCRIPTID)
#	define _ssr_func_paste(a, b) a ## $ ## b
#	define _ssr_func_eval(a, b) _ssr_func_paste(a, b)
#	define ssr_func(ret, name) extern "C" __declspec(dllexport) ret _ssr_func_eval(SSR_SCRIPTID, name)
#else
#	define ssr_func(ret, name) extern "C" __declspec(dllexport) ret name
#endif

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