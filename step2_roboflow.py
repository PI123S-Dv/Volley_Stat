import cv2
import json
import threading
from collections import Counter
from inference_sdk import InferenceHTTPClient

# ── Roboflow client ───────────────────────────────────────
CLIENT = InferenceHTTPClient(
    api_url="https://detect.roboflow.com",
    api_key="BAnKCFkYPrS0Le1z4q5Q"
)
MODEL_ID = "volleyball-actions-cptry/11"

# ── Colours per class ─────────────────────────────────────
CLASS_COLORS = {
    "Player":  (0,   255, 0  ),
    "ball":    (0,   255, 255),
    "serve":   (255, 100, 0  ),
    "Serve":   (255, 100, 0  ),
    "attack":  (0,   0,   255),
    "Attack":  (0,   0,   255),
    "spike":   (0,   0,   255),
    "block":   (0,   140, 255),
    "Block":   (0,   140, 255),
    "dig":     (255, 0,   255),
    "Dig":     (255, 0,   255),
}
DEFAULT_COLOR = (255, 255, 255)

# ── Shared state between main thread and API thread ───────
latest_predictions = []   # most recent detections from API
predictions_lock   = threading.Lock()
all_events         = []   # saved for JSON export

def call_api(frame):
    """Runs in background thread — sends frame to Roboflow, stores result"""
    global latest_predictions
    try:
        small = cv2.resize(frame, (416, 416))
        cv2.imwrite("temp_frame.jpg", small,
                    [cv2.IMWRITE_JPEG_QUALITY, 70])
        result = CLIENT.infer("temp_frame.jpg", model_id=MODEL_ID)
        preds  = result.get("predictions", [])
        with predictions_lock:
            latest_predictions = preds
    except Exception as e:
        print(f"API error: {e}")

# ── Open video ────────────────────────────────────────────
cap    = cv2.VideoCapture("test_match.mp4")
fps    = cap.get(cv2.CAP_PROP_FPS)
width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
print(f"Video: {width}x{height} @ {fps:.1f}fps")
print("Controls: Q = quit  |  SPACE = pause\n")

frame_number         = 0
api_thread           = None
display_action       = None
display_action_until = 0
ball_history         = []

while True:
    ret, frame = cap.read()
    if not ret:
        print("Video finished")
        break

    frame_number += 1
    timestamp = frame_number / fps

    # ── Send frame to API every 30 frames ─────────────────
    # Runs in background so video keeps playing
    if frame_number % 30 == 0:
        if api_thread is None or not api_thread.is_alive():
            api_thread = threading.Thread(
                target=call_api,
                args=(frame.copy(),),
                daemon=True
            )
            api_thread.start()

    # ── Draw latest predictions on every frame ────────────
    # (These are from the last API response — updated in background)
    with predictions_lock:
        current_preds = list(latest_predictions)

    player_count = 0
    ball_pos     = None

    for pred in current_preds:
        class_name = pred["class"]
        confidence = pred["confidence"]
        color      = CLASS_COLORS.get(class_name, DEFAULT_COLOR)

        # Scale coordinates back to original frame size
        # (we sent a 416x416 image but display is original size)
        scale_x = width  / 416
        scale_y = height / 416

        cx = int(pred["x"] * scale_x)
        cy = int(pred["y"] * scale_y)
        w  = int(pred["width"]  * scale_x)
        h  = int(pred["height"] * scale_y)

        x1 = cx - w // 2
        y1 = cy - h // 2
        x2 = cx + w // 2
        y2 = cy + h // 2

        # ── Player ────────────────────────────────────────
        if class_name in ("Player", "player", "person"):
            player_count += 1
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
            cv2.putText(frame, f"P {confidence:.0%}",
                        (x1, y1 - 6),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 1)

        # ── Ball ──────────────────────────────────────────
        elif class_name in ("ball", "Ball", "volleyball"):
            ball_pos = (cx, cy)
            cv2.circle(frame, (cx, cy), 15, color, 2)
            cv2.circle(frame, (cx, cy), 4,  color, -1)
            cv2.putText(frame, f"Ball {confidence:.0%}",
                        (x1, y1 - 6),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 1)

        # ── Action ────────────────────────────────────────
        else:
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 3)
            label = f"{class_name} {confidence:.0%}"
            cv2.putText(frame, label, (x1, y1 - 8),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

            # Only save if this is a new event (not the same action repeating)
            if (not all_events or
                all_events[-1]["type"] != class_name or
                timestamp - all_events[-1]["timestamp"] > 1.5):

                all_events.append({
                    "type":       class_name,
                    "timestamp":  round(timestamp, 2),
                    "confidence": round(confidence, 2),
                    "position":   {"x": cx, "y": cy}
                })
                display_action       = class_name
                display_action_until = frame_number + 60
                print(f"  ⚡ {class_name:10s} | t={timestamp:.1f}s "
                      f"| conf={confidence:.0%}")

    # ── Ball trail ────────────────────────────────────────
    if ball_pos:
        ball_history.append(ball_pos)
        if len(ball_history) > 10:
            ball_history.pop(0)

    for i in range(1, len(ball_history)):
        alpha = i / len(ball_history)
        c = (0, int(200 * alpha), int(255 * alpha))
        cv2.line(frame, ball_history[i-1], ball_history[i], c, 2)

    # ── Big action text ───────────────────────────────────
    if display_action and frame_number < display_action_until:
        color = CLASS_COLORS.get(display_action, (255,255,255))
        text  = display_action.upper()
        sz    = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, 2.0, 4)[0]
        tx    = (width  - sz[0]) // 2
        ty    = (height + sz[1]) // 2
        cv2.putText(frame, text, (tx+3, ty+3),
                    cv2.FONT_HERSHEY_SIMPLEX, 2.0, (0,0,0), 6)
        cv2.putText(frame, text, (tx, ty),
                    cv2.FONT_HERSHEY_SIMPLEX, 2.0, color, 4)

    # ── Info panel ────────────────────────────────────────
    cv2.rectangle(frame, (0, 0), (330, 110), (0, 0, 0), -1)
    cv2.putText(frame, f"Time   : {timestamp:.1f}s",
                (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255,255,255), 2)
    cv2.putText(frame, f"Players: {player_count}",
                (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0,255,0), 2)
    cv2.putText(frame, "Ball: detected" if ball_pos else "Ball: not seen",
                (10, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.65,
                (0,255,255) if ball_pos else (100,100,100), 2)
    cv2.putText(frame, f"Events : {len(all_events)}",
                (10, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (200,200,200), 2)

    cv2.imshow("Volleyball Analysis", frame)

    key = cv2.waitKey(1)
    if key == ord('q'):
        print("Stopped by user")
        break
    elif key == ord(' '):
        cv2.waitKey(0)

# ── Save results ──────────────────────────────────────────
cap.release()
cv2.destroyAllWindows()

with open("events.json", "w") as f:
    json.dump(all_events, f, indent=2)

print(f"\n{'='*40}")
print(f"  Total events : {len(all_events)}")
print(f"  Saved to     : events.json")
print(f"{'='*40}")

if all_events:
    counts = Counter(e["type"] for e in all_events)
    print("\nBreakdown:")
    for action, count in sorted(counts.items()):
        print(f"  {action:12s}: {count}")