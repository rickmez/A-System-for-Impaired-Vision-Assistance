from flask import Flask, request
import cv2
import numpy as np
import queue
import threading

app = Flask(__name__)

# Queues for threading
raw_queue = queue.Queue()
decoded_queue = queue.Queue()

done = False

# ---------------- THREADS ---------------- #
def decode_images():
    """Thread that decodes JPEG bytes into OpenCV images."""
    while not done:
        try:
            data = raw_queue.get(timeout=1)
            np_arr = np.frombuffer(data, np.uint8)
            img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
            if img is not None:
                decoded_queue.put(img)
            else:
                print("Warning: Could not decode image")
        except queue.Empty:
            continue

def display_images():
    """Thread that displays decoded images."""
    while not done:
        try:
            img = decoded_queue.get(timeout=1)
            cv2.imshow("Received Image", img)
            cv2.waitKey(1)
        except queue.Empty:
            continue

# ---------------- FLASK ROUTE ---------------- #
@app.route('/data', methods=['POST'])
def receive_data():
    """Receive raw JPEG bytes and push to raw queue."""
    data = request.get_data()
    # print(f"Received RGB {len(data)} bytes")
    if len(data) > 0:
        raw_queue.put(data)
    return "OK", 200

@app.route('/depth', methods=['POST'])
def receive_depth():
    data = request.get_data()
    depth_array = np.frombuffer(data, dtype=np.uint16).reshape((240, 320))
    # print("Depth shape:", depth_array.shape)
    # print("Min depth:", depth_array.min(), "Max depth:", depth_array.max())
    return "OK", 200

@app.route('/imu', methods=['POST'])
def receive_imu():
    data = request.get_data()
    print(data)
    # depth_array = np.frombuffer(data, dtype=np.uint16).reshape((240, 320))
    # print("Depth shape:", depth_array.shape)
    # print("Min depth:", depth_array.min(), "Max depth:", depth_array.max())
    return "OK", 200

# ---------------- MAIN ---------------- #
if __name__ == '__main__':
    # Start decoder thread
    threading.Thread(target=decode_images, daemon=True).start()

    # Start display thread
    threading.Thread(target=display_images, daemon=True).start()

    try:
        # Flask runs as the receiver thread
        app.run(host='0.0.0.0', port=8080, threaded=True)
    finally:
        done = True


# from flask import Flask, request

# app = Flask(__name__)

# @app.route('/data', methods=['POST'])
# def receive_data():
#     data = request.get_data()  # raw bytes
#     print(f"Received {len(data)} bytes")
#     return "hello from PC", 200

# if __name__ == '__main__':
#     app.run(host='0.0.0.0', port=8080)
