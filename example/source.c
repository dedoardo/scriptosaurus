#include <stdlib.h>

#define SSR_LIVE
#define SSR_IMPLEMENTATION
#include "../scriptosaurus.h"

// !!! remember that this is called asynchronously
void msg_callback(int type, const char* msg)
{
	if (type == SSR_CB_ERR || type == SSR_CB_WARN)
		fprintf(stderr, "ssr: %s\n", msg);
	else
		fprintf(stdout, "ssr: %s\n", msg);
}

struct Entry
{
	_ssr_hash_t hash;
	_ssr_str_t id;
	int x;
};

_ssr_map_t map;

typedef float(*func_t)(float);
int main()
{
	ssr_t ssr;

	// !!! SET YOUR PATH HERE
	// Doesn't necessarily have to be global, just make sure it's relative to your working directory
	ssr_init(&ssr, "scripts", NULL);
	
	ssr_cb(&ssr, SSR_CB_ERR | SSR_CB_WARN | SSR_CB_INFO, msg_callback); 

	// Always call run before any other ssr_add
	ssr_run(&ssr);

	ssr_func_t func = NULL;
	while (true)
	{
		char script_name[255];
		char func_name[255];
		float arg;
		printf(">");
		scanf("%s %s %f", script_name, func_name, &arg);

        if (strcmp("quit", script_name) == 0)
            break;

		ssr_add(&ssr, script_name, func_name, &func);
		while (func == NULL); // Waiting for script to be compiled
		float ret = (*(func_t)func)(arg);
		printf("%f\n", ret);
		ssr_remove(&ssr, script_name, func_name, &func);
		func = NULL;
	}

	ssr_destroy(&ssr);
 	return EXIT_SUCCESS;
}
