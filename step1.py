import cv2

# ── 1. Open the video file ──────────────────────
cap = cv2.VideoCapture("test_match.mp4")

# Check it opened successfully
if not cap.isOpened():
    print("Error: could not open video file")
    exit()

# ── 2. Read basic info about the video ──────────
total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
fps          = cap.get(cv2.CAP_PROP_FPS)
width        = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height       = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
duration_sec = total_frames / fps

print(f"Video loaded successfully!")
print(f"Resolution : {width} x {height}")
print(f"FPS        : {fps}")
print(f"Frames     : {total_frames}")
print(f"Duration   : {duration_sec:.1f} seconds")

# ── 3. Read and display frames one by one ───────
frame_number = 0

while True:
    ret, frame = cap.read()
    # ret = True if a frame was read successfully
    # frame = the actual image data (a grid of pixels)

    if not ret:
        # No more frames = end of video
        print("Video finished")
        break

    frame_number += 1

    # Only process every 30th frame (every 1 second at 30fps)
    # Without this the window updates too fast to see anything useful
    if frame_number % 30 != 0:
        continue

    # ── 4. Draw info text onto the frame ────────
    timestamp = frame_number / fps
    text = f"Frame: {frame_number}  |  Time: {timestamp:.1f}s"

    cv2.putText(
        frame,          # image to draw on
        text,           # text to write
        (20, 40),       # position (x, y) from top-left
        cv2.FONT_HERSHEY_SIMPLEX,  # font style
        1.0,            # font size
        (0, 255, 0),    # colour (Blue, Green, Red) - this is green
        2               # thickness
    )

    # ── 5. Show the frame in a window ───────────
    cv2.imshow("Volleyball Analysis - Step 1", frame)

    # Wait 30ms between frames. If user presses 'q', quit early
    key = cv2.waitKey(30)
    if key == ord('q'):
        print("Stopped early by user")
        break

# ── 6. Always release resources when done ───────
cap.release()           # close the video file
cv2.destroyAllWindows() # close the display window

print(f"Processed {frame_number} frames total")