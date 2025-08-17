from multiprocessing import shared_memory
import time

shm = shared_memory.SharedMemory(name="MySharedMemory")

while True:
    msg = bytes(shm.buf[:1024]).split(b'\x00', 1)[0]
    print("Python read:", msg.decode())
    time.sleep(1)

shm.close()
