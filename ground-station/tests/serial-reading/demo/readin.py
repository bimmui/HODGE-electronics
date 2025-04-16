import serial
import threading


def read_from_port(port):
    while True:
        try:
            line = port.readline().decode("utf-8").strip()
            if line:
                print(f"{port.port}: {line}")
        except Exception as e:
            print(f"Error reading from {port.port}: {e}")
            break


esp1 = serial.Serial("COM3", 115200, timeout=1)
esp2 = serial.Serial("COM5", 115200, timeout=1)

# do i need to worry about using threading and multiprocessing in one big program
# could just have a single process do multithreading and just pool together data somehow
# also need to make them daemon threads
thread1 = threading.Thread(target=read_from_port, args=(esp1,))
thread2 = threading.Thread(target=read_from_port, args=(esp2,))

thread1.start()
thread2.start()

# hmm
thread1.join()
thread2.join()
