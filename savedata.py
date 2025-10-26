import os
import sys
import argparse
from typing import List, Tuple, Dict
import numpy as np
import cv2
import mediapipe as mp


def log(msg: str) -> None:
    sys.stderr.write(msg + "\n")
    sys.stderr.flush()


def parse_indices_arg(indices_str: str) -> List[Tuple[int, int]]:
    if not indices_str:
        return []
    out = []
    for token in indices_str.split(","):
        t = token.strip()
        if len(t) != 4 or not t.isdigit():
            raise ValueError(f"Invalid index token: '{t}' (must be 4 digits like 1230)")
        lm_id = int(t[:3])
        coord_id = int(t[3])
        if not (0 <= lm_id <= 468 and 0 <= coord_id <= 2):
            raise ValueError(f"Out of range: '{t}'")
        out.append((lm_id, coord_id))
    seen = set()
    uniq = []
    for x in out:
        if x not in seen:
            uniq.append(x)
            seen.add(x)
    return uniq


def all_xyz_indices() -> List[Tuple[int, int]]:
    idx = []
    for lm in range(468):
        for c in (0, 1, 2):
            idx.append((lm, c))
    return idx


def index_code(lm_id: int, coord_id: int) -> str:
    return f"{lm_id:03d}{coord_id:d}"


def capture_face_landmarks(
    cam_index: int,
    width: int,
    height: int,
    frames: int,
    min_valid: int,
    refine: bool = False,
) -> List[List[Tuple[float, float, float]]]:
    cap = cv2.VideoCapture(cam_index, cv2.CAP_DSHOW)
    if not cap.isOpened():
        cap = cv2.VideoCapture(cam_index)
        if not cap.isOpened():
            raise RuntimeError("Failed to open camera. Check device index or permissions.")

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    cap.set(cv2.CAP_PROP_FPS, 30)

    mp_face_mesh = mp.solutions.face_mesh
    mesh = mp_face_mesh.FaceMesh(
        static_image_mode=False,
        refine_landmarks=refine,
        max_num_faces=1,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5,
    )

    collected: List[List[Tuple[float, float, float]]] = []
    try:
        log("Keep your face steady in view... collecting frames.")
        attempts = 0
        while attempts < frames:
            ok, frame_bgr = cap.read()
            if not ok or frame_bgr is None:
                attempts += 1
                continue
            rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
            results = mesh.process(rgb)
            attempts += 1
            if results.multi_face_landmarks:
                lm = results.multi_face_landmarks[0].landmark
                if len(lm) >= 468:
                    collected.append([(p.x, p.y, p.z) for p in lm[:468]])
            if attempts % 20 == 0:
                log(f"Progress: {attempts}/{frames} frames, valid={len(collected)}")

            if len(collected) >= frames:
                break

        if len(collected) < min_valid:
            raise RuntimeError(
                f"Not enough valid frames: {len(collected)} < {min_valid}. "
                "Make sure your face is fully visible and lighting is good."
            )

        return collected
    finally:
        try:
            mesh.close()
        except Exception:
            pass
        try:
            cap.release()
        except Exception:
            pass


def aggregate_values(
    collected: List[List[Tuple[float, float, float]]],
    indices: List[Tuple[int, int]],
) -> Dict[str, float]:
    if not indices:
        indices = all_xyz_indices()

    buckets: Dict[str, List[float]] = {index_code(lm, c): [] for lm, c in indices}

    for frame_pts in collected:
        for lm_id, coord_id in indices:
            x, y, z = frame_pts[lm_id]
            val = x if coord_id == 0 else (y if coord_id == 1 else z)
            buckets[index_code(lm_id, coord_id)].append(float(val))

    out: Dict[str, float] = {}
    for key, arr in buckets.items():
        if not arr:
            out[key] = 0.0
        else:
            out[key] = float(np.median(np.asarray(arr, dtype=np.float32)))
    return out

def write_profile(path: str, values: Dict[str, float]) -> None:
    with open(path, "w", encoding="utf-8") as f:
        for key in sorted(values.keys()):
            f.write(f"{key} {values[key]:.8f}\n")
        f.write("FFFF")

def main():
    ap = argparse.ArgumentParser(
        description="Enroll a user's MediaPipe FaceMesh profile on Windows (USB camera)."
    )
    ap.add_argument("--out", required=True, help="Output profile txt path")
    ap.add_argument("--cam", type=int, default=0, help="Camera index (default 0)")
    ap.add_argument("--width", type=int, default=1280, help="Capture width")
    ap.add_argument("--height", type=int, default=720, help="Capture height")
    ap.add_argument("--frames", type=int, default=120, help="Frames to attempt collecting")
    ap.add_argument("--min-valid", type=int, default=40, help="Minimum valid frames required")
    ap.add_argument(
        "--indices",
        type=str,
        default="",
        help="Comma-separated 4-digit indices to store, e.g. 1230,0561,0072. "
             "Empty means store all x,y,z for all 468 landmarks.",
    )
    ap.add_argument(
        "--refine",
        action="store_true",
        help="Use refine_landmarks=True for potentially better stability (slower).",
    )
    args = ap.parse_args()

    try:
        indices = parse_indices_arg(args.indices)
        collected = capture_face_landmarks(
            cam_index=args.cam,
            width=args.width,
            height=args.height,
            frames=args.frames,
            min_valid=args.min_valid,
            refine=args.refine,
        )
        values = aggregate_values(collected, indices)
        write_profile(args.out, values)
        print(f"Saved profile to: {args.out}")
    except KeyboardInterrupt:
        log("Interrupted by user.")
        sys.exit(1)
    except Exception as e:
        log(f"Enrollment failed: {e}")
        sys.exit(2)


if __name__ == "__main__":
    main()
