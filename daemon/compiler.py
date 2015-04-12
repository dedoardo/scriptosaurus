# File: compiler.py
import os
import subprocess
import re

if __name__ == "__main__":
    print("[D][Error]See daemon.py for usage")

class Command:
    AddLink = 0
    AddLinkDirectories = 1
    AddIncludeDirectories = 2
    Compile = 3
    ForceInit = 4


supported_commands = [
    "add_link",
    "add_link_directories",
    "add_include_directories",
    "compile",
    "force_init"
]

mapped_commands = {
    "add_link" : Command.AddLink,
    "add_link_directories" : Command.AddLinkDirectories,
    "add_include_directories" : Command.AddIncludeDirectories,
    "compile" : Command.Compile,
    "force_init" : Command.ForceInit
}

# Preprocesses the file and extract useful informations such as flags and more
def parse_command(command):
    # Finding function name and argument and making sure command is a valid one
    match = re.search(r"^(?P<function_name>[a-zA-Z_]*)\((?P<argument>[/:0-9a-zA-Z_ ]*)\)", command)
    if (not match.group("function_name")) or (not match.group("argument")):
        return None

    try:
        matched_command = supported_commands.index(match.group("function_name"))
    except ValueError:
        # TODO: Compute some string distance ? should be fun to write such a system
        return None

    return (match.group("function_name"), match.group("argument"))

def preprocess(file):
    # Regex for single-line comments, ensure that there is a space after the last line
    # //\s*([a-zA-Z_]*\s*\([a-zA-Z_ ]*\))\n
    # Regex for multiple lines
    # ([a-zA-Z_]*\(.*?\))
    # Everything has to be done after removing all spaces
    with open(file) as source_file:
        code = source_file.read()

    # First off we check if the file starts with /* or //
    match = re.search("^\s*(//|/\*).*?", code)
    if match is None:
        return

    # List of commands obtained by parsing the system
    commands = []

    # As by specification if the // is used, every line has to start
    if match.group(0) == "//":
        # Finding the last valid // line
        last_match = None
        for last_match in re.finditer("(\s*//.*?)[\n]+", code):
            pass

        if last_match is None:
            return commands

        comment_section = code[:last_match.span()[1]]
        # After finding all the interesting lines, we are parsing the valid ones

        # TODO: Add one last newline to make sure the last line is added found by the regex, dunno
        # why it can't be found
        comment_section = comment_section.replace(" ", "") # Trimming space for easier parsing and reduces regex clutter
        commands = re.findall("//([a-zA-Z_]*\([/:0-9a-zA-Z_ ]*\))\n", comment_section)

    elif match == "/*":
        pass

    compile_flags = [[] for _ in range(len(supported_commands))]
    for command in commands:
        flag = parse_command(command)
        if flag:
            compile_flags[mapped_commands[flag[0]]].append(flag[1])

    return compile_flags

def compile(file, root_directory, bin_directory, include_directories, link_directories, link_libraries):
    return compile_gcc(file, root_directory, bin_directory, include_directories, link_directories, link_libraries)

# MSVC cl
def compile_msvc(file, root_directory, bin_directory, include_directories, link_directories, link_libraries):
    cl_dir = "\"C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\bin\\amd64\\cl.exe\""
    output_file = os.path.join(bin_directory, file.name + "_tmp.dll")
    debug_file = os.path.join(bin_directory, file.name + "_tmp.pdb")
    # TODO : Implement debugging /DEBUG /PDBALTPATH:{2}
    compile_command = "{0} /D_USRDLL /D_WINDLL /EHsc {1} /MT /link /DLL /OUT:{2}".format(cl_dir,  file.get_full_path(), output_file)

    bat_file_path = os.path.join(bin_directory, "cl_compile.bat")

    with open(bat_file_path, "w") as bat_file:
        bat_file.write("\"C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\bin\\amd64\\vcvars64.bat\" && ")
        bat_file.write(compile_command)

    ret_value = subprocess.call(bat_file_path)
    if ret_value:
        print(ret_value)
    # Removing bat file
    os.remove(bat_file_path)

    return (True, "Failed to compile")

# GNU g++
def compile_gcc(file, root_directory, bin_directory, include_directories, link_directories, link_libraries):
    formatted_include_paths = ""
    for path in map(lambda x : "-I\"" + x, include_directories):
        formatted_include_paths += path + "\" "

    formatted_link_paths = ""
    for path in map(lambda x : "-L\"" + x, link_directories):
        formatted_link_paths += path + "\" "

    formatted_links = ""
    for link in map(lambda x : "-l\"" + x, link_libraries):
        formatted_links += link + "\" "

    try:
        # TODO : should i add -fPIC flag ?
        object_command = "g++ -g -O0 -std=c++0x {0} {1} {2} -c -o {3} {4}".format(formatted_include_paths,formatted_link_paths, formatted_links, os.path.join(bin_directory, "tmp.o"), file.get_full_path())
        ret_value = subprocess.call(object_command)
        if ret_value:
            return (False, "Failed to compile " + file.name)
        link_command = "g++ -shared -o {0} {1}".format(os.path.join(bin_directory, file.name + "_tmp.dll"), os.path.join(bin_directory, "tmp.o"))
        ret_value = subprocess.call(link_command)
        if ret_value:
            return (False, "Failed to link " + file.name)
        return (True, "")
    except IOError as error:
        return (False, "Failed to compile")