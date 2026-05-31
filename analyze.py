import cv2
import json
from collections import Counter
from inference_sdk import InferenceHTTPClient

CLIENT = InferenceHTTPClient(
    api_url="https://detect.roboflow.com",
    api_key="PASTE_YOUR_API_KEY_HERE"
)
MODEL_ID = "volleyball-actions-cptry/11"

CLASS_COLORS = {
    "Player":  (0,   255, 0  ),
    "ball":    (0,   255, 255),
    "serve":   (255, 100, 0  ),
    "Serve":   (255, 100, 0  ),
    "attack":  (0,   0,   255),
    "Attack":  (0,   0,   255),
    "spike":   (0,   0,   255),
    "Spike":   (0,   0,   255),
    "block":   (0,   140, 255),
    "Block":   (0,   140, 255),
    "dig":     (255, 0,   255),
    "Dig":     (255, 0,   255),
    "set":     (255, 255, 0  ),
    "Set":     (255, 255, 0  ),
}

# ── Rally states ──────────────────────────────────────────
WAITING  = "WAITING"
ACTIVE   = "ACTIVE"
COOLDOWN = "COOLDOWN"

RALLY_START_ACTIONS = {"serve", "Serve"}
RALLY_END_ACTIONS   = {"attack", "Attack", "spike", "Spike", "dig", "Dig"}
# Set/Block are mid-rally actions — they don't end the rally

# How long the same action must wait before being saved again
DUPLICATE_WINDOW_SECONDS = 2.0

# How long after a rally-ending action before we confirm point is over
COOLDOWN_SECONDS = 2.5

# ─────────────────────────────────────────────────────────
#  RALLY TRACKER
# ─────────────────────────────────────────────────────────

class RallyTracker:
    def __init__(self):
        self.state          = WAITING
        self.current_rally  = []
        self.all_rallies    = []
        self.cooldown_until = 0.0
        self.rally_number   = 0
        self.ball_positions = []

    # ── Duplicate filter ──────────────────────────────────
    def is_duplicate(self, event_type, timestamp):
        """
        Returns True if this exact action type was already saved
        within the last DUPLICATE_WINDOW_SECONDS.
        Checks all recent events, not just the last one —
        so Block/Set alternating can't bypass the filter.
        """
        for past_event in reversed(self.current_rally):
            if past_event["type"] == event_type:
                if timestamp - past_event["timestamp"] < DUPLICATE_WINDOW_SECONDS:
                    return True   # same action seen recently — skip it
                else:
                    break         # same type but long ago — allow it
        return False

    # ── Ball-based serve detection ────────────────────────
    def check_ball_serve(self, ball_pos, timestamp):
        """
        Detects a serve from ball movement alone.
        Catches far-side serves the model misses because
        the player is small and arm motion is not visible.
        Only triggers when state = WAITING.
        """
        if self.state != WAITING:
            self.ball_positions = []
            return False

        self.ball_positions.append((ball_pos[0], ball_pos[1], timestamp))
        if len(self.ball_positions) > 20:
            self.ball_positions.pop(0)

        if len(self.ball_positions) < 10:
            return False

        older     = self.ball_positions[-10]
        newer     = self.ball_positions[-1]
        time_diff = newer[2] - older[2]

        if time_diff <= 0:
            return False

        dist  = ((newer[0]-older[0])**2 + (newer[1]-older[1])**2) ** 0.5
        speed = dist / time_diff

        if speed > 80:
            print(f"\n  🏐 Ball-based serve at t={timestamp:.1f}s "
                  f"(speed={speed:.0f}px/s) — likely far-side serve")
            self.ball_positions = []
            return True

        return False

    # ── Main event processor ──────────────────────────────
    def process_event(self, event_type, timestamp, position, confidence):
        """
        Feed each detected action into the tracker.
        Returns current state string.
        """

        # ── WAITING ───────────────────────────────────────
        if self.state == WAITING:
            if event_type in RALLY_START_ACTIONS:
                self.state         = ACTIVE
                self.rally_number += 1
                self.current_rally = [{
                    "type":       event_type,
                    "timestamp":  timestamp,
                    "position":   position,
                    "confidence": confidence
                }]
                print(f"\n  🏐 Rally #{self.rally_number} STARTED "
                      f"at t={timestamp:.1f}s")
            return self.state

        # ── ACTIVE ────────────────────────────────────────
        elif self.state == ACTIVE:

            if not self.is_duplicate(event_type, timestamp):
                self.current_rally.append({
                    "type":       event_type,
                    "timestamp":  timestamp,
                    "position":   position,
                    "confidence": confidence
                })
                print(f"  ⚡ [ACTIVE] Rally #{self.rally_number} "
                      f"| {event_type:10s} | t={timestamp:.1f}s "
                      f"| conf={confidence:.0%}")

            # Rally-ending action → enter cooldown
            if event_type in RALLY_END_ACTIONS:
                self.state          = COOLDOWN
                self.cooldown_until = timestamp + COOLDOWN_SECONDS
                print(f"  ⏳ Cooldown — waiting {COOLDOWN_SECONDS}s "
                      f"to confirm point is over")

            # New serve while active = old rally ended, new one starts
            elif event_type in RALLY_START_ACTIONS and len(self.current_rally) > 1:
                self._end_rally(timestamp)
                self.state         = ACTIVE
                self.rally_number += 1
                self.current_rally = [{
                    "type":       event_type,
                    "timestamp":  timestamp,
                    "position":   position,
                    "confidence": confidence
                }]
                print(f"\n  🏐 Rally #{self.rally_number} STARTED "
                      f"at t={timestamp:.1f}s")

            return self.state

        # ── COOLDOWN ──────────────────────────────────────
        elif self.state == COOLDOWN:

            # New serve = previous rally confirmed over, new one starts
            if event_type in RALLY_START_ACTIONS:
                self._end_rally(timestamp)
                self.state         = ACTIVE
                self.rally_number += 1
                self.current_rally = [{
                    "type":       event_type,
                    "timestamp":  timestamp,
                    "position":   position,
                    "confidence": confidence
                }]
                print(f"\n  🏐 Rally #{self.rally_number} STARTED "
                      f"at t={timestamp:.1f}s")

            # Another end-action in cooldown → add if not duplicate, reset timer
            elif event_type in RALLY_END_ACTIONS:
                if not self.is_duplicate(event_type, timestamp):
                    self.current_rally.append({
                        "type":       event_type,
                        "timestamp":  timestamp,
                        "position":   position,
                        "confidence": confidence
                    })
                self.cooldown_until = timestamp + COOLDOWN_SECONDS

            return self.state

        return self.state

    def tick(self, timestamp):
        """Call every frame — handles cooldown expiry."""
        if self.state == COOLDOWN and timestamp >= self.cooldown_until:
            self._end_rally(timestamp)
            self.state = WAITING
            print(f"  🔴 Point over — waiting for next serve")

    def _end_rally(self, timestamp):
        if self.current_rally:
            duration = round(timestamp - self.current_rally[0]["timestamp"], 2)
            self.all_rallies.append({
                "rally_number": self.rally_number,
                "start_time":   self.current_rally[0]["timestamp"],
                "end_time":     timestamp,
                "duration":     duration,
                "events":       list(self.current_rally),
                "event_count":  len(self.current_rally)
            })
            print(f"  ✅ Rally #{self.rally_number} ENDED — "
                  f"{len(self.current_rally)} events, {duration}s")
            self.current_rally = []

    def finish(self, timestamp):
        """Save any unfinished rally at end of video."""
        if self.current_rally:
            self._end_rally(timestamp)

    def get_all_events(self):
        """Flat list of all events across all rallies for C++ JSON."""
        events = []
        for rally in self.all_rallies:
            for e in rally["events"]:
                event = dict(e)
                event["rally_number"] = rally["rally_number"]
                events.append(event)
        return events


# ─────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────

VIDEO_PATH  = "test_match.mp4"
OUTPUT_PATH = "annotated_match.mp4"

cap    = cv2.VideoCapture(VIDEO_PATH)
fps    = cap.get(cv2.CAP_PROP_FPS)
width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
total  = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

fourcc = cv2.VideoWriter_fourcc(*"mp4v")
out    = cv2.VideoWriter(OUTPUT_PATH, fourcc, fps, (width, height))

print(f"╔══════════════════════════════════════╗")
print(f"║   Volleyball Analysis — analyze.py   ║")
print(f"╚══════════════════════════════════════╝")
print(f"Video  : {width}x{height} @ {fps:.1f}fps  ({total} frames)")
print(f"Output : {OUTPUT_PATH}")
print(f"\nProcessing every 15th frame — grab a coffee ☕\n")
print(f"{'─'*50}")

tracker          = RallyTracker()
last_predictions = []
ball_history     = []
frame_number     = 0

while True:
    ret, frame = cap.read()
    if not ret:
        break

    frame_number += 1
    timestamp = frame_number / fps

    # Tick tracker every frame — handles cooldown expiry
    tracker.tick(timestamp)

    # Call API every 15 frames
    if frame_number % 15 == 0:
        try:
            cv2.imwrite("temp_frame.jpg", frame)
            result           = CLIENT.infer("temp_frame.jpg", model_id=MODEL_ID)
            last_predictions = result.get("predictions", [])
        except Exception as e:
            print(f"\n  API error at frame {frame_number}: {e}")

        pct = (frame_number / total) * 100
        print(f"  {pct:5.1f}%  |  t={timestamp:7.1f}s  |  "
              f"state={tracker.state:8s}  |  "
              f"rallies={len(tracker.all_rallies)}", end="\r")

    # Dim frame when not in an active rally
    if tracker.state != ACTIVE:
        overlay = frame.copy()
        cv2.rectangle(overlay, (0,0), (width,height), (0,0,0), -1)
        cv2.addWeighted(overlay, 0.3, frame, 0.7, 0, frame)

    player_count = 0
    ball_pos     = None

    for pred in last_predictions:
        class_name = pred["class"]
        confidence = pred["confidence"]
        color      = CLASS_COLORS.get(class_name, (255,255,255))

        cx = int(pred["x"])
        cy = int(pred["y"])
        w  = int(pred["width"])
        h  = int(pred["height"])
        x1, y1 = cx - w//2, cy - h//2
        x2, y2 = cx + w//2, cy + h//2

        # ── Player ────────────────────────────────────────
        if class_name in ("Player", "player", "person"):
            player_count += 1
            cv2.rectangle(frame, (x1,y1), (x2,y2), color, 2)
            cv2.putText(frame, f"P {confidence:.0%}",
                        (x1, y1-6),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 1)

        # ── Ball ──────────────────────────────────────────
        elif class_name in ("ball", "Ball", "volleyball"):
            ball_pos = (cx, cy)
            cv2.circle(frame, (cx,cy), 15, color, 2)
            cv2.circle(frame, (cx,cy), 4,  color, -1)
            cv2.putText(frame, f"Ball {confidence:.0%}",
                        (x1, y1-6),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 1)

            # Ball-based serve detection — catches far-side serves
            if tracker.check_ball_serve(ball_pos, timestamp):
                tracker.process_event(
                    "Serve", timestamp,
                    {"x": cx, "y": cy},
                    0.70
                )

        # ── Action ────────────────────────────────────────
        else:
            if tracker.state in (ACTIVE, COOLDOWN):
                cv2.rectangle(frame, (x1,y1), (x2,y2), color, 3)
                cv2.putText(frame, f"{class_name} {confidence:.0%}",
                            (x1, y1-8),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
                tracker.process_event(
                    class_name, timestamp,
                    {"x": cx, "y": cy},
                    round(confidence, 2)
                )

            # WAITING — only accept serves with lower confidence threshold
            elif tracker.state == WAITING:
                if class_name in RALLY_START_ACTIONS and confidence > 0.25:
                    cv2.rectangle(frame, (x1,y1), (x2,y2), color, 3)
                    cv2.putText(frame, f"{class_name} {confidence:.0%}",
                                (x1, y1-8),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
                    tracker.process_event(
                        class_name, timestamp,
                        {"x": cx, "y": cy},
                        round(confidence, 2)
                    )

    # Ball trail — only during active rally
    if ball_pos and tracker.state == ACTIVE:
        ball_history.append(ball_pos)
        if len(ball_history) > 10:
            ball_history.pop(0)
        for i in range(1, len(ball_history)):
            alpha = i / len(ball_history)
            c = (0, int(200*alpha), int(255*alpha))
            cv2.line(frame, ball_history[i-1], ball_history[i], c, 2)
    elif tracker.state == WAITING:
        ball_history.clear()

    # Info panel
    state_colors = {
        ACTIVE:   (0,   255, 0  ),
        COOLDOWN: (0,   140, 255),
        WAITING:  (100, 100, 100),
    }
    state_color = state_colors.get(tracker.state, (255,255,255))

    cv2.rectangle(frame, (0,0), (360,130), (0,0,0), -1)
    cv2.putText(frame, f"Time    : {timestamp:.1f}s",
                (10,25), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255,255,255), 2)
    cv2.putText(frame, f"State   : {tracker.state}",
                (10,50), cv2.FONT_HERSHEY_SIMPLEX, 0.65, state_color, 2)
    cv2.putText(frame, f"Rally   : #{tracker.rally_number}",
                (10,75), cv2.FONT_HERSHEY_SIMPLEX, 0.65, state_color, 2)
    cv2.putText(frame, f"Players : {player_count}",
                (10,100), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0,255,0), 2)
    cv2.putText(frame, f"Finished: {len(tracker.all_rallies)} rallies",
                (10,125), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (200,200,200), 2)

    out.write(frame)

# ─────────────────────────────────────────────────────────
#  WRAP UP
# ─────────────────────────────────────────────────────────

tracker.finish(frame_number / fps)
cap.release()
out.release()

flat_events = tracker.get_all_events()

with open("events.json", "w") as f:
    json.dump(flat_events, f, indent=2)

with open("rallies.json", "w") as f:
    json.dump(tracker.all_rallies, f, indent=2)

print(f"\n\n{'='*50}")
print(f"  Done!")
print(f"  Annotated video  : {OUTPUT_PATH}")
print(f"  Events (for C++) : events.json  ({len(flat_events)} events)")
print(f"  Rally breakdown  : rallies.json ({len(tracker.all_rallies)} rallies)")
print(f"{'='*50}")

if tracker.all_rallies:
    print(f"\nRally summary:")
    for r in tracker.all_rallies:
        actions = " → ".join(e["type"] for e in r["events"])
        print(f"  Rally #{r['rally_number']:2d} | "
              f"{r['duration']:5.1f}s | {actions}")

if flat_events:
    print(f"\nEvent breakdown:")
    counts = Counter(e["type"] for e in flat_events)
    for action, count in sorted(counts.items()):
        print(f"  {action:12s}: {count}")