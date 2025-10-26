#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import re
import time
import math
import threading
from typing import Dict, List, Tuple
import numpy as np

try:
    from picamera2 import Picamera2
except Exception as e:
    Picamera2 = None

try:
    import mediapipe as mp
except Exception as e:
    mp = None

def log(msg: str) -> None:
    if os.getenv("FACE_AUTH_LOG"):
        sys.stderr.write(msg + "\n")
        sys.stderr.flush()

def parse_profile(txt_path: str) -> Tuple[List[Tuple[int, int]], np.ndarray]:
    idx_list: List[Tuple[int, int]] = []
    vals: List[float] = []

    if not os.path.exists(txt_path):
        raise FileNotFoundError(f"Profile not found: {txt_path}")

    idx_re = re.compile(r"\b(\d{4})\b")
    val_re = re.compile(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?")

    with open(txt_path, "r", encoding="utf-8", errors="ignore") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue
            if "FFFF" in line.upper():
                break
            m_idx = idx_re.search(line)
            m_vals = val_re.findall(line)
            if not m_idx or not m_vals:
                continue
            idx = m_idx.group(1)
            try:
                v = float(m_vals[-1])
            except ValueError:
                continue
            lm_id = int(idx[:3])
            coord_id = int(idx[3])
            if not (0 <= lm_id <= 468 and 0 <= coord_id <= 2):
                continue
            idx_list.append((lm_id, coord_id))
            vals.append(v)

    if not idx_list:
        raise ValueError("No valid indices parsed from profile. Check file format.")

    return idx_list, np.asarray(vals, dtype=np.float32)

class CameraFaceMesh:
    def __init__(self):
        if Picamera2 is None:
            raise RuntimeError("picamera2 not available. Install and enable camera.")
        if mp is None:
            raise RuntimeError("mediapipe not available. pip install mediapipe")

        self.picam = Picamera2()
        try:
            self.picam.configure(self.picam.create_preview_configuration(main={"format": "RGB888", "size": (1280, 720)}))
        except Exception:
            pass
        self.picam.start()
        time.sleep(0.3)

        self._mp_face_mesh = mp.solutions.face_mesh
        self._face_mesh = self._mp_face_mesh.FaceMesh(
            static_image_mode=False,
            refine_landmarks=False,
            max_num_faces=1,
            min_detection_confidence=0.5,
            min_tracking_confidence=0.5,
        )

    def close(self):
        try:
            if hasattr(self, "_face_mesh"):
                self._face_mesh.close()
        except Exception:
            pass
        try:
            self.picam.stop()
        except Exception:
            pass

    def capture_landmarks(self, attempts: int = 60, sleep_s: float = 0.05) -> List[Tuple[float, float, float]]:
        for i in range(attempts):
            frame = self.picam.capture_array()
            rgb = frame
            results = self._face_mesh.process(rgb)
            if results.multi_face_landmarks:
                lm = results.multi_face_landmarks[0].landmark
                return [(p.x, p.y, p.z) for p in lm]
            time.sleep(sleep_s)
        return []

def zscore(v: np.ndarray) -> np.ndarray:
    m = float(v.mean())
    s = float(v.std())
    if s < 1e-8:
        return v * 0.0
    return (v - m) / s

def cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    if a.size == 0 or b.size == 0:
        return -1.0
    na = float(np.linalg.norm(a))
    nb = float(np.linalg.norm(b))
    if na < 1e-8 or nb < 1e-8:
        return -1.0
    return float(np.dot(a, b) / (na * nb))

def build_vector_from_landmarks(indices: List[Tuple[int, int]], landmarks: List[Tuple[float, float, float]]) -> np.ndarray:
    out = []
    for lm_id, coord_id in indices:
        if lm_id >= len(landmarks):
            out.append(np.nan)
            continue
        x, y, z = landmarks[lm_id]
        if coord_id == 0:
            out.append(x)
        elif coord_id == 1:
            out.append(y)
        else:
            out.append(z)
    v = np.asarray(out, dtype=np.float32)
    if np.isnan(v).any():
        finite = v[np.isfinite(v)]
        fill = float(finite.mean()) if finite.size else 0.0
        v = np.where(np.isfinite(v), v, fill)
    return v


def match_profile(camera: CameraFaceMesh, indices: List[Tuple[int, int]], stored_vec: np.ndarray, max_attempts: int) -> bool:
    attempts = max(1, int(max_attempts))
    for i in range(attempts):
        lms = camera.capture_landmarks(attempts=1)
        if not lms:
            continue
        measured = build_vector_from_landmarks(indices, lms)
        s_norm = zscore(stored_vec)
        m_norm = zscore(measured)
        sim = cosine_similarity(s_norm, m_norm)
        log(f"attempt {i+1}: cosine={sim:.6f}")
        thresh = float(os.getenv("FACE_AUTH_THRESHOLD", "0.97"))
        if sim >= thresh:
            return True
    return False

def main():
    try:
        camera = CameraFaceMesh()
    except Exception as e:
        log(f"Camera/MediaPipe init error: {e}")
        camera = None

    sys.stdout.write("initialized\n")
    sys.stdout.flush()

    try:
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            cmd = line.strip()
            if not cmd:
                continue

            lower = cmd.lower()
            if lower.startswith("face recognition"):
                path = ""
                if "," in cmd:
                    path = cmd.split(",", 1)[1].strip()
                else:
                    parts = cmd.split(None, 2)
                    if len(parts) >= 3:
                        path = parts[2].strip()
                if not path:
                    log("No txt path provided for face recognition")
                    sys.stdout.write("false\n")
                    sys.stdout.flush()
                    continue
                try:
                    indices, stored_vec = parse_profile(path)
                except Exception as e:
                    log(f"Profile parse error: {e}")
                    sys.stdout.write("false\n")
                    sys.stdout.flush()
                    continue

                if camera is None:
                    sys.stdout.write("false\n")
                    sys.stdout.flush()
                    continue

                max_attempts = int(os.getenv("FACE_AUTH_MAX_ATTEMPTS", "60"))
                try:
                    ok = match_profile(camera, indices, stored_vec, max_attempts)
                except Exception as e:
                    log(f"Match error: {e}")
                    ok = False
                sys.stdout.write(("true" if ok else "false") + "\n")
                sys.stdout.flush()

            elif lower.startswith("terminate"):
                sys.stdout.write("terminated\n")
                sys.stdout.flush()
                break
            else:
                log(f"Unknown command: {cmd}")
    finally:
        try:
            if camera:
                camera.close()
        except Exception:
            pass


if __name__ == "__main__":
    main()
