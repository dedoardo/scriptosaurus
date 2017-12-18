#ifndef _SSR_H_GUARD_
#define _SSR_H_GUARD_

// libc includes
#include <stdlib.h>  
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>

#ifdef _WIN32
#	define SSR_WIN
#	ifdef _WIN64
#		define SSR_64
#	else
#		define SSR_32
#	endif
#elif defined(__linux__)
#	define SSR_LINUX
#	ifdef __x86_64__
#		define SSR_64
#	else
#		define SSR_32
#	endif
#else
#	error
#endif

/*-----------------------------------------------------------------------------
Scriptosaurus API - abbreviation is ssr

Usage:
	For usage see example/
	Remember to call run() before any add() otherwise scripts won't be loaded
	when not in live.

Macros:
	SSR_LIVE - If defined scripts will dynamically recompiled and updated by the
		daemon. Otherwise all the files will be compiled on the run().
 */ 

#ifndef SSR_MAX_SCRIPTS
#define SSR_MAX_SCRIPTS 128 // TODO: not really needed atm
#endif

#ifndef SSR_BIN_DIR
#define SSR_BIN_DIR ".bin"
#endif

#ifndef SSR_SL_LEN
#define SSR_SL_LEN 10
#endif

#ifndef SSR_SINGLE_SL_NAME
#define SSR_SINGLE_SL_NAME "ssr"
#endif

#ifndef SSR_FILE_EXTS
#define SSR_FILE_EXTS ".c", "" // end w/ empty string
#endif

#ifndef SSR_LOG_BUF
#define SSR_LOG_BUF 1024 * 4
#endif

#ifndef SSR_COMPILER_BUF
#define SSR_COMPILER_BUF 1024 * 4
#endif

#ifndef SSR_SLEEP_MS
#define SSR_SLEEP_MS 16
#endif

#ifdef SSR_STATIC
#define SSR_DEF static
#else
#define SSR_DEF extern
#endif

enum SSR_COMPILER
{
	SSR_COMPILER_MSVC, // no need for vcvars/cl/link to be in PATH
	SSR_COMPILER_CLANG, // assumes clang is a valid command
	SSR_COMPILER_GCC // assumes gcc is a valid command
};

enum SSR_MSVC_VER
{
	SSR_MSVC_VER_14_1, // Visual Studio 2017
	SSR_MSVC_VER_14_0, // Visual Studio 2015
	// .. Older MSVC versions should work out of the box
};

enum SSR_ARCH
{
	SSR_ARCH_X86,
	SSR_ARCH_X64,
	SSR_ARCH_ARM
};

enum SSR_FLAGS
{
	SSR_FLAGS_GEN_DEBUG = 1,

	SSR_FLAGS_GEN_OPT1 = 1 << 1,
	SSR_FLAGS_GEN_OPT2 = 1 << 2,
};

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SSR_CB_TYPE
{
	SSR_CB_INFO = 1,
	SSR_CB_WARN = 1 << 1,
	SSR_CB_ERR = 1 << 2,
	SSR_CB_ALL = SSR_CB_INFO | SSR_CB_WARN | SSR_CB_ERR
}SSR_CB_TYPE;

struct ssr_t;

typedef struct ssr_config_t
{
	// If not specified they are filled in matching the client compiler
	int compiler; // SSR_COMPILER
	int msvc_ver; // SSR_MSVC_VER
	const char* msvc141_path;
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


typedef void* ssr_func_t;
typedef void(*ssr_cb_t)(int, const char*); // flags is SSR_CB_*** | [SSR_CB_ERROR]
typedef void* ssr_routine_t;
#define SSR_SEP '$' // Not the best looking separator indeed, but it makes life slightly easier as it's also valid as a function character (TODO)

SSR_DEF bool ssr_init(struct ssr_t*, const char* root, struct ssr_config_t* config); // if config==NULL a default one is generated based on the host arch/compiler
SSR_DEF void ssr_destroy(struct ssr_t*);
SSR_DEF bool ssr_run(struct ssr_t*);
SSR_DEF bool ssr_add(struct ssr_t*, const char*, const char*, ssr_func_t*);
SSR_DEF void ssr_remove(struct ssr_t*, const char*, const char*, ssr_func_t*);
SSR_DEF void ssr_cb(struct ssr_t*, int mask, ssr_cb_t);

#endif

#ifdef __cplusplus
}
#endif

/*-----------------------------------------------------------------------------
Internal API
	_ssr_str
	_ssr_vec
	_ssr_map
*/
#ifdef SSR_IMPLEMENTATION

static void _ssr_log(int flags, const char* fmt, ...);

typedef struct __ssr_str_t
{
	char* b;
}_ssr_str_t;

static char* _ssr_alloc(size_t len);
static void  _ssr_free(char* ptr);

static _ssr_str_t _ssr_str_e(void);
static _ssr_str_t _ssr_str(const char* src);
static _ssr_str_t _ssr_str_f(const char* fmt, ...);
static _ssr_str_t _ssr_str_rnd(size_t len);
static void	      _ssr_str_destroy(_ssr_str_t);
static char* 	  _ssr_strncpy(char* a, const char* b, size_t n);

typedef struct __ssr_vec_t
{
	uint8_t* beg;
	uint8_t* cur;
	uint8_t* end;
	size_t size;
}_ssr_vec_t;

static void		  _ssr_vec(_ssr_vec_t* vec, size_t stride, size_t len);
static void		  _ssr_vec_destroy(_ssr_vec_t* vec);
static size_t	  _ssr_vec_capacity(const _ssr_vec_t* vec);
static size_t	  _ssr_vec_len(_ssr_vec_t* vec);
static void*	  _ssr_vec_at(_ssr_vec_t* vec, size_t idx);
static void	      _ssr_vec_push(_ssr_vec_t* vec, const void* el);
static void		  _ssr_vec_remove(_ssr_vec_t* vec, const void* el);

#define _SSR_MAP_CAPACITY SSR_MAX_SCRIPTS * 4
#define _SSR_MAP_HASH_MASK (_ssr_hash_t)1 << 63
typedef struct __ssr_map_t
{
	uint8_t* buf;
	size_t size;
	size_t mask;
	size_t key_off;
}_ssr_map_t;
typedef uint64_t _ssr_hash_t;
typedef void(*_ssr_map_iter_cb_t)(void* el, void* args);

static void  _ssr_map(_ssr_map_t* map, size_t entry_size, size_t key_off);
static void  _ssr_map_destroy(_ssr_map_t* map);
static void* _ssr_map_find(_ssr_map_t* map, _ssr_hash_t hash);
static void* _ssr_map_find_str(_ssr_map_t* map, const char* key);
static void  _ssr_map_add(_ssr_map_t* map, const void* _obj, _ssr_hash_t hash);
static void  _ssr_map_add_str(_ssr_map_t* map, const void* _obj);
static void  _ssr_map_iter(_ssr_map_t* map, _ssr_map_iter_cb_t cb, void* args);


/*-----------------------------------------------------------------------------
Internal System API
	_ssr_lock
	_srr_thread
	_ssr_lib
	string & file helpers
*/
struct _ssr_lock_t;
struct _ssr_thread_t;
struct _ssr_lib_t;
typedef uint64_t _ssr_timestamp_t;
typedef void(*_ssr_iter_dir_cb_t)(void* args, const char*, const char*);

static void _ssr_lock(struct _ssr_lock_t* lock);
static void _ssr_lock_destroy(struct _ssr_lock_t* lock);
static void _ssr_lock_acq(struct _ssr_lock_t* lock);
static void _ssr_lock_rel(struct _ssr_lock_t* lock);

static bool _ssr_thread(struct _ssr_thread_t* thread, void* fun, void* args);
static void _ssr_thread_join(struct _ssr_thread_t* thread);

static const char*	_ssr_lib_ext(void);
static bool		    _ssr_lib(struct _ssr_lib_t* lib, const char* path);
static void			_ssr_lib_destroy(struct _ssr_lib_t* lib);
static void*		_ssr_lib_func_addr(struct _ssr_lib_t* lib, const char* fname);

static _ssr_timestamp_t	_ssr_file_timestamp(const char* path);
static bool				_ssr_file_exists(const char* path);
static _ssr_str_t		_ssr_fullpath(const char* rel);
static _ssr_str_t		_ssr_remove_ext(const char* str, long long len);
static const char*		_ssr_extract_rel(const char* base, const char* path);
static const char*		_ssr_extract_ext(const char* path);
static _ssr_str_t		_ssr_replace_seps(const char* str, char new_sep);
static void				_ssr_iter_dir(const char* root, _ssr_iter_dir_cb_t cv, void* args);
static void				_ssr_new_dir(const char* dir);
static void				_ssr_sleep(unsigned int ms);
static int				_ssr_run(char* cmd, _ssr_str_t* out, _ssr_str_t* err);

/*-----------------------------------------------------------------------------
	Implementation
*/
static char* _ssr_alloc(size_t len)
{
	return (char*)malloc(sizeof(char) * len);
}

static void _ssr_free(char* str)
{
	free(str);
}

static _ssr_str_t _ssr_str_e(void)
{
	_ssr_str_t ret;
	ret.b = NULL;
	return ret;
}

static _ssr_str_t _ssr_str(const char* src)
{
	size_t len = strlen(src);
	_ssr_str_t ret;
	ret.b = _ssr_alloc(len + 1);
	_ssr_strncpy(ret.b, src, len + 1);
	return ret;
}

static _ssr_str_t _ssr_str_f(const char* fmt, ...)
{
    // The code runs without va_copy on gcc and msvc
    va_list args, args_cp;
    va_start(args, fmt);
    va_copy(args_cp, args);
    size_t len = vsnprintf(NULL, 0, fmt, args);
    _ssr_str_t ret;
    ret.b = _ssr_alloc(len + 1);
    vsnprintf(ret.b, len + 1, fmt, args_cp);
    va_end(args);
    return ret;
}

static _ssr_str_t _ssr_str_rnd(size_t len)
{
	if (len == 0) 
		return _ssr_str_e();
	_ssr_str_t ret;
	ret.b = _ssr_alloc(len + 1);
	size_t i;
	for (i = 0; i < len; ++i)
		ret.b[i] = (char)(97 + (rand() % 25));
	ret.b[i] = '\0';
	return ret;
}

static void _ssr_str_destroy(_ssr_str_t str)
{
	_ssr_free(str.b);
}

static char* _ssr_strncpy(char* a, const char* b, size_t n)
{
	char* p = a;
	while (n > 0 && *b != '\0')
	{
		*p++ = *b++;
		--n;		
	}

	while (n > 0)
	{
		*p++ = '\0';
		--n;
	}
	return a;
}

static void _ssr_vec(_ssr_vec_t* vec, size_t size, size_t len)
{
	if (vec == NULL) return;
	vec->beg = (uint8_t*)malloc(size * len);
	vec->cur = vec->beg;
	vec->end = vec->beg + size * len;
	vec->size = size;
}

static void _ssr_vec_destroy(_ssr_vec_t* vec)
{
	if (vec != NULL)
	{
		free(vec->beg);
		vec->beg = NULL;
	}
}

static size_t _ssr_vec_len(_ssr_vec_t* vec)
{
	if (vec == NULL || vec->beg == NULL) return 0;
	return (vec->cur - vec->beg) / vec->size;
}

static size_t _ssr_vec_capacity(const _ssr_vec_t* vec)
{
	return (vec->end - vec->beg) / vec->size;
}

static void* _ssr_vec_at(_ssr_vec_t* vec, size_t idx)
{
	return vec->beg + idx * vec->size;
}

static void _ssr_vec_push(_ssr_vec_t* vec, const void* el)
{
	size_t capacity = _ssr_vec_capacity(vec
	);
	if (_ssr_vec_len(vec) >= capacity)
	{
		size_t new_capacity = capacity * 2 > 32 ? capacity * 2 : 32;
		vec->beg = (uint8_t*)realloc(vec->beg, vec->size * new_capacity);
		vec->cur = vec->beg;
		vec->end = vec->beg + new_capacity * vec->size;
	}

	memcpy(vec->cur, el, vec->size);
	vec->cur += vec->size;
}

// swaps with last
static void _ssr_vec_remove(_ssr_vec_t* vec, const void* obj)
{
	uint8_t* cur = vec->beg;
	while (cur < vec->cur)
	{
		if (memcmp(cur, obj, vec->size) == 0)
		{
			memcpy(cur, (vec->cur - vec->size), vec->size);
			vec->cur-= vec->size;
			return;
		}

		cur += vec->size;
	}
}

static void  _ssr_map(_ssr_map_t* map, size_t entry_size, size_t key_off)
{
	map->buf = (uint8_t*)malloc(entry_size * _SSR_MAP_CAPACITY);
	memset(map->buf, 0, entry_size * _SSR_MAP_CAPACITY);
	map->size = entry_size;
	map->mask = _SSR_MAP_CAPACITY - 1;
	map->key_off = key_off;
}

static void  _ssr_map_destroy(_ssr_map_t* map)
{
	if (map != NULL)
	{
		free(map->buf);
		map->buf = 0;
	}
}

static void* _ssr_map_find(_ssr_map_t* map, _ssr_hash_t hash)
{
	size_t base = (size_t)hash & map->mask;
	for (size_t i = 0; i <= map->mask; ++i)
	{
		uint8_t* cur = map->buf + (((base + i) & map->mask) * map->size);
		uint8_t* cur_key = cur + map->key_off;
		_ssr_hash_t* cur_hash = (_ssr_hash_t*)cur_key;

		if ((uint32_t)*cur_hash == (uint32_t)hash)
			return cur;

		if ((*cur_hash & _SSR_MAP_HASH_MASK) == 0)
			return NULL;
	}

	return NULL;
}

static uint32_t _fnv_32_str(const char *str, uint32_t hval)
{
	unsigned char *s = (unsigned char *)str;
	while (*s)
	{
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
		hval ^= (uint32_t)*s++;
	}
	return hval;
}

static void* _ssr_map_find_str(_ssr_map_t* map, const char* key)
{
	uint32_t hash = _fnv_32_str(key, 0);
	return _ssr_map_find(map, hash);

}

static void  _ssr_map_add(_ssr_map_t* map, const void* _obj, _ssr_hash_t hash)
{
	size_t base = (size_t)hash & map->mask;
	for (size_t i = 0; i <= map->mask; ++i)
	{
		uint8_t* cur = map->buf + (((base + i) & map->mask) * map->size);
		uint8_t* cur_key = cur + map->key_off;
		_ssr_hash_t* cur_hash = (_ssr_hash_t*)cur_key;

		if ((*cur_hash & _SSR_MAP_HASH_MASK) == 0)
		{
			memcpy(cur, _obj, map->size);
			*cur_hash = (uint64_t)hash | (uint64_t)1 << 63;
			return;
		}
	}

}

static void  _ssr_map_add_str(_ssr_map_t* map, const void* _obj)
{
	_ssr_str_t* str = (_ssr_str_t*)((const char*)_obj + map->key_off + sizeof(_ssr_hash_t));
	uint32_t hash = _fnv_32_str(str->b, 0);
	_ssr_map_add(map, _obj, hash);
}

static void  _ssr_map_iter(_ssr_map_t* map, _ssr_map_iter_cb_t cb, void* args)
{
	for (size_t i = 0; i <= map->mask; ++i)
	{
		uint8_t* cur = map->buf + (i & map->mask) * map->size;
		uint8_t* cur_key = cur + map->key_off;
		_ssr_hash_t* cur_hash = (_ssr_hash_t*)cur_key;

		if ((*cur_hash & _SSR_MAP_HASH_MASK) != 0x0)
			cb(cur, args);
	}
}

// Windows
#ifdef SSR_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>
#pragma comment(lib, "Shlwapi.lib") // TODO

typedef struct _ssr_lock_t
{
	// Initially I had just CRITICAL_SECTION, but it gets memcpyd around and the application
	// verifier triggered an "Uninitialized critical section error"
	CRITICAL_SECTION* h;
}_ssr_lock_t;

static void _ssr_lock(struct _ssr_lock_t* lock)
{
	lock->h = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(lock->h);
}

static void _ssr_lock_destroy(struct _ssr_lock_t* lock)
{
	if (lock != NULL)
	{
		DeleteCriticalSection(lock->h);
		free(lock->h);
	}
}

static void _ssr_lock_acq(struct _ssr_lock_t* lock)
{
	EnterCriticalSection(lock->h);
}

static void _ssr_lock_rel(struct _ssr_lock_t* lock)
{
	LeaveCriticalSection(lock->h);
}

typedef struct _ssr_thread_t
{
	HANDLE handle;
	unsigned int id;
}_ssr_thread_t;

static bool _ssr_thread(struct _ssr_thread_t* thread, void* fun, void* args)
{
	thread->handle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)fun, args, 0, &thread->id);
	return thread->handle != NULL;
}

static void _ssr_thread_join(struct _ssr_thread_t* thread)
{
	if (thread == NULL || thread->handle == NULL)
		return;

	WaitForSingleObject(thread->handle, INFINITE);
	CloseHandle(thread->handle);
}

typedef struct _ssr_lib_t
{
	HMODULE h;
}_ssr_lib_t;

static const char*	_ssr_lib_ext(void)
{
	return "dll";
}

static bool _ssr_lib(struct _ssr_lib_t* lib, const char* path)
{
	lib->h = LoadLibraryA(path);
	if (lib->h == NULL)
	{
		_ssr_log(SSR_CB_ERR, "Error loading shared library: %s", path);
		return false;
	}

	return true;
}

static void _ssr_lib_destroy(struct _ssr_lib_t* lib)
{
	FreeLibrary(lib->h);
}

static void* _ssr_lib_func_addr(struct _ssr_lib_t* lib, const char* fname)
{
	if (lib == NULL) return NULL;
	return (void*)GetProcAddress(lib->h, fname);
}

static _ssr_timestamp_t _ssr_file_timestamp(const char* path)
{
	if (path == NULL) return (uint64_t)-1;
	HANDLE hfile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hfile == NULL)
		return (uint64_t)-1;

	FILETIME wt;
	GetFileTime(hfile, NULL, NULL, &wt);

	CloseHandle(hfile);

	ULARGE_INTEGER wt_i;
	wt_i.u.LowPart = wt.dwLowDateTime;
	wt_i.u.HighPart = wt.dwHighDateTime;
	return (_ssr_timestamp_t)wt_i.QuadPart;
}

static bool _ssr_file_exists(const char* path)
{
	return PathFileExistsA(path);
}

static _ssr_str_t _ssr_fullpath(const char* rel)
{
	_ssr_str_t ret;
	ret.b = _fullpath(NULL, rel, 0);
	return ret;
}


static void _ssr_iter_dir(const char* root, _ssr_iter_dir_cb_t cb, void* args)
{
	char path[2048];
	sprintf(path, "%s\\*.*", root);

	WIN32_FIND_DATAA fd;
	HANDLE hfind = FindFirstFileA(path, &fd);
	if (hfind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0 || strcmp(fd.cFileName, ".bin") == 0)
				continue;

			sprintf(path, "%s\\%s", root, fd.cFileName);

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				_ssr_iter_dir(path, cb, args);
				continue;
			}

			FILE* fp = fopen(path, "r");
			if (fp != NULL)
			{
				cb(args, root, fd.cFileName);
				fclose(fp);
			}
		} while (FindNextFileA(hfind, &fd));
		FindClose(hfind);
	}
}

static void _ssr_new_dir(const char* dir)
{
	if (dir == NULL || dir[0] == '\0')
	{
		_ssr_log(SSR_CB_ERR, "Failed to create new directory");
		return;
	}

	_ssr_str_t rm = _ssr_str_f("cmd.exe /C \"rd /s /q \"%s\"\"", dir);
	_ssr_run(rm.b, NULL, NULL);
	CreateDirectoryA(dir, NULL);
	_ssr_str_destroy(rm);
}

static void _ssr_sleep(unsigned int ms)
{
	Sleep(ms);
}

static int _ssr_run(char* cmd, _ssr_str_t* out, _ssr_str_t* err)
{
	HANDLE stdout_r, stdout_w;
	HANDLE stderr_r, stderr_w;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = true;
	sa.lpSecurityDescriptor = NULL;

	bool ret = CreatePipe(&stdout_r, &stdout_w, &sa, 0);
	ret = SetHandleInformation(stdout_r, HANDLE_FLAG_INHERIT, 0);
	ret = CreatePipe(&stderr_r, &stderr_w, &sa, 0);
	ret = SetHandleInformation(stderr_r, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si;
	ZeroMemory(&si, sizeof(STARTUPINFOA));
	si.cb = sizeof(STARTUPINFOA);
	si.hStdOutput = stdout_w;
	si.hStdError = stderr_w;
	si.dwFlags = STARTF_USESTDHANDLES;;

	PROCESS_INFORMATION pi;
	if (!CreateProcessA(NULL, cmd, NULL, NULL, true, 0, NULL, NULL, &si, &pi))
		return -1;

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD out_size;
	PeekNamedPipe(stdout_r, NULL, 0, NULL, &out_size, NULL);
	DWORD err_size;
	PeekNamedPipe(stderr_r, NULL, 0, NULL, &err_size, NULL);

	if (out != NULL) {
		if (out_size == 0) {
			out->b = _ssr_alloc(out_size);
			out->b[out_size] = '\0';
		} else {
			out->b = _ssr_alloc(out_size);
			if (out_size) ReadFile(stdout_r, out->b, out_size, &out_size, NULL);
			out->b[out_size - 2] = '\0';	// overwrite trailing \r\n
		}
	}

	if (err != NULL) {
		if (err_size == 0) {
			err->b = _ssr_alloc(err_size);
			err->b[err_size] = '\0';
		} else {
			err->b = _ssr_alloc(err_size);
			if (err_size) ReadFile(stderr_r, err->b, err_size, &err_size, NULL);
			err->b[err_size - 2] = '\0';	// overwrite trailing \r\n
		}
	}

	CloseHandle(stdout_w);
	CloseHandle(stderr_w);
	CloseHandle(stdout_r);
	CloseHandle(stderr_r);

	DWORD ret_val;
	GetExitCodeProcess(pi.hProcess, &ret_val);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return (int) ret_val;
}

#elif defined(SSR_LINUX)
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

typedef struct _ssr_lock_t
{
	pthread_mutex_t h;
}_ssr_lock_t;

static void _ssr_lock(struct _ssr_lock_t* lock)
{
	if (lock == NULL) return;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock->h, &attr);
}

static void _ssr_lock_destroy(struct _ssr_lock_t* lock)
{
	if (lock == NULL) return;
	pthread_mutex_destroy(&lock->h);
}

static void _ssr_lock_acq(struct _ssr_lock_t* lock)
{
	if (lock == NULL) return;
	pthread_mutex_lock(&lock->h);
}

static void _ssr_lock_rel(struct _ssr_lock_t* lock)
{
	if (lock == NULL) return;
	pthread_mutex_unlock(&lock->h);
}

typedef struct _ssr_thread_t
{
	pthread_t handle;
}_ssr_thread_t;

static bool _ssr_thread(struct _ssr_thread_t* thread, void* fun, void* args)
{
	typedef void* fun_t(void*);
	if (thread == NULL || fun == NULL) return false;
	if(pthread_create(&thread->handle, NULL, (fun_t*)fun, (void*)args))
		return true;
	return false;
}

static void _ssr_thread_join(struct _ssr_thread_t* thread)
{
	if (thread == NULL) return;
	pthread_join(thread->handle, NULL);
}

typedef struct _ssr_lib_t
{
	void* h;
}_ssr_lib_t;

static const char*	_ssr_lib_ext(void)
{
	return "so";
}

static bool _ssr_lib(struct _ssr_lib_t* lib, const char* path)
{
	if (lib == NULL || path == NULL) return false;
	lib->h = dlopen(path, RTLD_NOW);
	if (lib->h == NULL)
	{
		const char* err = dlerror();
		_ssr_log(SSR_CB_WARN, "%s", err);
		return false;
	}
	return true;
}

static void _ssr_lib_destroy(struct _ssr_lib_t* lib)
{
	if (lib == NULL || lib->h == NULL) return;
	dlclose(lib->h);
}

static void* _ssr_lib_func_addr(struct _ssr_lib_t* lib, const char* fname)
{
	if (lib == NULL) return NULL;
	return dlsym(lib->h, fname);
}

static _ssr_timestamp_t _ssr_file_timestamp(const char* path)
{
	struct stat s;
	if (stat(path, &s) == -1)
		return 0;

	return s.st_mtime;
}

static bool _ssr_file_exists(const char* path)
{
	if (path == NULL) return false;
	return access(path, R_OK) != -1;
}

static _ssr_str_t _ssr_fullpath(const char* rel)
{
	char fullpath[PATH_MAX+1];
	if (realpath(rel, fullpath) == NULL) return _ssr_str_e();
	return _ssr_str(fullpath);
}

static void _ssr_iter_dir(const char* root, _ssr_iter_dir_cb_t cb, void* args)
{
	DIR* dir;
	dir = opendir(root);

	struct dirent* de;
	while ((de = readdir(dir)) != NULL)
	{
		if (de->d_type == DT_DIR)
		{
			if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
				continue;
			char path[PATH_MAX+1];
			snprintf(path, PATH_MAX+1, "%s/%s", root, de->d_name);
			_ssr_iter_dir(path, cb, args);
		}
		else
			cb(args, root, de->d_name);
	}
	closedir(dir);
}

static void _ssr_new_dir(const char* dir)
{
	if (dir == NULL) return;
	if (mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) return;
}

static void _ssr_sleep(unsigned int ms)
{
	usleep(ms * 1000);
}

static int	_ssr_run(char* cmd, _ssr_str_t* out, _ssr_str_t* err)
{
    char buf[SSR_COMPILER_BUF+3];
	memset(buf, 0, SSR_COMPILER_BUF+3);

    FILE* pipe_out  = popen(cmd,  "r");
	if (pipe_out == NULL)
	{
		// TODO: Error
		return 1;
	}

    while ( fgets(buf, SSR_COMPILER_BUF, pipe_out) != NULL );
    int ret_code = pclose(pipe_out);

	if (WIFEXITED(ret_code))
		ret_code = 0;
	else
		ret_code = WEXITSTATUS(ret_code);

	// Gcc output sometimes contains '\0\0' on newlines, as the buffer is guaranteed
	// to have '\0\0\0' at the end this is safe. Just replacing '\0\0' to make code printable
	char* cur = buf;
	while (*cur != '\0' || *(cur+1) != '\0' || (*(cur+2) != '\0'))
	{
		if (*cur == '\0') *cur = ' ';
		++cur;
	}

    if (buf[0] != '\0')
	    *err = _ssr_str(buf);
	return ret_code;
}

#else
#	error
#endif

static _ssr_str_t _ssr_remove_ext(const char* str, long long len)
{
	if (len == -1)
		len = strlen(str);
	const char* cur = str + len - 1;
	while (cur > str && *cur != '.')
		--cur;
	--cur;
	_ssr_str_t ret;
	size_t buf_len = cur - str + 1;
	ret.b = (char*)malloc(buf_len + 1);
	memcpy(ret.b, str, buf_len);
	ret.b[buf_len] = '\0';
	return ret;
}

// Returns a pointer in the same buffer that represents the relative path based on base.
// base and path need to be full paths w/ driver letters in the same notation. 
// path separators are not required to be the same
static const char* _ssr_extract_rel(const char* base, const char* path)
{
	// let's be safe on the first comparisons
	if (base == NULL || path == NULL || *base == '\0' || *path == '\0' || *(base+1) == '\0' || *(base+2) == '\0')
		return NULL;

	const char* b = base;
	const char* p = path;

	if (*(b + 1) == ':')
	{
		if (*(p + 1) == ':')
		{
			if (*b != *p)
				return NULL;
		}
		else
			return NULL;

		b += 2;
		p += 2;
	}

	while (true)
	{
		const char* sb = b;
		const char* sp = p;

		// Stopping at first path separator
		while (*b != '\0' && *b != '/' && *b != '\\') ++b;
		while (*p != '\0' && *p != '/' && *p != '\\') ++p;

		// Comparing directory
		if (b - sb != p - sp) return p;
		if (memcmp(sb, sp, p - sp) != 0) return p;

		// Skipping separators
		while (*b != '\0' && *b == '/' || *b == '\\') ++b;
		while (*p != '\0' && *p == '/' || *p == '\\') ++p;

			if (*b == '\0') return p;
		if (*p == '\0') return NULL; // the other way around
	}
	return NULL;
}

static const char* _ssr_extract_ext(const char* path)
{
	const char* cur = path + strlen(path);
	while (*(--cur) != '.' && cur >= path);
	return cur;
}

static _ssr_str_t _ssr_replace_seps(const char* str, char new_sep)
{
	size_t rel_path_len = strlen(str); // TODO: Can we avoid?
    char* buffer = _ssr_alloc(rel_path_len + 1);
	const char* read = str;
	char* write = buffer;
	while (*read != '\0')
	{
		if (*read == '\\' || *read == '/')
		{
			if (write > buffer && *(buffer - 1) != new_sep) // Avoiding double separator
			{
				*(write++) = new_sep;
				++read;
				continue;
			}
		}
		*(write++) = *(read++);
	}
	*write = '\0';
	_ssr_str_t no_ext_ret = _ssr_remove_ext(buffer, -1);
	_ssr_free(buffer);
	return no_ext_ret;
}

// Needed for live/not live
enum _SSR_COMPILE_STAGES
{
	_SSR_COMPILE = 1,
	_SSR_LINK = 1 << 1,
	_SSR_COMPILE_N_LINK = _SSR_COMPILE | _SSR_LINK
};

static void _ssr_merge_in(char* buf, size_t buf_len, const char* flag, char** args, size_t num_args)
{
	buf[0] = '\0';
	// TODO: Could be done in a bette way ?
	size_t len = 0;
	for (size_t i = 0; i < num_args; ++i)
	{
		size_t new_len = snprintf(NULL, 0, "%s%s%s", buf, flag, args[i]);
		if (new_len > buf_len) break; // TODO: warn user that skipping define
		len += snprintf(buf, buf_len, "%s%s%s", buf, flag, args[i]);
	}
}

// In the todolist there is eventually a string pool
#define _SSR_ARGS_BUF_LEN 1 << 13
//runs `vcvars && cl` and `vcvars && link`
//@todo: in the future you can call vcvars, save env and feed env to `cl` and `link`, maybe there is even better way?
//default compiler arguments:
//absolute paths assumed
//satges is SSR_COMPILE_STAGES
//if stages == SSR_LINK then inputs should be the object files
//input is single string with filesnames separated by spaces
static bool _ssr_compile_msvc(const char* input, ssr_config_t* config, const char* _out, int stages)
{
	bool ret = false;

	// Generates vcvars string as is both used in cl & link
	char* vcvars = NULL;
	{
		// Atleast from 15->17 installations paths are different

		const char* bases[] = {
			config->msvc141_path,
			"C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC"
		};

		static const char* archs[] = {
			"x86",
			"x86_amd64",
			"x86_arm"
		};

		const char* base = bases[config->msvc_ver];
		const char* arch = archs[config->target_arch];

		char* fmt = "\"%svcvarsall.bat\" %s";
		size_t vcvars_len = snprintf(NULL, 0, fmt, base, arch);
		vcvars = (char*)malloc(vcvars_len + 1);
		snprintf(vcvars, vcvars_len + 1, fmt, base, arch);
	}

	// compile step
	_ssr_str_t compile_out = _ssr_str_e();
	
	if (stages & _SSR_COMPILE)
	{
		// defines
		char defines[_SSR_ARGS_BUF_LEN];
		_ssr_merge_in(defines, _SSR_ARGS_BUF_LEN, "/D", config->defines, config->num_defines);

		// include directories
		char include_directories[_SSR_ARGS_BUF_LEN];
		_ssr_merge_in(include_directories, _SSR_ARGS_BUF_LEN, "/I", config->include_directories, config->num_include_directories);

		// default flags
		const char* default_args = "/c /EHsc /nologo";

		// flags
		const char* gen_dbg = config->flags & SSR_FLAGS_GEN_DEBUG ? "/Zi" : "";
		const char* mt_lib = config->flags & SSR_FLAGS_GEN_DEBUG ? "/Mtd" : "Mt";

		// custom args
		const char* beg_args = config->compile_args_beg == NULL ? "" : config->compile_args_beg;
		const char* end_args = config->compile_args_end == NULL ? "" : config->compile_args_end;

		// output just appending .obj at end of full path
		if (stages & _SSR_LINK)
			compile_out = _ssr_str_f("%s.obj", _out);

		// vcvars - base - beg_args - default_flags - gen_dbg - mt_lib - compile_out - end_args - input files
		char* fmt = "%s > nul && cl.exe %s %s %s %s /Fo\"%s\" %s \"%s\"";

		_ssr_str_t compile = _ssr_str_f(fmt, vcvars, beg_args, default_args, gen_dbg, mt_lib, stages & _SSR_LINK ? compile_out.b : _out, end_args, input);
		_ssr_log(SSR_CB_INFO, "Compiling %s ...", input);

		_ssr_str_t err;
		int ret = _ssr_run(compile.b, NULL, &err) == 0;
		_ssr_str_destroy(compile);
		if (strlen(err.b) > 0) 
		{
			if (ret)
				_ssr_log(SSR_CB_WARN, err.b);
			else
				_ssr_log(SSR_CB_ERR, err.b);
		}
		_ssr_str_destroy(err);
        
		if (!ret) goto end;
	}
	
	if (stages & _SSR_LINK)
	{
		// link libraries
		char link_directories[_SSR_ARGS_BUF_LEN];
		_ssr_merge_in(link_directories, _SSR_ARGS_BUF_LEN, "", config->link_libraries, config->num_link_libraries);

		// default flags
		const char* default_flags = "/DLL /NOLOGO";

		// flags
		const char* gen_dbg = config->flags & SSR_FLAGS_GEN_DEBUG ? "/DEBUG" : "";

		// custom args
		const char* beg_args = config->link_args_beg == NULL ? "" : config->link_args_beg;
		const char* end_args = config->link_args_end == NULL ? "" : config->link_args_end;

		// target
		const char* target =
#if defined SSR_64
			"/MACHINE:AMD64"
#else
			"/MACHINE:X86"
#endif
		;

		// vcvars - base - beg_args - default_flags - gen_dbg - target - _out - end_args - input
		char* fmt = "%s > nul && link.exe %s %s %s %s /OUT:\"%s\" %s \"%s\" %s";

		_ssr_str_t link = _ssr_str_f(fmt, vcvars, beg_args, default_flags, gen_dbg, target, _out, end_args, stages & _SSR_COMPILE ? compile_out.b : input, link_directories);
		_ssr_log(SSR_CB_INFO, "Linking %s ...", input);

		_ssr_str_t err;
		int ret = _ssr_run(link.b, NULL, &err) == 0;
		_ssr_str_destroy(link);
		if (err.b != NULL)
		{
			if (ret)
				_ssr_log(SSR_CB_WARN, err.b);
			else
				_ssr_log(SSR_CB_ERR, err.b);
		}

		if (!ret) goto end;
	}

	ret = true;
end:
	free(vcvars);
	_ssr_str_destroy(compile_out);
	return ret;
}

#if !defined SSR_CLANG_EXEC
#	define SSR_CLANG_EXEC "clang"
#endif
static bool _ssr_compile_clang(const char* input, ssr_config_t* config, const char* _out, int stages)
{
    bool ret = false;
    _ssr_str_t compile_out = _ssr_str_e();
    if (stages & _SSR_COMPILE)
    {
        compile_out = _ssr_str_f("%s.obj", _out);

        // defines
        char defines[_SSR_ARGS_BUF_LEN];
        _ssr_merge_in(defines, _SSR_ARGS_BUF_LEN, "-D", config->defines, config->num_defines);

        // include directories
        char include_directories[_SSR_ARGS_BUF_LEN];
        _ssr_merge_in(include_directories, _SSR_ARGS_BUF_LEN, "-I", config->include_directories, config->num_include_directories);

        const char* gen_dbg = config->flags & SSR_FLAGS_GEN_DEBUG ? "-g " : "";
        const char* opt_lvl = config->flags & SSR_FLAGS_GEN_OPT2 ? "-O2" : (config->flags & SSR_FLAGS_GEN_OPT1 ? "-O1" : ""); // Are they actually the same as gcc?

#if defined(SSR_LINUX)
        char* fmt = "%s -c -fPIC -o %s %s %s %s -o%s %s -lm 2>&1";
#else
        char* fmt = "%s -c -fPIC -o %s %s %s %s -o%s %s";
#endif

        _ssr_str_t compile = _ssr_str_f(fmt, SSR_CLANG_EXEC, gen_dbg, opt_lvl, defines, include_directories, stages & _SSR_LINK ? compile_out.b : _out, input);
		_ssr_log(SSR_CB_INFO, "Compiling %s ...", input);
		
		_ssr_str_t err;
		int ret = _ssr_run(compile.b, NULL, &err) == 0;
		_ssr_str_destroy(compile);
		if (err.b != NULL)
		{
			if (ret)
				_ssr_log(SSR_CB_WARN, err.b);
			else
				_ssr_log(SSR_CB_ERR, err.b);
		}
		_ssr_str_destroy(err);

        if (!ret) goto end;
    }

    if (stages & _SSR_LINK)
    {
        // link libraries
        char link_directories[_SSR_ARGS_BUF_LEN];
        _ssr_merge_in(link_directories, _SSR_ARGS_BUF_LEN, "/DEFAULTLIB", config->link_libraries, config->num_link_libraries);

#if defined(SSR_LINUX)
        char* fmt = "%s -shared -lm -o %s %s -lm 2>&1";
#else
        char* fmt = "%s -shared -lm -o %s %s";
#endif

        _ssr_str_t link = _ssr_str_f(fmt, SSR_CLANG_EXEC, _out, stages & _SSR_COMPILE ? compile_out.b : input);
		_ssr_log(SSR_CB_INFO, "Linking %s ...", input);
		
		_ssr_str_t err;
		int ret = _ssr_run(link.b, NULL, &err) == 0;
		_ssr_str_destroy(link);
		if (err.b != NULL)
		{
			if (ret)
				_ssr_log(SSR_CB_WARN, err.b);
			else
				_ssr_log(SSR_CB_ERR, err.b);
		}
		_ssr_str_destroy(err);

		if (!ret) goto end;
    }

    ret = true;
    end:
    _ssr_str_destroy(compile_out);
    return ret;
}

#if !defined SSR_GCC_EXEC
#	define SSR_GCC_EXEC "gcc"
#endif

static bool _ssr_compile_gcc(const char* input, ssr_config_t* config, const char* _out, int stages)
{
    bool ret = false;
    _ssr_str_t compile_out = _ssr_str_e();
    if (stages & _SSR_COMPILE)
    {
        compile_out = _ssr_str_f("%s.obj", _out);

        // defines
        char defines[_SSR_ARGS_BUF_LEN];
        _ssr_merge_in(defines, _SSR_ARGS_BUF_LEN, "-D", config->defines, config->num_defines);

        // include directories
        char include_directories[_SSR_ARGS_BUF_LEN];
        _ssr_merge_in(include_directories, _SSR_ARGS_BUF_LEN, "-I", config->include_directories, config->num_include_directories);

        const char* gen_dbg = config->flags & SSR_FLAGS_GEN_DEBUG ? "-g " : "";
        const char* opt_lvl = config->flags & SSR_FLAGS_GEN_OPT2 ? "-O2" : (config->flags & SSR_FLAGS_GEN_OPT1 ? "-O1" : "");

#if defined(SSR_LINUX)
        char* fmt = "%s -c -o %s %s %s %s -o %s %s -lm 2>&1";
#else
        char* fmt = "%s -c -o %s %s %s %s -o %s %s";
#endif

        _ssr_str_t compile = _ssr_str_f(fmt, SSR_GCC_EXEC, gen_dbg, opt_lvl, defines, include_directories, stages & _SSR_LINK ? compile_out.b : _out, input);
		_ssr_log(SSR_CB_INFO, "Compiling %s ...", input);
		
		_ssr_str_t err;
		int ret = _ssr_run(compile.b, NULL, &err) == 0;
		_ssr_str_destroy(compile);
		if (err.b != NULL)
		{
			if (ret)
				_ssr_log(SSR_CB_WARN, err.b);
			else
				_ssr_log(SSR_CB_ERR, err.b);
		}
		_ssr_str_destroy(err);

        if (!ret) goto end;
    }

    if (stages & _SSR_LINK)
    {
        // link libraries
        char link_directories[_SSR_ARGS_BUF_LEN];
        _ssr_merge_in(link_directories, _SSR_ARGS_BUF_LEN, "/DEFAULTLIB", config->link_libraries, config->num_link_libraries);

#if defined(SSR_LINUX)
        char* fmt = "%s -shared -o %s %s -lm 2>&1";
#else
        char* fmt = "%s -shared -o %s %s";
#endif

        _ssr_str_t link = _ssr_str_f(fmt, SSR_GCC_EXEC, _out, stages & _SSR_COMPILE ? compile_out.b : input);
		_ssr_log(SSR_CB_INFO, "Linking %s ...", input);
		
		_ssr_str_t err;
		int ret = _ssr_run(link.b, NULL, &err) == 0;
		_ssr_str_destroy(link);
		if (err.b != NULL)
		{
			if (ret) 
				_ssr_log(SSR_CB_WARN, err.b);
			else 
				_ssr_log(SSR_CB_ERR, err.b);
		}
		_ssr_str_destroy(err);

        if (!ret) 
			goto end;
    }

    ret = true;
    end:
    _ssr_str_destroy(compile_out);
    return ret;
}

static bool _ssr_compile(const char* path, ssr_config_t* config, const char* out, int stages)
{
	switch (config->compiler)
	{
	case SSR_COMPILER_MSVC:
		return _ssr_compile_msvc(path, config, out, stages);
	case SSR_COMPILER_CLANG:
		return _ssr_compile_clang(path, config, out, stages);
	case SSR_COMPILER_GCC:
		return _ssr_compile_gcc(path, config, out, stages);
	}
	return true;
}

// Internal
typedef struct __ssr_routine_t
{
	_ssr_str_t name; // name of the function
	void* addr; // current pointer to function address
	_ssr_vec_t moos; // listeners
	_ssr_lock_t moos_lock;
}_ssr_routine_t;

// Element in dameon script map, indexed by script id (relative path with SSR_SEP)
typedef struct __ssr_script_t
{
	_ssr_hash_t hash;
	_ssr_str_t id;
	_ssr_vec_t				routines; // list of routines, could use a map, but not worth atm 
	clock_t					last_seen; // removal after X time, data stays in memory for a bit (Ctrl+Z friendly)
	_ssr_timestamp_t		last_written; // for updating 
	_ssr_str_t				rnd_id; // current id of the dll (also part of filename
	_ssr_lib_t				lib; // current lib lodaded in memory
}_ssr_script_t;

typedef struct ssr_t
{
	ssr_config_t*		config; // global config
	char*				root; // base directory (your scripts/ directory)
	_ssr_thread_t		thread; // daemon thread
	volatile uint32_t	state; // for joining & keep track of wheter we generated a config or the user supplied one

	ssr_cb_t			cb;
	int					cb_mask;

#ifdef SSR_LIVE
	_ssr_map_t			scripts; // id -> _sso_script_t
	const char* bin;
#else
	_ssr_lib_t			lib; // single library when running 'release'
#endif
}ssr_t;

SSR_DEF bool ssr_init(struct ssr_t* ssr, const char* root, struct ssr_config_t* config)
{
	memset(ssr, 0, sizeof(ssr_t));
	ssr->root = _ssr_fullpath(root).b;
	ssr->state = 0x0;

#ifdef SSR_LIVE
	_ssr_map(&ssr->scripts, sizeof(_ssr_script_t), 0);
#endif

	if (config == NULL)
	{
		ssr->config = (ssr_config_t*)malloc(sizeof(ssr_config_t));
#if defined(__clang__)
		ssr->config->compiler = SSR_COMPILER_CLANG;
#elif defined(__GNUC__) || defined(__GNUG__)
		ssr->config->compiler = SSR_COMPILER_GCC;
#elif defined(_MSC_VER)
		ssr->config->compiler = SSR_COMPILER_MSVC;
#	if _MSC_VER >= 1910
		ssr->config->msvc_ver = SSR_MSVC_VER_14_1;
		ssr->config->msvc141_path = NULL;
#	elif _MSC_VER >= 1900
		ssr->config->msvc_ver = SSR_MSVC_VER_14_0;
#	else
#		error Previous MSVC versions have not been tested, but should probably work with the right paths
#	endif
#else
#	error Unsupported compiler
#endif

#if defined(SSR_32)
		ssr->config->target_arch = SSR_ARCH_X86;
#elif defined(SSR_64)
		ssr->config->target_arch = SSR_ARCH_X86;
#else
#	error Unrecognized platform
#endif
        ssr->config->flags = SSR_FLAGS_GEN_DEBUG;
        ssr->config->include_directories = NULL;
        ssr->config->num_include_directories = 0;
        ssr->config->link_libraries = NULL;
        ssr->config->num_link_libraries = 0;
        ssr->config->defines = NULL;
        ssr->config->num_defines = 0;
        ssr->config->compile_args_beg = NULL;
        ssr->config->compile_args_end = NULL;
        ssr->config->link_args_beg = NULL;
        ssr->config->link_args_end = NULL;
        ssr->state |= 0x2;
    }
    else
    {
        *ssr->config = *config;
    }

	if (ssr->config->compiler == SSR_COMPILER_MSVC)
	{
		if (ssr->config->msvc_ver == SSR_MSVC_VER_14_1) 
		{
			const char* vswhere_cmd = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe -property installationPath";
			_ssr_str_t output;
			_ssr_run(vswhere_cmd, &output, NULL);
			ssr->config->msvc141_path = _ssr_str_f("%s/VC/Auxiliary/Build/", output).b;
		} 
		else 
		{
			// TODO
			assert(false);
		}
	}

    /* Syntax:
        <filter>:[L]
        <command>:<value>
        ..
        EOF
        <filter> can be either a single file (relative from root) or a directory, no support for globbing here
        e.g. math/incr math/incr/
        L specifies if the commands are valid only when in live mode
        <command>:<value> represents a compile option in cmake style (w/o the target_):
        include_directories <path> [,<path>]
        link_directories <path> [,<path>]
        link_libraries <libname> [,<libname>]
        add_definitions <name>=<value> [,<name>=<value>]*
        [clang|msvc|gcc]:<arg>
    */
    return true;

}

static void _ssr_script_destroy(void* el, void* args)
{
    _ssr_script_t* script = (_ssr_script_t*)el;

    size_t routines_len = _ssr_vec_len(&script->routines);
    for (size_t i = 0; i < routines_len; ++i)
    {
        _ssr_routine_t* routine = (_ssr_routine_t*)_ssr_vec_at(&script->routines, i);
        _ssr_str_destroy(routine->name);
        _ssr_vec_destroy(&routine->moos);
        _ssr_lock_destroy(&routine->moos_lock);
    }

    _ssr_vec_destroy(&script->routines);
    _ssr_str_destroy(script->id);
    _ssr_str_destroy(script->rnd_id);
    _ssr_lib_destroy(&script->lib);
}

SSR_DEF void ssr_destroy(struct ssr_t* ssr)
{
#ifdef SSR_LIVE
    ssr->state |= 0x1;
    _ssr_thread_join(&ssr->thread);
#endif

    free(ssr->root);
    if (ssr->state & 0x2)
        free(ssr->config);

#ifdef SSR_LIVE
    _ssr_map_iter(&ssr->scripts, _ssr_script_destroy, NULL);
    _ssr_map_destroy(&ssr->scripts);
#else
    _ssr_lib_destroy(&ssr->lib);
#endif
}

#ifdef SSR_LIVE
static void _ssr_on_file(void* args, const char* base, const char* filename)
{
    const char* exts[] = { SSR_FILE_EXTS };
    ssr_t* ssr = (ssr_t*)args;

    // Extracting full path
    _ssr_str_t full_path = _ssr_str_f("%s/%s", base, filename);
    const char* ext = _ssr_extract_ext(filename);

    // Checking if file is of a valid extension
    size_t exti = 0;
    while (exts[exti][0] != '\0')
    {
        if (strcmp(exts[exti], ext) == 0)
            break;
        ++exti;
    }

    // In case skipping file
    if (exts[exti][0] == '\0') {
        _ssr_str_destroy(full_path);
        return;
    }

    // Computing script id
    // |root|rel_path|filename
    // |     base    |filename
    // ptr is in base
    const char* rel_path = _ssr_extract_rel(ssr->root, full_path.b);
    _ssr_str_t id = _ssr_replace_seps(rel_path, SSR_SEP);

    _ssr_script_t* script = (_ssr_script_t*)_ssr_map_find_str(&ssr->scripts, id.b);

    // new script, ssr has no permission to add scripts or routines, if script == NULL means
    // that no one is registered to listen to this file, no need to go further
    if (script == NULL || _ssr_vec_len(&script->routines) == 0)
        goto end;

    _ssr_timestamp_t ts = _ssr_file_timestamp(full_path.b);
    if (script->last_written < ts)
    {
        // Generating name for the share library
        _ssr_str_t _shared_lib_out = _ssr_str_rnd(SSR_SL_LEN);
        while (_ssr_file_exists(_shared_lib_out.b))
        {
            _ssr_str_destroy(_shared_lib_out);
            _shared_lib_out = _ssr_str_rnd(SSR_SL_LEN);
        }

        // Combining into full path
        _ssr_str_t shared_lib_out = _ssr_str_f("%s/%s.%s", ssr->bin, _shared_lib_out.b, _ssr_lib_ext());

        // Time to compile and link into dll
        bool compile_ret = _ssr_compile(full_path.b, ssr->config, shared_lib_out.b, _SSR_COMPILE_N_LINK);
        if (!compile_ret) {
            _ssr_log(SSR_CB_ERR, "something went wrong"); // todo
        }

        // Loading library
        _ssr_lib_t shared_lib;
        if (!_ssr_lib(&shared_lib, shared_lib_out.b))
		{
			_ssr_str_destroy(_shared_lib_out);
			_ssr_str_destroy(shared_lib_out);
			goto end;
		}

        // Updating map and all listeneres
        _ssr_script_t* script = (_ssr_script_t*)_ssr_map_find_str(&ssr->scripts, id.b);
        size_t routines_len = _ssr_vec_len(&script->routines);
        for (size_t i = 0; i < routines_len; ++i)
        {
            _ssr_routine_t* routine = (_ssr_routine_t*)_ssr_vec_at(&script->routines, i);

            // Finding new function pointer
            void* new_fptr = _ssr_lib_func_addr(&shared_lib, routine->name.b);

            _ssr_lock_acq(&routine->moos_lock);
            size_t moos_len = _ssr_vec_len(&routine->moos);
            for (size_t j = 0; j < moos_len; ++j)
            {
                ssr_routine_t* user_routine = *(ssr_routine_t**)_ssr_vec_at(&routine->moos, j);
                // Updating function pointer
                *user_routine = new_fptr;
            }
            _ssr_lock_rel(&routine->moos_lock);
        }

        // Can safely free library
        _ssr_lib_destroy(&script->lib);
        script->lib = shared_lib;

        // rnd_id is saved for cleaning
        _ssr_str_destroy(script->rnd_id);
        script->rnd_id = _shared_lib_out;

        script->last_written = ts;
        _ssr_str_destroy(shared_lib_out);
    }
    script->last_seen = clock();

    size_t routines_len = _ssr_vec_len(&script->routines);
    for (size_t i = 0; i < routines_len; ++i)
    {
        _ssr_routine_t* routine = (_ssr_routine_t*)_ssr_vec_at(&script->routines, i);
        void* addr = routine->addr;
        routine->addr = _ssr_lib_func_addr(&script->lib, routine->name.b);
        if (addr != routine->addr)
        {
            void* new_fptr = _ssr_lib_func_addr(&script->lib, routine->name.b);

            _ssr_lock_acq(&routine->moos_lock);
            size_t moos_len = _ssr_vec_len(&routine->moos);
            for (size_t j = 0; j < moos_len; ++j)
            {
                ssr_routine_t* user_routine = *(ssr_routine_t**)_ssr_vec_at(&routine->moos, j);
                // Updating function pointer
                *user_routine = new_fptr;
            }
            _ssr_lock_rel(&routine->moos_lock);
        }
    }
end:
    _ssr_str_destroy(full_path);
    _ssr_str_destroy(id);
}


//@main
#ifdef _WIN32 // && _MSC_VER
static DWORD WINAPI
#else
static void*
#endif
_ssr_main(void* args)
{
    ssr_t* ssr = (ssr_t*)args;

    _ssr_str_t bin_dir = _ssr_str_f("%s/%s", ssr->root, SSR_BIN_DIR);
    ssr->bin = bin_dir.b;

    // Setting up logging for current instance
    _ssr_log(0, NULL, ssr);
    //_ssr_log(SSR_CB_INFO, "Watcher running on %s", ssr->root);

    // Removing current bin directory
    _ssr_new_dir(bin_dir.b);

    while ((ssr->state & 0x1) == 0)
    {
        _ssr_iter_dir(ssr->root, _ssr_on_file, ssr);
        _ssr_sleep(SSR_SLEEP_MS);
    }

    _ssr_str_destroy(bin_dir);
    return EXIT_SUCCESS;
}
#else
static void _ssr_add_file_cb(void* args, const char* base, const char* filename)
{
    _ssr_vec_t* files = (_ssr_vec_t*)args;
    _ssr_str_t full_path = _ssr_str_f("%s/%s", base, filename);
    _ssr_vec_push(files, &full_path);
}
#endif

SSR_DEF bool ssr_run(struct ssr_t* ssr)
{
#ifdef SSR_LIVE
    if (_ssr_thread(&ssr->thread, (void*)_ssr_main, ssr))
    {
        // ERROR: Failed to launch thread
        return false;
    }
    return true;
#else
    bool ret = false;
    _ssr_vec_t files;
    _ssr_vec(&files, sizeof(_ssr_str_t), 12);
    _ssr_iter_dir(ssr->root, _ssr_add_file_cb, &files);

    _ssr_str_t bin = _ssr_str_f("%s/%s", ssr->root, SSR_BIN_DIR);
    _ssr_new_dir(bin.b);

    size_t files_len = _ssr_vec_len(&files);
    if (files_len == 0)
        goto end_no_files;

    char** defines = (char**)malloc(sizeof(char*) * (ssr->config->num_defines+1));
    for (size_t i = 0; i < ssr->config->num_defines; ++i)
        defines[i] = ssr->config->defines[i];
    ++ssr->config->num_defines;
    ssr->config->defines = defines;

    size_t linker_input_len = 0;
    size_t linker_input_cap = 1024;
    char* linker_input = (char*)malloc(linker_input_cap);
    linker_input[0] = '\0';
    for (size_t i = 0; i < files_len; ++i)
    {
        _ssr_str_t* path = (_ssr_str_t*)_ssr_vec_at(&files, i);
        _ssr_str_t out = _ssr_str_f("%s/%d.obj", bin.b, i);

        size_t out_len = strlen(out.b);
        if (out_len + linker_input_len > linker_input_cap)
        {
            linker_input = (char*)realloc(linker_input, linker_input_cap * 2); // TODO: max..
            linker_input_cap *= 2;
        }

        snprintf(linker_input, linker_input_cap, "%s %s", linker_input, out.b);
        linker_input_len += out_len;

        // In order to avoid name clashes <script-id>_<func>
        const char* script_rel = _ssr_extract_rel(ssr->root, ((_ssr_str_t*)_ssr_vec_at(&files, i))->b);
        _ssr_str_t script_id = _ssr_replace_seps(script_rel, SSR_SEP);
        char buf[1024];
        size_t buf_len = snprintf(buf, 1024, "\"SSR_SCRIPTID=%s\"", script_id.b);
        _ssr_str_destroy(script_id);
        defines[ssr->config->num_defines-1] = (char*)malloc(buf_len+1);
        _ssr_strncpy(defines[ssr->config->num_defines-1], buf, buf_len+1);

        bool ret = _ssr_compile(path->b, ssr->config, out.b, _SSR_COMPILE);
        _ssr_str_destroy(out);
        free(defines[ssr->config->num_defines - 1]);
        if (!ret)
            goto end_no_lib;
    }

    _ssr_str_t out = _ssr_str_f("%s/%s.%s", bin.b, "ssr", _ssr_lib_ext());
    bool link_ret = _ssr_compile(linker_input, ssr->config, out.b, _SSR_LINK);
    if (!link_ret)
        goto end;

    // loading library & hooking up functions
    _ssr_lib(&ssr->lib, out.b); // todo check valid

    ret = true;

end:
    _ssr_str_destroy(out);
end_no_lib:
    free(linker_input);
    free(defines);
end_no_files:
    _ssr_str_destroy(bin);

    for (size_t i = 0; i < files_len; ++i)
        _ssr_str_destroy(*(_ssr_str_t*)_ssr_vec_at(&files, i));
    _ssr_vec_destroy(&files);
    return ret;
#endif
}

SSR_DEF bool ssr_add(struct ssr_t* ssr, const char* script_id, const char* fname, ssr_func_t* user_routine)
{
#ifdef SSR_LIVE
    _ssr_script_t* script = (_ssr_script_t*)_ssr_map_find_str(&ssr->scripts, script_id);

    // First time request
    if (script == NULL)
    {
        _ssr_script_t new_script;
        _ssr_vec(&new_script.routines, sizeof(_ssr_routine_t), 32);
        new_script.last_seen = 0;
        new_script.last_written = 0;
        new_script.rnd_id.b = NULL;
        new_script.id = _ssr_str(script_id);
        new_script.lib.h = NULL;
        _ssr_map_add_str(&ssr->scripts, &new_script);
    }
    script = (_ssr_script_t*)_ssr_map_find_str(&ssr->scripts, script_id); // TODO can be avoided

    _ssr_routine_t* routine = NULL;
    size_t routines_len = _ssr_vec_len(&script->routines);
    for (size_t i = 0; i < routines_len; ++i)
    {
        _ssr_routine_t* _routine = (_ssr_routine_t*)_ssr_vec_at(&script->routines, i);
        if (strcmp(fname, _routine->name.b) == 0)
        {
            routine = _routine;
            break;
        }
    }

    if (routine == NULL)
    {
        _ssr_routine_t new_routine;
        new_routine.name = _ssr_str(fname);
        new_routine.addr = NULL;
        _ssr_vec(&new_routine.moos, sizeof(void*), 32);
        _ssr_lock(&new_routine.moos_lock);

        _ssr_lock_acq(&new_routine.moos_lock);
        _ssr_vec_push(&new_routine.moos, &user_routine);
        _ssr_lock_rel(&new_routine.moos_lock);

        _ssr_vec_push(&script->routines, &new_routine);
        routine = (_ssr_routine_t*)_ssr_vec_at(&script->routines, _ssr_vec_len(&script->routines) - 1);
    }

    // Registering as listener
    void* routine_addr = routine->addr;
    if (routine_addr != NULL)
        *user_routine = routine_addr; // Might be already up to date but not a problem

#else
    _ssr_str_t func = _ssr_str_f("%s$%s", script_id, fname);
    void* fptr = _ssr_lib_func_addr(&ssr->lib, func.b);
    if (fptr == NULL)
    {
        // todo log
        return false;
    }
    (*user_routine) = fptr;
#endif
    return true;
}

SSR_DEF void ssr_remove(struct ssr_t* ssr, const char* script_id, const char* fname, ssr_func_t* user_routine)
{
#ifdef SSR_LIVE
    _ssr_script_t* script = (_ssr_script_t*)_ssr_map_find_str(&ssr->scripts, script_id);
    if (script == NULL)
        return;

    size_t num_routines = _ssr_vec_len(&script->routines);
    for (size_t i = 0; i < num_routines; ++i)
    {
        _ssr_routine_t* routine = (_ssr_routine_t*)_ssr_vec_at(&script->routines, i);
        if (strcmp(routine->name.b, fname) == 0)
        {
            _ssr_vec_remove(&routine->moos, &user_routine);

            if (_ssr_vec_len(&routine->moos) == 0) {} // TODO: Remove subnode
            return;
        }
    }
#else
    (void)ssr; (void)script_id; (void)fname; (void)user_routine;
#endif
}

SSR_DEF void ssr_cb(struct ssr_t* ssr, int mask, ssr_cb_t cb)
{
    ssr->cb_mask = mask;
    ssr->cb = cb;
}

static void _ssr_log(int type, const char* fmt, ...)
{
    static ssr_t* ssr = NULL;

    if (fmt == NULL)
    {
        va_list args;
        va_start(args, fmt);
        ssr = va_arg(args, ssr_t*);
        va_end(args);
        return;
    }

    if (ssr == NULL || ssr->cb == NULL || (type & ssr->cb_mask) == 0x0)
        return;

    char log_buf[SSR_LOG_BUF];
    va_list args;
    va_start(args, fmt);
    vsnprintf(log_buf, SSR_LOG_BUF, fmt, args);
    va_end(args);
    ssr->cb(type, log_buf);
}
#endif
