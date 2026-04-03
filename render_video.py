import json

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont

# --- CONFIGURATION ---
FPS = 20
WIDTH, HEIGHT = 1920, 1080

# Bar dimensions
BAR_WIDTH = 2
BAR_HEIGHT_MAX = 80
BAR_HEIGHT_MIN = 3
MAX_VALUE = 992  # Actual max in data

# Layout
LEFT_COLUMN_X = 80
RIGHT_COLUMN_X = 960  # 80 + 820 + gap(40? no, let's do 200 gap from left col end)
# Actually: LEFT_COLUMN_X to right column:
# Left column: 80 to 80+820 = 900
# Gap: 200
# Right column: 1100 to 1920-80 = 1840
# So RIGHT_COLUMN_X = 80 + 820 + 200 = 1100
RIGHT_COLUMN_X = 1100

START_Y = 150
BG_COLOR = (30, 30, 30)
BAR_COLOR = (150, 150, 150)
LEFT_COLOR = (50, 150, 255)  # Blue
RIGHT_COLOR = (255, 100, 100)  # Red
TEXT_COLOR = (0, 0, 0)

# Tier spacing
TIER_GAP = 5
# Pairing uses BAR_HEIGHT_MAX + TIER_GAP per tier
# Merging uses half that per step (so 2 steps = 1 pairing tier)
MERGING_STEP = (BAR_HEIGHT_MAX + TIER_GAP) // 2


def value_to_height(value):
    """Map value to bar height (linear interpolation)."""
    return BAR_HEIGHT_MIN + (value - 1) * (BAR_HEIGHT_MAX - BAR_HEIGHT_MIN) / (
        MAX_VALUE - 1
    )


try:
    font = ImageFont.truetype("arial.ttf", 16)
except IOError:
    font = ImageFont.load_default()


def draw_frame(
    visible_positions,
    top_tier_positions,
    bottom_tier_positions,
    highlight_left=None,
    highlight_right=None,
):
    """
    Draws all visible values as horizontal bars.
    visible_positions: dict {value: [(x, y), (x, y), ...]} - positions are bottom of bar
    top_tier_positions: set of (x, y) positions in top tier (for red coloring)
    bottom_tier_positions: set of (x, y) positions in bottom tier (for blue coloring)
    """
    img = Image.new("RGB", (WIDTH, HEIGHT), color=BG_COLOR)
    draw = ImageDraw.Draw(img)

    for val, positions in visible_positions.items():
        bar_height = value_to_height(val)
        for x, y in positions:
            color = BAR_COLOR
            if (x, y) in bottom_tier_positions:
                if val == highlight_left:
                    color = LEFT_COLOR
                elif val == highlight_right:
                    color = RIGHT_COLOR
            elif (x, y) in top_tier_positions:
                if val == highlight_left:
                    color = LEFT_COLOR
                elif val == highlight_right:
                    color = RIGHT_COLOR

            # Draw bar (bottom at y, extending upward)
            draw.rectangle(
                [x, y - bar_height, x + BAR_WIDTH, y],
                fill=color,
                outline=None,
            )

    open_cv_image = np.array(img)
    open_cv_image = open_cv_image[:, :, ::-1].copy()
    return open_cv_image


def extract_winners_only(state_array):
    """Extract just the winner values from state array (no losers)."""
    return [node["value"] for node in state_array]


def build_merge_tier(state_array, y_top, l_check):
    """
    Build a single tier with 2-height elements for merging phase.
    Returns: (top_tier, bottom_tier)

    y_top is the baseline (bottom of bars at this tier).
    Two-tier element: mainchain bar at y_top, loser bar at a fixed baseline below.
    All bottom-tier bars share the same baseline: y_top + BAR_HEIGHT_MAX + TIER_GAP
    """
    top_tier = {}
    bottom_tier = {}

    # Fixed baseline for bottom half (aligned to max height)
    bottom_baseline = y_top + BAR_HEIGHT_MAX + TIER_GAP

    for i, node in enumerate(state_array):
        x = RIGHT_COLUMN_X + (i * BAR_WIDTH)

        # Top half: mainchain value at y_top (baseline)
        top_tier[node["value"]] = (x, y_top)

        # Bottom half: loser bar at fixed baseline
        if i >= 2:
            losers = node.get("losers", [])
            current_l = len(losers)
            if current_l >= l_check and l_check > 0:
                loser_val = losers[l_check - 1]["value"]
                bottom_tier[loser_val] = (x, bottom_baseline)

    return top_tier, bottom_tier


def main():
    print("Loading JSON events...")
    with open("events.json") as f:
        events = json.load(f)

    # Find MAX_VALUE from first init event
    global MAX_VALUE
    for event in events:
        if event and event.get("type") == "state_update":
            phase = list(event["data"].keys())[0]
            if phase == "init":
                state = event["data"][phase]["state"]
                MAX_VALUE = max(node["value"] for node in state)
                print(f"Detected MAX_VALUE: {MAX_VALUE}")
                break

    print("Initializing Video Writer...")
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    video = cv2.VideoWriter("sorting_video_silent.mp4", fourcc, FPS, (WIDTH, HEIGHT))

    # Pairing phase
    pairing_tiers = []
    read_tier = {}
    write_tier = {}
    pairing_tier_index = 0

    # Merging phase: new single-tier approach
    merging_tiers = []  # List of (top_tier, bottom_tier) tuples for history
    merge_current_top = {}
    merge_current_bottom = {}
    merge_l_check = 0  # Persists between inserting phases, resets at merge-init
    merging_tier_index = 0

    last_phase = None
    final_pairing_y = None
    last_state_array = None

    print("Rendering frames...")
    for event in events:
        if not event:
            continue

        if event.get("type") == "state_update":
            phase_key = list(event["data"].keys())[0]
            state_array = event["data"][phase_key]["state"]

            # PAIRING PHASE
            if phase_key == "init":
                # When we encounter init, move read tier to y+1 (new level)
                if last_phase == "pairing":
                    # Save old read_tier before incrementing
                    if read_tier:
                        pairing_tiers.append(read_tier.copy())
                    pairing_tier_index += 1
                elif last_phase != "init" and last_phase is not None:
                    if read_tier:
                        pairing_tiers.append(read_tier.copy())
                    if write_tier:
                        read_tier = write_tier.copy()
                        write_tier = {}

                y = START_Y + (pairing_tier_index * (BAR_HEIGHT_MAX + TIER_GAP))
                read_tier = {}
                for i, node in enumerate(state_array):
                    x = LEFT_COLUMN_X + (i * BAR_WIDTH)
                    read_tier[node["value"]] = (x, y)
                final_pairing_y = y
                last_phase = phase_key

            elif phase_key == "pairing":
                # Write pairing to read_tier + 1 (one tier below), no increment
                y = START_Y + ((pairing_tier_index + 1) * (BAR_HEIGHT_MAX + TIER_GAP))
                write_tier = {}
                winners = extract_winners_only(state_array)
                for i, val in enumerate(winners):
                    x = LEFT_COLUMN_X + (i * BAR_WIDTH)
                    write_tier[val] = (x, y)
                final_pairing_y = y
                last_phase = phase_key

            # MERGING PHASE
            elif phase_key == "merge-init":
                # Save current tier to history before creating new one
                if merge_current_top:
                    merging_tiers.append(
                        (merge_current_top.copy(), merge_current_bottom.copy())
                    )

                # Only freeze left side when FIRST entering merging phase
                if last_phase != "merge-init" and last_phase != "inserting":
                    if read_tier:
                        pairing_tiers.append(read_tier.copy())
                    if write_tier:
                        pairing_tiers.append(write_tier.copy())

                # Stack upward - start from final pairing level, move up by half-tier per step
                merging_tier_index += 2
                y_top = final_pairing_y - ((merging_tier_index - 2) * MERGING_STEP)

                # Set l_check from state (resets at merge-init)
                if len(state_array) >= 3:
                    merge_l_check = len(state_array[2].get("losers", []))
                else:
                    merge_l_check = 0

                # Build new tier
                merge_current_top, merge_current_bottom = build_merge_tier(
                    state_array, y_top, merge_l_check
                )

                last_phase = phase_key

            elif phase_key == "inserting":
                # Stay at same Y level as merge-init
                y_top = final_pairing_y - ((merging_tier_index - 2) * MERGING_STEP)

                # Rebuild current tier using persisted l_check
                merge_current_top, merge_current_bottom = build_merge_tier(
                    state_array, y_top, merge_l_check
                )

                last_phase = phase_key

            last_state_array = state_array

        elif event.get("type") == "straggler_insert":
            # Add straggler to rightmost position of current bottom tier
            if merge_current_bottom is not None:
                y_top = final_pairing_y - ((merging_tier_index - 2) * MERGING_STEP)
                x = RIGHT_COLUMN_X + (len(last_state_array) * BAR_WIDTH)
                merge_current_bottom[event["value"]] = (x, y_top)

        elif event.get("type") == "compare":
            if (
                not read_tier
                and not write_tier
                and not pairing_tiers
                and not merge_current_top
            ):
                continue

            val_left = event["left"]
            val_right = event["right"]

            visible = {}
            top_tier_positions = set()
            bottom_tier_positions = set()

            # Pairing phase
            if last_phase in ["init", "pairing"]:
                for tier in pairing_tiers:
                    for val, pos in tier.items():
                        if val not in visible:
                            visible[val] = []
                        visible[val].append(pos)
                for val, pos in read_tier.items():
                    if val not in visible:
                        visible[val] = []
                    visible[val].append(pos)
                    top_tier_positions.add(pos)
                for val, pos in write_tier.items():
                    if val not in visible:
                        visible[val] = []
                    visible[val].append(pos)

            # Merging phase
            elif last_phase in ["merge-init", "inserting"]:
                # Left side: all pairing history
                for tier in pairing_tiers:
                    for val, pos in tier.items():
                        if val not in visible:
                            visible[val] = []
                        visible[val].append(pos)
                for val, pos in read_tier.items():
                    if val not in visible:
                        visible[val] = []
                    visible[val].append(pos)
                for val, pos in write_tier.items():
                    if val not in visible:
                        visible[val] = []
                    visible[val].append(pos)

                # Right side: merging tiers history (BOTTOM half only)
                for top_tier, bottom_tier in merging_tiers:
                    for val, pos in bottom_tier.items():
                        if val not in visible:
                            visible[val] = []
                        visible[val].append(pos)
                        bottom_tier_positions.add(pos)

                # Current active tier
                for val, pos in merge_current_top.items():
                    if val not in visible:
                        visible[val] = []
                    visible[val].append(pos)
                    top_tier_positions.add(pos)

                for val, pos in merge_current_bottom.items():
                    if val not in visible:
                        visible[val] = []
                    visible[val].append(pos)
                    bottom_tier_positions.add(pos)

            frame = draw_frame(
                visible, top_tier_positions, bottom_tier_positions, val_left, val_right
            )
            video.write(frame)

    video.release()
    print("Video rendered successfully to sorting_video_silent.mp4!")

    # Combine with audio if available
    import os

    if os.path.exists("ford_johnson_sort.wav"):
        print("Combining with audio...")
        import subprocess

        subprocess.run(
            [
                "ffmpeg",
                "-y",
                "-i",
                "sorting_video_silent.mp4",
                "-i",
                "ford_johnson_sort.wav",
                "-c:v",
                "copy",
                "-c:a",
                "aac",
                "-strict",
                "experimental",
                "sorting_video.mp4",
            ],
            capture_output=True,
        )
        print("Combined video with audio: sorting_video.mp4")


if __name__ == "__main__":
    main()
