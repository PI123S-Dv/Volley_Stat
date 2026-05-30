import cv2
from ultralytics import YOLO

# ── 1. Load the YOLO model ───────────────────────────────
# First run: auto-downloads yolov8n.pt (~6MB)
# Every run after: loads from cache instantly
model = YOLO("yolov8n.pt")

# ── 2. Open the video ────────────────────────────────────
cap = cv2.VideoCapture("test_match.mp4")

if not cap.isOpened():
    print("Error: could not open video")
    exit()

fps    = cap.get(cv2.CAP_PROP_FPS)
width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
print(f"Video: {width}x{height} @ {fps}fps")

# ── 3. YOLO knows 80 object types — we only care about these two
# Full list at: https://docs.ultralytics.com/datasets/detect/coco
# Add near the top with your other constants
PERSON_CLASS      = 0
SPORTS_BALL_CLASS = 32
FRISBEE_CLASS     = 29   # often misclassified volleyballs
ORANGE_CLASS      = 49   # round, similar size
CLOCK_CLASS       = 74   # sometimes round objects get this

BALL_CLASSES = {SPORTS_BALL_CLASS, FRISBEE_CLASS, ORANGE_CLASS, CLOCK_CLASS}

# Only detect objects YOLO is this confident about (0.0 to 1.0)
CONFIDENCE_THRESHOLD = 0.7

frame_number = 0

while True:
    ret, frame = cap.read()
    if not ret:
        print("Video finished")
        break

    frame_number += 1

    # Process every 3rd frame — good balance of speed vs smoothness
    # (every frame is too slow on most computers)
    if frame_number % 3 != 0:
        continue

    timestamp = frame_number / fps

    # ── 4. Run YOLO on this frame ────────────────────────
    # verbose=False stops YOLO printing to terminal every frame
    results = model(frame, verbose=False)

    # results[0] = detections for the first (only) image we passed in
    detections = results[0].boxes

    # Add this right after: detections = results[0].boxes
    for box in detections:
        cid  = int(box.cls[0])
        conf = float(box.conf[0])
        name = model.names[cid]
        print(f"  Detected: {name} (class {cid}) — confidence {conf:.0%}")

    player_count = 0
    ball_found   = False
    ball_x       = 0
    ball_y       = 0

    # ── 5. Loop through every detection ──────────────────
    for box in detections:

        # Get the class ID (what type of object is this?)
        class_id   = int(box.cls[0])

        # Get confidence (how sure is YOLO?)
        confidence = float(box.conf[0])

        # Skip if not confident enough
        if class_id == PERSON_CLASS and confidence < 0.5:
            continue
        if class_id == SPORTS_BALL_CLASS and confidence < 0.15:
            continue
        if class_id != PERSON_CLASS and class_id != SPORTS_BALL_CLASS:
            continue

        # Get bounding box coordinates
        # xyxy = (x_top_left, y_top_left, x_bottom_right, y_bottom_right)
        x1, y1, x2, y2 = map(int, box.xyxy[0])

        # Centre point of the box
        cx = (x1 + x2) // 2
        cy = (y1 + y2) // 2

        # ── 6. Handle players ────────────────────────────
        if class_id == PERSON_CLASS:
            player_count += 1

            # Draw a green rectangle around the player
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

            # Label above the box
            label = f"Player {confidence:.0%}"
            cv2.putText(frame, label, (x1, y1 - 8),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # Draw a dot at the player's feet (bottom centre of box)
            feet_x = cx
            feet_y = y2
            cv2.circle(frame, (feet_x, feet_y), 4, (0, 255, 0), -1)

        # ── 7. Handle the ball ───────────────────────────
        elif class_id == BALL_CLASSES and confidence > 0.1:
            ball_found = True
            ball_x     = cx
            ball_y     = cy

            # Draw a yellow circle around the ball
            cv2.circle(frame, (cx, cy), 15, (0, 255, 255), 2)

            # Draw a filled dot at centre
            cv2.circle(frame, (cx, cy), 4, (0, 255, 255), -1)

            label = f"Ball {confidence:.0%}"
            cv2.putText(frame, label, (x1, y1 - 8),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 2)

    # ── 8. Draw info panel in top-left corner ────────────
    # Dark background rectangle so text is readable on any video
    cv2.rectangle(frame, (0, 0), (300, 80), (0, 0, 0), -1)

    cv2.putText(frame, f"Time : {timestamp:.1f}s",
                (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    cv2.putText(frame, f"Players detected: {player_count}",
                (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

    ball_text = f"Ball: ({ball_x}, {ball_y})" if ball_found else "Ball: not detected"
    ball_color = (0, 255, 255) if ball_found else (100, 100, 100)
    cv2.putText(frame, ball_text,
                (10, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.7, ball_color, 2)

    # ── 9. Show the frame ────────────────────────────────
    cv2.imshow("Volleyball Analysis - Step 2", frame)

    # Press q to quit, press space to pause
    key = cv2.waitKey(1)
    if key == ord('q'):
        print("Stopped by user")
        break
    elif key == ord(' '):
        cv2.waitKey(0)   # freeze until any key pressed

cap.release()
cv2.destroyAllWindows()
print(f"Done — processed {frame_number} frames")