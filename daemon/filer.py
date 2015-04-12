# File: file_cache.py

import os
import shutil
import filecmp
import connection as conn
import compiler

if __name__ == "__main__":
    print("[D][Error]See daemon.py for usage")

class File:
    def __init__(self, name, location, extension, subdir):
        self.name = name
        self.location = location
        self.extension = extension
        self.subdir = subdir

    def __hash__(self):
        return (self.name + self.location + self.extension).__hash__()

    def __eq__(self, other):
        return (self.name + self.location + self.extension) == (other.name + other.location + other.extension)

    def get_full_path(self):
        return os.path.join(self.location + self.name + "." + self.extension)

    def get_codename(self):
        # We add \\ at the end of every path, but when loading file that are located in the
        # root directory it's stupid to have a leading _ that's why we remove it
        if self.subdir == '\\':
            return self.name
        return self.subdir.replace('\\', '_') + self.name

    @staticmethod
    def get_extension(filename):
        return filename.split('.')[-1]

    @staticmethod
    def get_relative(root, current):
        if len(root) > len(current):
            return ""
        return current[len(root) + 1:] + '\\'

class FileCache:
    def __init__(self, root_directory, connection):
        self.cache = { }
        self.root_directory = root_directory
        self.cache_directory = os.path.join(root_directory, ".cache\\")
        self.bin_directory = os.path.join(root_directory, ".bin\\")
        self.connection = connection

    def init_dirs(self):
        # Creating cache directory
        try:
            if not os.path.exists(self.cache_directory):
                os.makedirs(self.cache_directory)

            if not os.path.exists(self.bin_directory):
                os.makedirs(self.bin_directory)

        except os.error as error:
            self.connection.log(conn.MessageLevel.Error, "Failed to create .cache and .bin directories")
            return False
        return True

    def update(self):
        current_files = list_all_files(self.root_directory, ["cpp"])
        old_files = list(key for key in self.cache)

        # Calculating differences and invoking callbacks
        self.process_new(list(set(current_files) - set(old_files)))
        self.process_diff(list(set(current_files) & set(old_files)))
        self.process_del(list(set(old_files) - set(current_files)))

    def process_file(self, file, cached_file, is_last_in_queue):
        try:
            # Copy modified file to destination
            shutil.copyfile(file.get_full_path(), cached_file.get_full_path())

            # Proprocessing file header
            update_options = conn.UpdateOption.Default
            compiler_options = compiler.preprocess(cached_file.get_full_path())

            # Removing trailing and leading spaces ( if any ) TODO: Might not need this control, check regex
            if compiler_options is None:
                compiler_options = []
                for i in range(len(compiler.supported_commands)):
                    compiler_options.append([])

            print(compiler_options)

            for i in range(len(compiler_options)):
                compiler_options[i] = list(map(lambda x : x.lstrip().rstrip(), compiler_options[i]))

            # Checking if the user wants the file to be compiled
            # If multiple compile() flags are specified the
            if compiler_options[compiler.Command.Compile]:
                if compiler_options[compiler.Command.Compile][0].lower() == "false":
                    return

                if (compiler_options[compiler.Command.Compile][0].lower() != "true"):
                    self.connection.log(conn.MessageLevel.Warning, "compile(token) : token not recognized, true assumed")

            if not compiler.compile(cached_file, self.root_directory, self.bin_directory,
                                    compiler_options[compiler.Command.AddIncludeDirectories],
                                    compiler_options[compiler.Command.AddLinkDirectories],
                                    compiler_options[compiler.Command.AddLink])[0]:
                self.connection.log(conn.MessageLevel.Error, "Failed to compile : " + file.name)
                return

            if compiler_options[compiler.Command.ForceInit]:
                if compiler_options[compiler.Command.ForceInit][0].lower() == "true":
                    update_options |= conn.UpdateOption.ForceInit
                if compiler_options[compiler.Command.ForceInit][0].lower() != "false":
                    self.connection.log(conn.MessageLevel.Warning, "force_init(token) : token not recognized, true assumed")

        except IOError:
                self.connection.log(conn.MessageLevel.Error, """
                    Failed to write to cache directory source : {0} | target : {1}
                """.format(file.get_full_path(), cached_file.get_full_path()))

        self.connection.log(conn.MessageLevel.Info, "Succesfully compiled " + file.name)

        if is_last_in_queue:
            update_options |= conn.UpdateOption.LastInQueue

        self.connection.notify(conn.UpdateNotification(file.get_codename().encode(), update_options))

    def process_new(self, files):
        for file in files:
            self.connection.log(conn.MessageLevel.Info, "New file found : " + file.name)
            cached_file = File(file.get_codename(), self.cache_directory, "cpp", file.subdir)
            self.process_file(file, cached_file, True if file == files[-1] else False)
            self.cache[file] = cached_file

    def process_diff(self, files):
        for file in files:
            if not filecmp.cmp(file.get_full_path(), self.cache[file].get_full_path()):
                self.connection.log(conn.MessageLevel.Info, "Different file found : " + file.name)
                self.process_file(file, self.cache[file], True if file == files[-1] else False)

    def process_del(self, files):
        for file in files:
            self.connection.log(conn.MessageLevel.Info, "File {0} has been deleted".format(file.name))

            # Keeping the file in the cache for now, can be helpful to the user to restore lost files
            del self.cache[file]

def list_all_files(root_directory, extensions):
    file_list = []
    for dirname, dirs, files in os.walk(root_directory):
        # Ignoring .cache .bin directories
        if ".bin" in dirs:
            dirs.remove(".bin")
        if ".cache" in dirs:
            dirs.remove(".cache")

        # Parsing valid files
        for file in files:
            extension = File.get_extension(file)
            # Checking if they file's extension matches any of the provided ones
            if any(map(lambda x: True if x == extension else False, extensions)):
                file_list.append(File(os.path.splitext(file)[0], dirname + "\\", extension, File.get_relative(root_directory, dirname)))
    return file_list