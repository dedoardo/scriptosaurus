#pragma once

#if defined(scriptosaurus_live)
#	define script_routine(fun) extern "C" __declspec(dllexport) void fun(void* args)
#else
#	if !defined(scriptosaurus_script_name)
#		error Need a script name for merging
#	endif
#	if !defined(scriptosaurus_separator)
#		error Need a separator for merging
#	endif
#	define _script_concat_eval(x, y) x ## y
#	define _script_concat(x, y) _script_concat_eval(x, y)
#	define script_routine(fun) extern "C" __declspec(dllexport) void _script_concat(scriptosaurus_script_name, scriptosaurus_separator)##fun(void* args)
#endif