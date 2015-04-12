# daemon.py
# Usage:
#   python daemon.py [Mode] [Script's root directory] [Update host] [Update port] [Stream host] [Stream port]
#   [Mode] -> can be either Test or Release
#   [SRD]  -> root directory the daemon will be run onto
#   [Update port] -> port that we should connect to when sending update notifications
#   [Stream port] -> port that we should connect to when sending stdout
# Note:
#   Tested on python 3.4, should work on all 3.X versions
import environment as env
import connection  as conn
import filer
import time

idle_time = 0.1

def parse_arguments():
    pass

def main():
    print("[D][Info]Starting daemon") # TODO Remove
    environment = env.Environment()
    if not environment.setup():
        print("[D][Error]Invalid execution line")

    connection = conn.ClientConnection(environment, int(environment.update_port), int(environment.update_port) + 1,
                                       False if environment.mode is env.Environment.Mode.Test else True)

    file_cache = filer.FileCache(environment.root_directory, connection)
    if not file_cache.init_dirs():
        return False

    if environment.mode is environment.Mode.Release:
        connection.try_connect()
        file_cache.update()
        return

    while True:
        connection.try_connect()
        file_cache.update()
        time.sleep(idle_time)

    return True

if __name__ == "__main__":
    main()