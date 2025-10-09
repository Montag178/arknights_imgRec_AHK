# server.py
import threading
import os
import win32pipe, win32file, pywintypes
import sys

stop_event = threading.Event()

class PipeServer():
    def __init__(self, pipeName):
        self.pipe = win32pipe.CreateNamedPipe(
            r'\\.\pipe\{}'.format(pipeName),
            win32pipe.PIPE_ACCESS_DUPLEX,
            win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT,
            1, 65536, 65536,
            0,
            None)
        self.pipename = pipeName

    #Careful, this blocks until a connection is established
    def connect(self):
        win32pipe.ConnectNamedPipe(self.pipe, None)

    #Message without tailing '\n'
    def write(self, message):
        win32file.WriteFile(self.pipe, message.encode() + b'\n')

    def read(self, size=64*1024):
        hr, data = win32file.ReadFile(self.pipe, size)
        return data.decode("utf-8").strip()

    def close(self):
        win32file.CloseHandle(self.pipe)

def server():
    try:
        pipe = PipeServer("MyPipe")
        pipe.connect()
        message = pipe.read()
        if message:
            print("Received:", message)
            print("hello, world")  # 合図を受けたら出力
        pipe.close()

    except pywintypes.error as e:
        print("Error:", e)


def main():
    server_thread = threading.Thread(target=server, daemon=True)
    server_thread.start()
    print("Server start")

    try:
        while not stop_event.is_set():
            stop_event.wait(timeout=0.1)
    except KeyboardInterrupt:
        print("\nCtrl+C detected. Exiting gracefully")
        stop_event.set()
        sys.exit(0)
        # メインスレッドが終了するとデーモンスレッドも終了する
    

if __name__ == "__main__":
    main()