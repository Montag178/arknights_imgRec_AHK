# server.py
import os
import win32pipe, win32file, pywintypes

PIPE_NAME = r'\\.\pipe\MyPipe'

print("Python server running. Waiting for AHK...")

while True:
    try:
        # パイプを作って待ち受け
        pipe = win32pipe.CreateNamedPipe(
            PIPE_NAME,
            win32pipe.PIPE_ACCESS_DUPLEX,
            win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT,
            1, 65536, 65536,
            0,
            None
        )

        win32pipe.ConnectNamedPipe(pipe, None)  # AHKから接続が来るまでブロック

        # メッセージを受信
        hr, data = win32file.ReadFile(pipe, 64*1024)
        message = data.decode("utf-8").strip()
        if message:
            print("Received:", message)
            print("hello, world")  # 合図を受けたら出力

        win32file.CloseHandle(pipe)

    except pywintypes.error as e:
        print("Error:", e)
