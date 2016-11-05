# Scriptosaurus - Single-Header C++ Live Scripting ( MIT License ) 

* **Must Read**
* **Basic Usage** 
* **Supported Platforms**
* **Advanced Usage**
  * **x86/x64**
  * **Scripting metadata**
  * **Subdirectories**
  * **Preprocessor directives**
  * **Logging**
  * **Porting**
* **Planned features / Wish list**
* **Features / How it works**

### Must read

Currently this is very unstable. It is being used for another project and thus will hopefully receive tons of testing and bug-fixing in the near future, as well as more portings to other compilers / platforms.

Parts of the README are yet to be completed mostly because the code is very subject to change. Planned Features contains the things I'm currently working on.

P.S. I lied, there are 2 headers, one for the caller and one for the callee ( script ).

### Basic Usage

`main.cpp`

```c++
#define scriptosaurus_live 	// Live editing
#define scriptosaurus_win32 // Platform
#define scriptosaurus_msvc "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin" 
// ^ Fell free to change it depending on the architecture you are compiling for
#include "scriptosaurus.hpp" // Single header

// The base class only makes sure you assign an immutable name
struct SampleScript : public scriptosaurus::Script
{
	SampleScript() : Script("SampleScript") { } 
  
  	// Declares a swappable routine
	scriptosaurus_routine(sample_callback);
};

void main()
{
	using namespace scriptosaurus;
  
  	// Handles script's reloading
	ScriptDaemon daemon("scripts"); // << Root directory
  
  	// Your script instance
	SampleScript s1;
	daemon.add(s1, { 
		scriptosaurus_prepare(s1, sample_callback)
	});

  	// Launches daemon
	daemon.link_all();
	
	while (true) 
	{
		scriptosaurus_call(s1, sample_callback, nullptr);
	}
}
```

`scripts/SampleScript.cpp`

```c++
/*
	:scriptosaurus
	debug: true
	:end
*/

#include <iostream>
#include "../scriptsaurus_routines.hpp"

script_routine(sample_callback)
{
	std::cout << "b";
}
```

**What is all of the above?**

`scriptosaurus_routine` Declares a routine whose pointer can be swapped at runtime.

`scriptosaurus_prepare` Saves a pointer to the specified function

`scriptosaurus_call` When not testing ( `undefined scriptosaurus_live` ) simply calls the function, otherwise performs operations to ensure that the function has been correctly loaded by the system and that is not in use by it.

`daemon.link_all` When not testing ( `undefined scriptosaurus_live` ) creates a dynamic library containing all the symbols ( manually mangled to avoid multiple references ), otherwise simply launches the daemon thread.

**Note**: If you want more information just browse `scriptosaurus.hpp`.

### Supported platforms

List of supported or to be supported platforms / compilers for the immediate future.

| Backend                  | Supported | Planned? |
| ------------------------ | :-------: | :------: |
| Win32                    |    Yes    |          |
| Posix                    |    No     |   Yes    |
| MSVC2015                 |   Yes*    |   Done   |
| MSVC2013                 |    No     |   No**   |
| Clang 3.8 ( under MSVC ) |    Yes    |   Done   |
| GCC??                    |    No     |    No    |

- `*`  Visual studio 2015 keeps `.pdb` pinned down even if `FreeLibrary` is called , thus if you want to enable live scripting you have to check `Tools > Options > Debugging > General > Use Native Compatibility Mode` . This will use an old debugging engine where some preview features are unfortunately not supported. 
- `**` Visual studio 2013 hasn't been tested yet, if you want to know if it works you should: 
  - See if all C++11+ features are supported
  - Find a way to use the native debugging engine if you want the live scripting
- `***` Very soon

### Advanced Usage

#### x64/x86 

By default `scriptosaurus` is 32-bit and you should define the compiler paths accordingly. 

```
#define scriptosaurus_msvc "<path-to-32bit-msvc-compiler>"
#include <scriptosaurus.hpp>
```

```
#define scriptosaurus_64bit
#define scriptosaurus_msvc "<path-to-64bit-msvc-compiler>"
#include <scriptosaurus.hpp>
```

You can specify `scriptosaurus_32bit` if you want, but it's optional.

**Note**: Remember to make sure that the binary's architecture matches the `scriptosaurus`' one. 

**Note**:Currently there is no "clear" API for cross-compiling, will work on that soon.

#### Scripting Metadata

Currently only able to produce debug symbols, will be extended soon to allow for including/linking.

Just put this wherever you want in the c++ code, only the first block is currently parsed.

```
scriptosaurus:
debug:[true|false]
end:
```



#### Subdirectories

You can nest scripts inside the root directory. When referring to them via the `c++` code simply substitute the path separator (`/` `\\`) with `@` ( See Preprocessor directives to change it ).

```c
scripts/ <= Root directory
	ScriptA.cpp
	sub1/	<= Sub directory 1
		ScriptA.cpp
	sub2/   <= Sub directory 2
		ScriptA.cpp
		sub3/ <= Sub directory 3
			ScriptA.cpp
```

You can refer to the scripts in the following way. **Note** how the extension is not specified in the ID.

```
Script("ScriptA")
Script("sub1@ScriptA")
Script("sub2@ScriptA")
Script("sub2@sub3@ScriptA")
```

#### Preprocessor directives 

TODO

#### Logging

TODO

#### Porting

TODO

### Planned Features

- Extended metadata API ( Basic compiler/linker arguments )
- Multiple script instances
- Catch dem exceptions
- Proper string wrapper that handles `path` +`basename` + `extension`.  Performance is not critical and a nice immutable interface would be nice.
- Ensure same calling convention
- Properly redirect compiler output
- Ensure that target architecture is the same as the one the executable is being compiled with.
- Cross-compilation
- Reduce clutter generated by linking/compiling
- C++ STL is literally abused especially string, might reduce it's usage when implementing the wrapper.

### How it works

TODO