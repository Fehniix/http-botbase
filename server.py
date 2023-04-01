import socket
import os
from time import sleep

HOST = "0.0.0.0"  # Standard loopback interface address (localhost)
PORT = 9002  # Port to listen on (non-privileged ports are > 1023)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print(f"Listening on port {PORT}.")
    try:
        s.bind((HOST, PORT))
    except:
        os.system("kill -9 $(lsof -i tcp:9002 | perl -ne 'm/^pyt.*?\\s+(\\d+)\\s/ && print \"$1\\n\"')")
        print("Killed existing process listening on default port and retrying.")
        sleep(1)
        s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print(f"Connected by {addr}")
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print(data.decode("utf-8").replace("\n", ""))