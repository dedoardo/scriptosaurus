/*
    File: scriptosaurus.hpp
        File that has to be included in every script to make function exporting possible. Modifying this 
        file might cause the API to not work.
    Author: Edoardo 'sparkon' Dominici
    License: Mozilla Public License 2.0
    Note: 
    	If you want to know more about compilation flags, see the Flags.md file in the root directory
*/

#define dll_export __attribute__((dllexport))
#define no_mangle extern "C"

/*
	Function: init
		Simple function that is supposed to be used as initialization. This is loaded once when the script is
		first compiled, if you modify it and want to call it again w/o touching the source code specify force_init(true)

	Parameters:
		data - This parameter is saved inside the Script itself, when a force_init is set the same parameter is reused
*/
no_mangle dll_export bool init(void* data);

/*
	Function: update
		This function is the core, it is reloaded every recompilation

	Parameters:
		data - Pointer to custom data
*/
no_mangle dll_export void update(void* data);

/*
	Function: destroy
		Called when the script handle in the client code is destroyed, it is reloaded every recompilation

	Parameters:
		data - Pointer to custom data
*/
no_mangle dll_export void destroy(void* data);