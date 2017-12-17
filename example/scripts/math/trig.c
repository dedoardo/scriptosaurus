#include <stdio.h>
#include <math.h>

#if __cplusplus__
#define SSR_EXTERN extern "C"
#else
#define SSR_EXTERN
#endif

// Switch is not on the compiler but on the platform as gcc and clang when on windows try to be compatible with MSVC
#define SSR_LINUX
#if defined(SSR_WIN)
#	define SSR_EXPORT __declspec(dllexport)
#elif defined(SSR_LINUX)
#	define SSR_EXPORT __attribute__ ((visibility ("default")))
#endif

#if defined(SSR_SCRIPTID)
#	define _ssr_func_paste(a, b) a ## $ ## b
#	define _ssr_func_eval(a, b) _ssr_func_paste(a, b)
#	define ssr_func(ret, name) SSR_EXTERN __declspec(dllexport) ret _ssr_func_eval(SSR_SCRIPTID, name)
#else
#	define ssr_func(ret, name) SSR_EXTERN SSR_EXPORT ret name
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