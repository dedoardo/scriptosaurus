##Scriptosaurus

### Introduction
Scriptosaurus is a C++ scripting system that allows you to use C++ as a scripting system and modify scripts at runtime. 

**Features** : 
- Debug / Release mode 
- stdout / stderr ( not yet ) synched ( via client side callback ) 
- Modify script and runtime and specify custom compiler flags / what to do all inside the script.
- Script is 100% portable to the client application, no custom syntax or anything else.
- Separate script / output channel ( run script on one machine and redirect output to another ) 
- Script subdirectories and addressing works

### TODOs:
- MSVC debugging ( .pdb files are copied, but still something doesn't work )
- Exception handling
- ```init()``` not properly called
- UNIX port

### Writing a script 

### Addressing a script from the client API
Since we support subdirectories if you need to address a file contained in a subdirectory you just need to substitute the ```//``` with ```:```, like this :
```
// Directory structure : 
// Scripts \ <-- You set this as root directory when initializing the ScriptManager
//      sub \
//          script.cpp

Script sub_script("sub:script");
```

### Compiler options
This chapter aims at explaining how you can specify compiler options for file. If you glance through the API you will notice that there is no reference to compiler flags. This is because everything is handled inside the script itself. Each script consist of a single .cpp file and it has its own flags. They are specified at the beginning of the file, **inside the first comment block**.  
**What do you mean by comment block ?**  
For commenting both the C & C++ styles are supported, being /\* \*/ or //. For the C comments the parser will stop when he finds the */, on the C++ side of things it will stop when he finds a line that does not start with //. Some examples :  
```
/*
This is considered scriptosaurus related
*/ <= We stop here
/*
This is not parsed
*/
```
```
// Valid
// Valid
// Valid
// Valid
void function_declaration(type parameters);
// Not valid
```
** Well, what then ? **  
We handle compiler flags via a restricted set of functions ( similar to the cmake ones ), First off i'm going to explain the syntax, simply :
```
// function_name(parameter)
```
Quite simple isn't it ;)

** Cool i guess.. what about these functions **  
Here you go this is a list of the supported functions as for now  
``` add_link(libraryname)  ``` Maps to -l command in g++ ( with -static flag ), simply links to the specified static library
``` add_link_directory(path_to_library) ``` Maps to -L command in g++, adds a directory to the list that is searched when trying to link  
``` add_include_directory(path) ``` Maps to -I command in g++, adds a directory to the list of include directories  
``` compile(true | false) ``` If false the script won't be compiled, if true it will be compiled. This is useful when writing a larger source file and you don't want to continuosly compile it every time you press Ctrl + S.  
``` force_init(true | false) ``` If the script is already running but you want to call the init() function again ( for some initialization purpose, set true. Defaults to false. It is useful when you may need to respawn an object or similar situations
