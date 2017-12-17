# Scriptosaurus - Single-Header C Live Scripting 

### Introduction
Scriptosaurus is a single-header C hot-reloading library. It's an end-to-end solution that runs a daemon on the specified user directory, recompiles the code on changes and hot-swaps the shared library replacing registered function pointers. 
Scripts are individual `*.c` source files that get compiled to a single shared library and can export multiple functions. If a script is located in `foo/bar.c` it is referenced to as `foo$bar` (path separators are replaced by `$`) from the application side. 
If hot-reloading is not enabled (by **not** defining `SSR_LIVE`) the library will run in **Release** mode, compiling all the scripts into a single shared library with zero-overhead at runtime.


## Example
```
<project-dir>/
    main.c
    scripts/
        foo/
            bar.c
```

`main.c`

```c
#define SSR_LIVE 	// Live editing
#define SSR_IMPLEMENTATION // Platform
#include "scriptosaurus.h" // Single header

typedef int(*fun_t)(int);

void main()
{
    fun_t my_fun;
    ssr_t ssr;
    ssr_init(&ssr, "scripts", NULL);
    
    // Always call run before any other ssr_add
	ssr_run(&ssr);
	
    // Links 
    ssr_add(&ssr, "foo$bar", "my_script_fun", &my_fun);
    
    // ...
    // If the content of scripts/foo/bar.c, it will be recompiled
    // and my_fun updated accordingly
}
```

`scripts/foo/bar.c`
```
#define SSR_SCRIPT
#include "scriptosaurus.h"

ssr_fun(int, my_script_fun)(int v)
{
    return i  * 2;
}
```

## API

**`ssr_t`** Library object, each instance works on a single directory. Multiple are supported.
```c
typedef struct _ssr_t { } ssr_t; 
``` 

**`ssr_init()`** Initializes the library and launches the daemon (if `SSR_LIVE` is defined)
```c
bool ssr_init(struct ssr_t* ssr, const char* root, struct ssr_config_t* config)
```
```
Arguments:
    - ssr: Library object
    - root: Relative of absolute path of the directory to be searched for scripts
    - config: Compiler options, see ssr_config_t for more information
Returns:
    True if initialization was successful, false otherwise 
```

**`ssr_add`** Registers a function for hot-reloading.
```c
bool ssr_add(struct ssr_t* srr, const char* script_id, const char* fun_name, ssr_func_t* user_routine)
```
```
Arguments:
    - ssr: Library object
    - script_id: Unique script identifier relative to root directory as specified in ssr_init(). Path separators are replaced by '$'.
    - fun_name: Name of the function exported by the script
    - user_routine: Pointer to function pointer which is to be registered for hot-reloading. Remember to cast it to the correct function type before loading

Returns:
    True if registering was successful and the function pointer will be updated, false otherwise.
```

**`ssr_remove`** Unregisters a function previously registered with `ssr_add`
```c
void ssr_remove(struct ssr_t* ssr, const char* script_id, const char* fun_name, ssr_func_t* user_routine)
```
```
Arguments:
    - ssr: Library object
    - script_id: Unique script identifier relative to root directory as specified in ssr_init(). Path separators are replaced by '$'.
    - fun_name: Name of the function user_routine refers to.
    - user_routine: Address of function pointer previously registered for listening.
```

Compiler and related options can be controlled by `ssr_config_t`. By default the client's configuration is used.

```c
typedef struct ssr_config_t
{
	int compiler; // SSR_COMPILER
	int msvc_ver; // SSR_MSVC_VER
	int target_arch; // SSR_ARCH
	int flags; // SSR_FLAGS
	char* compile_args_beg; // added before any other flag
	char* compile_args_end; // added before input files
	char* link_args_beg; // added before any other flag
	char* link_args_end; // added before input files
	char** include_directories; // compiler include directories
	char** link_libraries; // libraries to link to - absolute paths
	char** defines; // name=value strings
	size_t num_include_directories;
	size_t num_link_libraries;
	size_t num_defines;
}ssr_config_t;
```

**Note**: For more info please browse `scriptosaurus.h`

## Supported platforms

**Backends**: Win32, Posix
**Compilers**: gcc, clang, msvc