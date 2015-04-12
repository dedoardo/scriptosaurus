# File: connection.py
import ctypes
import socket

if __name__ == "__main__":
    print("[D][Error]See daemon.py for usage")

# Structure that represents messages sent to the client
class UpdateNotification(ctypes.Structure):
    _fields_ = [("filename", ctypes.c_char * 62),
               ("init", ctypes.c_ushort)]

class UpdateOption:
    Default   = 0
    ForceInit = 1
    LastInQueue = 2

# I'd use enumerators, but you need to install a backport if you want to use them
# since they have been introduced in python 3.4
class MessageLevel:
    Error    = 0
    Warning  = 1
    Info     = 2

    names = ["Error", "Warning", "Info"]

class ClientConnection:
    def __init__(self, environment, update_port, stream_port, blocking):
        self.environment = environment
        self.update_port = update_port
        self.stream_port = stream_port

        self.server_socket_update = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket_stream = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.update_socket = None # Socket used to send informations
        self.stream_socket = None # Socket used to stream messages to avoid I/O overlapping


        self.server_socket_update.bind(("", self.update_port))
        self.server_socket_update.listen(1)
        self.server_socket_stream.bind(("", self.stream_port))
        self.server_socket_stream.listen(1)

        self.buffered_notifications = []
        self.buffered_messages = []

        if not blocking:
            self.server_socket_update.setblocking(0)
            self.server_socket_stream.setblocking(0)

    def try_connect(self):
        # If both the update and the stream connection have been established then nothing has to be done here
        if self.update_socket is None:
            try:
                self.update_socket, update_address = self.server_socket_update.accept()
                self.log(MessageLevel.Info, "Established update channel with client from " + str(update_address))

                # Finally we can send all notifications that have been queued
                for notification in self.buffered_notifications:
                    self.notify(notification)
            except socket.error as error:
                pass

        if self.stream_socket is None:
            try:
                self.stream_socket, stream_address = self.server_socket_stream.accept()
                self.log(MessageLevel.Info, "Established stream channel with client from " + str(stream_address))

                # Finally we can send all the messages that have been queued
                for (message, level) in self.buffered_messages:
                    self.log(message, level)

            except socket.error as error:
                pass

    def notify(self, notification):
        if self.update_socket is None:
            self.buffered_notifications.append(notification)
        else:
            # Setting to proper byte order before sending
            notification.init = socket.htons(notification.init)

            try:
                self.update_socket.send(notification)
            except socket.error as error:
                self.log(MessageLevel.Error, "Failed to send update notification to client")

    def log(self, level, message):
        if self.stream_socket is None:
            self.buffered_messages.append((level, message))
        else:
            # Todo : Switch based on environment level
            try:
                composed_message = "[D][{0}]{1}".format(MessageLevel.names[level], message)
                self.stream_socket.send(ctypes.c_int32(len(composed_message)))
                self.stream_socket.send(composed_message.encode())
            except socket.error as error:
                print("[D][Error]Failed to send message to the client")

    def close(self):
        pass