import cv2
from ultralytics import YOLO
import numpy as np
import json

# ─────────────────────────────────────────────────────────
#  COURT SELECTOR
# ─────────────────────────────────────────────────────────

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
            cv2.putText(param, "Press Q to confirm", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        cv2.imshow("Select Court - Click 4 Corners then Q", param)

def expand_court(pts, expand_px=40):
    """Push each corner outward by expand_px pixels so players on the
    edge of the court are not accidentally filtered out."""
    cx = sum(p[0] for p in pts) / 4
    cy = sum(p[1] for p in pts) / 4
    expanded = []
    for (x, y) in pts:
        dx = x - cx
        dy = y - cy
        length = (dx**2 + dy**2) ** 0.5
        if length == 0:
            expanded.append((x, y))
            continue
        x_new = int(x + (dx / length) * expand_px)
        y_new = int(y + (dy / length) * expand_px)
        expanded.append((x_new, y_new))
    return expanded

def select_court(video_path):
    cap = cv2.VideoCapture(video_path)
    ret, frame = cap.read()
    cap.release()

    if not ret:
        print("Could not read video frame")
        exit()

    print("\n╔══════════════════════════════════════╗")
    print("║         COURT SELECTION              ║")
    print("╚══════════════════════════════════════╝")
    print("Click the 4 corners of the court IN ORDER:")
    print("  1. Top-left corner")
    print("  2. Top-right corner")
    print("  3. Bottom-right corner")
    print("  4. Bottom-left corner")
    print("Click OUTSIDE the actual court lines (one player-width beyond)")
    print("Press Q when done\n")

    cv2.imshow("Select Court - Click 4 Corners then Q", frame)
    cv2.setMouseCallback("Select Court - Click 4 Corners then Q",
                         click_court, frame)

    while True:
        key = cv2.waitKey(1)
        if key == ord('q') and len(court_points) == 4:
            break
        if key == ord('q') and len(court_points) != 4:
            print(f"  Need 4 corners — you only clicked {len(court_points)}")

    cv2.destroyAllWindows()

    # Expand boundary outward so edge players are not missed
    expanded = expand_court(court_points, expand_px=40)
    print(f"Court corners  : {court_points}")
    print(f"Expanded by 40px: {expanded}\n")
    return expanded

def is_inside_court(cx, cy, court_pts):
    if len(court_pts) < 4:
        return True
    polygon = np.array(court_pts, dtype=np.int32)
    return cv2.pointPolygonTest(polygon, (float(cx), float(cy)), False) >= 0

# ─────────────────────────────────────────────────────────
#  ACTION DETECTOR
# ─────────────────────────────────────────────────────────

class ActionDetector:
    def __init__(self, court_pts, frame_width, frame_height):
        self.court_pts    = court_pts
        self.fw           = frame_width
        self.fh           = frame_height
        self.ball_history = []   # list of (x, y, timestamp)
        self.events       = []   # all confirmed events

        self.last_action_frame = -999
        self.cooldown_frames   = 45   # ~1.5 sec gap between events

    # ── Ball tracking helpers ─────────────────────────────

    def update_ball(self, bx, by, timestamp):
        self.ball_history.append((bx, by, timestamp))
        if len(self.ball_history) > 12:
            self.ball_history.pop(0)

    def ball_speed(self):
        """Pixel distance ball moved over last 3 frames"""
        if len(self.ball_history) < 3:
            return 0
        x1, y1, _ = self.ball_history[-3]
        x2, y2, _ = self.ball_history[-1]
        return ((x2-x1)**2 + (y2-y1)**2) ** 0.5

    def ball_was_low_then_high(self):
        """Ball jumped upward by 40+ pixels recently = hit upward"""
        if len(self.ball_history) < 6:
            return False
        older_y = self.ball_history[-6][1]
        newer_y = self.ball_history[-1][1]
        return (older_y - newer_y) > 40   # moved UP (y decreases upward)

    def ball_was_high_then_low(self):
        """Ball dropped downward by 40+ pixels recently = spike/attack"""
        if len(self.ball_history) < 6:
            return False
        older_y = self.ball_history[-6][1]
        newer_y = self.ball_history[-1][1]
        return (newer_y - older_y) > 40   # moved DOWN

    # ── Court geometry helpers ────────────────────────────

    def net_x(self):
        """Horizontal centre of the court = approximate net position"""
        if len(self.court_pts) < 4:
            return self.fw // 2
        xs = [p[0] for p in self.court_pts]
        return (min(xs) + max(xs)) // 2

    def ball_near_net(self, bx):
        return abs(bx - self.net_x()) < self.fw * 0.2

    def ball_near_backline(self, by):
        """Ball is in the bottom 25% of the court = near back line"""
        if len(self.court_pts) < 4:
            return by > self.fh * 0.70
        ys  = [p[1] for p in self.court_pts]
        return by > max(ys) * 0.75

    def ball_near_floor(self, by):
        """Ball is below the vertical midpoint of the court"""
        if len(self.court_pts) < 4:
            return by > self.fh * 0.65
        ys = [p[1] for p in self.court_pts]
        return by > (sum(ys) / len(ys))

    def players_near_net(self, players):
        net = self.net_x()
        return sum(1 for p in players if abs(p[0] - net) < self.fw * 0.25)

    # ── Main detection ────────────────────────────────────

    def detect(self, players, ball_pos, timestamp, frame_number):
        """
        players  — list of (cx, cy, x1, y1, x2, y2)
        ball_pos — (bx, by) or None
        Returns action string or None
        """
        # Still in cooldown from last event
        if frame_number - self.last_action_frame < self.cooldown_frames:
            return None

        if not ball_pos:
            return None

        bx, by = ball_pos
        self.update_ball(bx, by, timestamp)

        # Need enough history to make decisions
        if len(self.ball_history) < 6:
            return None

        speed      = self.ball_speed()
        went_up    = self.ball_was_low_then_high()
        went_down  = self.ball_was_high_then_low()
        near_net   = self.ball_near_net(bx)
        near_back  = self.ball_near_backline(by)
        near_floor = self.ball_near_floor(by)
        at_net     = self.players_near_net(players)

        action = None

        # ── SERVE ─────────────────────────────────────────
        # Ball jumps up from near the back line with speed
        if went_up and near_back and speed > 15:
            action = "Serve"

        # ── ATTACK / SPIKE ────────────────────────────────
        # Ball drops fast — anywhere on court
        elif went_down and speed > 20:
            action = "Attack"

        # ── BLOCK ─────────────────────────────────────────
        # Ball goes up near net with 2+ players at the net
        elif went_up and near_net and at_net >= 2:
            action = "Block"

        # ── DIG ───────────────────────────────────────────
        # Ball is low and bounces upward
        elif went_up and near_floor and speed > 10:
            action = "Dig"

        if action:
            self.last_action_frame = frame_number
            event = {
                "type":      action,
                "timestamp": round(timestamp, 2),
                "position":  {"x": bx, "y": by},
                "speed":     round(speed, 1)
            }
            self.events.append(event)
            return action

        return None

# ─────────────────────────────────────────────────────────
#  COLOURS PER ACTION
# ─────────────────────────────────────────────────────────

ACTION_COLORS = {
    "Serve":  (255, 100, 0  ),   # blue-ish
    "Attack": (0,   0,   255),   # red
    "Block":  (0,   140, 255),   # orange
    "Dig":    (255, 0,   255),   # purple
}

# ─────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────

VIDEO_PATH = "test_match.mp4"

# Step 1 — select court boundary
court_pts = select_court(VIDEO_PATH)

# Step 2 — load YOLO
print("Loading YOLO model...")
model = YOLO("yolov8n.pt")
print("Model ready\n")

PERSON_CLASS = 0
BALL_CLASSES = {32, 29}   # sports ball + frisbee (often misclassified)

cap    = cv2.VideoCapture(VIDEO_PATH)
fps    = cap.get(cv2.CAP_PROP_FPS)
width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
total  = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

print(f"Video: {width}x{height} @ {fps:.1f}fps  ({total} frames total)")
print("Controls: Q = quit  |  SPACE = pause/unpause\n")

detector = ActionDetector(court_pts, width, height)

frame_number         = 0
display_action       = None
display_action_until = 0   # frame number until which to show the action text

while True:
    ret, frame = cap.read()
    if not ret:
        print("Video finished")
        break

    frame_number += 1

    # Process every 6th frame — good balance of speed vs accuracy
    if frame_number % 6 != 0:
        continue

    timestamp = frame_number / fps

    # ── Draw court boundary ───────────────────────────────
    if len(court_pts) == 4:
        pts = np.array(court_pts, dtype=np.int32)
        cv2.polylines(frame, [pts], isClosed=True,
                      color=(255, 255, 0), thickness=2)

    # ── Draw net line ─────────────────────────────────────
    net_x = detector.net_x()
    cv2.line(frame, (net_x, 0), (net_x, height),
             (200, 200, 200), 1)

    # ── Run YOLO ──────────────────────────────────────────
    results    = model(frame, verbose=False)
    detections = results[0].boxes

    players  = []   # (cx, cy, x1, y1, x2, y2)
    ball_pos = None

    for box in detections:
        class_id   = int(box.cls[0])
        confidence = float(box.conf[0])

        # Confidence thresholds — lower = catches more but more false positives
        if class_id == PERSON_CLASS and confidence < 0.35:
            continue
        if class_id in BALL_CLASSES and confidence < 0.10:
            continue
        if class_id != PERSON_CLASS and class_id not in BALL_CLASSES:
            continue

        x1, y1, x2, y2 = map(int, box.xyxy[0])
        cx = (x1 + x2) // 2
        cy = (y1 + y2) // 2

        # ── Players: must be inside court ─────────────────
        if class_id == PERSON_CLASS:
            if not is_inside_court(cx, cy, court_pts):
                # Show filtered-out people as dark grey
                cv2.rectangle(frame, (x1, y1), (x2, y2), (60, 60, 60), 1)
                continue

            players.append((cx, cy, x1, y1, x2, y2))
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(frame, f"P {confidence:.0%}",
                        (x1, y1 - 6),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 0), 1)
            # Dot at feet
            cv2.circle(frame, (cx, y2), 3, (0, 255, 0), -1)

        # ── Ball: only filter by horizontal court range ────
        elif class_id in BALL_CLASSES:
            court_min_x = min(p[0] for p in court_pts) if court_pts else 0
            court_max_x = max(p[0] for p in court_pts) if court_pts else width
            court_max_y = max(p[1] for p in court_pts) if court_pts else height

            # Ball can fly above the court but not outside left/right edges
            if not (court_min_x < cx < court_max_x and cy < court_max_y):
                continue

            ball_pos = (cx, cy)
            cv2.circle(frame, (cx, cy), 15, (0, 255, 255), 2)
            cv2.circle(frame, (cx, cy), 4,  (0, 255, 255), -1)
            cv2.putText(frame, f"Ball {confidence:.0%}",
                        (x1, y1 - 6),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 255), 1)

    # ── Draw ball trail ───────────────────────────────────
    if len(detector.ball_history) > 1:
        for i in range(1, len(detector.ball_history)):
            alpha = i / len(detector.ball_history)   # fade older points
            bx1, by1, _ = detector.ball_history[i-1]
            bx2, by2, _ = detector.ball_history[i]
            color = (0, int(255 * alpha), int(255 * alpha))
            cv2.line(frame, (bx1, by1), (bx2, by2), color, 2)

    # ── Run action detection ──────────────────────────────
    action = detector.detect(players, ball_pos, timestamp, frame_number)

    if action:
        display_action       = action
        display_action_until = frame_number + 90   # show for ~3 seconds
        color = ACTION_COLORS.get(action, (255, 255, 255))
        print(f"  ⚡ {action:8s} | t={timestamp:.1f}s | "
              f"players on court={len(players)}")

    # ── Info panel (top left) ─────────────────────────────
    cv2.rectangle(frame, (0, 0), (330, 110), (0, 0, 0), -1)
    cv2.putText(frame, f"Time   : {timestamp:.1f}s",
                (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255,255,255), 2)
    cv2.putText(frame, f"Players: {len(players)}",
                (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0,255,0), 2)
    cv2.putText(frame, "Ball   : detected" if ball_pos else "Ball   : not seen",
                (10, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.65,
                (0,255,255) if ball_pos else (100,100,100), 2)
    cv2.putText(frame, f"Events : {len(detector.events)}",
                (10, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (200,200,200), 2)

    # ── Show last action in big text ──────────────────────
    if display_action and frame_number < display_action_until:
        color = ACTION_COLORS.get(display_action, (255,255,255))

        # Small text in info panel
        cv2.putText(frame, f"ACTION: {display_action}",
                    (10, 130), cv2.FONT_HERSHEY_SIMPLEX,
                    0.65, color, 2)

        # Big text centred on screen
        text_size = cv2.getTextSize(display_action,
                                    cv2.FONT_HERSHEY_SIMPLEX, 2.0, 4)[0]
        text_x = (width  - text_size[0]) // 2
        text_y = (height + text_size[1]) // 2

        # Dark shadow behind text so it's readable on any background
        cv2.putText(frame, display_action,
                    (text_x + 3, text_y + 3),
                    cv2.FONT_HERSHEY_SIMPLEX, 2.0, (0,0,0), 6)
        cv2.putText(frame, display_action,
                    (text_x, text_y),
                    cv2.FONT_HERSHEY_SIMPLEX, 2.0, color, 4)

    # ── Show frame ────────────────────────────────────────
    cv2.imshow("Volleyball Analysis - Step 4", frame)

    key = cv2.waitKey(1)
    if key == ord('q'):
        print("Stopped by user")
        break
    elif key == ord(' '):
        print("  [paused — press any key to continue]")
        cv2.waitKey(0)

# ─────────────────────────────────────────────────────────
#  SAVE RESULTS
# ─────────────────────────────────────────────────────────

cap.release()
cv2.destroyAllWindows()

with open("events.json", "w") as f:
    json.dump(detector.events, f, indent=2)

print(f"\n{'='*40}")
print(f"  Analysis complete")
print(f"  Total events detected : {len(detector.events)}")
print(f"  Saved to              : events.json")
print(f"{'='*40}")

if detector.events:
    print("\nEvent breakdown:")
    from collections import Counter
    counts = Counter(e["type"] for e in detector.events)
    for action, count in sorted(counts.items()):
        print(f"  {action:10s} : {count}")
    print("\nFirst 5 events:")
    for e in detector.events[:5]:
        print(f"  t={e['timestamp']:6.1f}s  {e['type']:10s}  "
              f"speed={e['speed']}")