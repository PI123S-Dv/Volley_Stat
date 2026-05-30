import cv2
from ultralytics import YOLO
import numpy as np
import json

# ── Court selector ───────────────────────────────────────
court_points = []

def click_court(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        court_points.append((x, y))
        print(f"  Corner {len(court_points)}: ({x}, {y})")
        cv2.circle(param, (x, y), 6, (0, 0, 255), -1)
        if len(court_points) > 1:
            cv2.line(param, court_points[-2], court_points[-1], (0, 0, 255), 2)
        if len(court_points) == 4:
            cv2.line(param, court_points[-1], court_points[0], (0, 0, 255), 2)
        cv2.imshow("Select Court - Click 4 Corners then press Q", param)

def select_court(video_path):
    cap = cv2.VideoCapture(video_path)
    ret, frame = cap.read()
    cap.release()

    print("\nClick the 4 corners of the court:")
    print("  1. Top-left corner")
    print("  2. Top-right corner")
    print("  3. Bottom-right corner")
    print("  4. Bottom-left corner")
    print("Then press Q\n")

    cv2.imshow("Select Court - Click 4 Corners then press Q", frame)
    cv2.setMouseCallback("Select Court - Click 4 Corners then press Q",
                         click_court, frame)

    while True:
        key = cv2.waitKey(1)
        if key == ord('q') or len(court_points) == 4:
            break

    cv2.destroyAllWindows()
    print(f"Court defined: {court_points}\n")
    return court_points

def is_inside_court(cx, cy, court_pts):
    if len(court_pts) < 4:
        return True
    polygon = np.array(court_pts, dtype=np.int32)
    result  = cv2.pointPolygonTest(polygon, (float(cx), float(cy)), False)
    return result >= 0

# ── Main ─────────────────────────────────────────────────
VIDEO_PATH = "test_match.mp4"

# Step 1: select court
court_pts = select_court(VIDEO_PATH)

# Step 2: load model
model = YOLO("yolov8n.pt")

PERSON_CLASS = 0
BALL_CLASSES = {32, 29}

cap    = cv2.VideoCapture(VIDEO_PATH)
fps    = cap.get(cv2.CAP_PROP_FPS)
width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

frame_number = 0
all_events   = []

while True:
    ret, frame = cap.read()
    if not ret:
        break

    frame_number += 1
    if frame_number % 6 != 0:
        continue

    timestamp = frame_number / fps

    # Draw court boundary on every frame
    if len(court_pts) == 4:
        pts = np.array(court_pts, dtype=np.int32)
        cv2.polylines(frame, [pts], isClosed=True, color=(255, 255, 0), thickness=2)

    # Run YOLO
    results    = model(frame, verbose=False)
    detections = results[0].boxes

    player_count = 0
    ball_found   = False

    for box in detections:
        class_id   = int(box.cls[0])
        confidence = float(box.conf[0])

        if class_id == PERSON_CLASS and confidence < 0.5:
            continue
        if class_id in BALL_CLASSES and confidence < 0.15:
            continue
        if class_id != PERSON_CLASS and class_id not in BALL_CLASSES:
            continue

        x1, y1, x2, y2 = map(int, box.xyxy[0])
        cx = (x1 + x2) // 2
        cy = (y1 + y2) // 2

        # Use feet position for players, centre for ball
        check_x = cx
        check_y = cy

        # ── SKIP if outside court ────────────────────────
        if not is_inside_court(check_x, check_y, court_pts):
            # Draw a grey box so you can see what got filtered
            cv2.rectangle(frame, (x1, y1), (x2, y2), (80, 80, 80), 1)
            continue

        if class_id == PERSON_CLASS:
            player_count += 1
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(frame, f"Player {confidence:.0%}",
                        (x1, y1 - 8), cv2.FONT_HERSHEY_SIMPLEX,
                        0.5, (0, 255, 0), 2)
            # Dot at feet
            cv2.circle(frame, (cx, y2), 4, (0, 255, 0), -1)

            all_events.append({
                "type":      "player_position",
                "timestamp": round(timestamp, 2),
                "position":  {"x": cx, "y": cy}
            })

        elif class_id in BALL_CLASSES:
            ball_found = True
            cv2.circle(frame, (cx, cy), 15, (0, 255, 255), 2)
            cv2.circle(frame, (cx, cy), 4,  (0, 255, 255), -1)
            cv2.putText(frame, f"Ball {confidence:.0%}",
                        (x1, y1 - 8), cv2.FONT_HERSHEY_SIMPLEX,
                        0.5, (0, 255, 255), 2)

            all_events.append({
                "type":      "ball_position",
                "timestamp": round(timestamp, 2),
                "position":  {"x": cx, "y": cy}
            })

    # Info panel
    cv2.rectangle(frame, (0, 0), (300, 80), (0, 0, 0), -1)
    cv2.putText(frame, f"Time: {timestamp:.1f}s",
                (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    cv2.putText(frame, f"Players on court: {player_count}",
                (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    cv2.putText(frame, "Ball: YES" if ball_found else "Ball: NO",
                (10, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.7,
                (0, 255, 255) if ball_found else (100, 100, 100), 2)

    cv2.imshow("Volleyball Analysis - Step 3", frame)

    key = cv2.waitKey(1)
    if key == ord('q'):
        break
    elif key == ord(' '):
        cv2.waitKey(0)

# Save positions to JSON
with open("events.json", "w") as f:
    json.dump(all_events, f, indent=2)

print(f"Saved {len(all_events)} detections to events.json")

cap.release()
cv2.destroyAllWindows()