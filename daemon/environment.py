# File: environment.py
import sys

if __name__ == "__main__":
    print("[D][Error]See daemon.py for usage")

# Represents the main source for informations
class Environment:
    class Mode:
        Release = "Release"
        Test    = "Test"

    class Arguments:
        Mode            = 1
        RootDirectory   = 2
        UpdatePort      = 3
        StreamPort      = 4

    def __init__(self):
        self.mode = self.Mode.Test
        self.root_directory = None
        self.update_port = None
        self.stream_port = None

    # Parses the command line arguments
    # Checks are quite shallow, will add more later
    # TODO: Add better checks ^^^^^^^^^^^^^^^^^^^^^
    def setup(self):
        if len(sys.argv) != 5:
            return False

        if sys.argv[self.Arguments.Mode] == self.Mode.Test:
            self.mode = self.Mode.Test
        elif sys.argv[self.Arguments.Mode] == self.Mode.Release:
            self.mode = self.Mode.Release
        else:
            return False

        self.root_directory = sys.argv[self.Arguments.RootDirectory]

        self.update_port = sys.argv[self.Arguments.UpdatePort]
        if not self.update_port.isdigit():
            return False

        self.stream_port = sys.argv[self.Arguments.StreamPort]
        if not self.stream_port.isdigit():
            return False
        return True