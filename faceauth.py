import os
import sys
import re
import time
import math
import threading
from typing import Dict, List, Tuple
import numpy as np

try:
    import cv2
except Exception as e:
    cv2 = None

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
        if cv2 is None:
            raise RuntimeError("OpenCV not available. pip install opencv-python")
        if mp is None:
            raise RuntimeError("mediapipe not available. pip install mediapipe")

        cam_index = int(os.getenv("FACE_AUTH_CAM_INDEX", "0"))
        self.cap = cv2.VideoCapture(cam_index)

        w = int(os.getenv("FACE_AUTH_CAM_WIDTH", "1280"))
        h = int(os.getenv("FACE_AUTH_CAM_HEIGHT", "720"))
        try:
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, w)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, h)
            self.cap.set(cv2.CAP_PROP_FPS, int(os.getenv("FACE_AUTH_CAM_FPS", "30")))
        except Exception:
            pass

        if not self.cap.isOpened():
            raise RuntimeError("Failed to open USB camera. Check device index or permissions.")

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
            if self.cap:
                self.cap.release()
        except Exception:
            pass

    def capture_landmarks(self, attempts: int = 60, sleep_s: float = 0.05) -> List[Tuple[float, float, float]]:
        for _ in range(attempts):
            ok, frame_bgr = self.cap.read()
            if not ok or frame_bgr is None:
                time.sleep(sleep_s)
                continue
            rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
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
    in_path = os.getenv("FACE_AUTH_INPUT_FILE", "input.txt")
    out_path = os.getenv("FACE_AUTH_OUTPUT_FILE", "output.txt")
    profile_path = os.getenv("FACE_AUTH_PROFILE_PATH", "user1.txt").strip()
    poll_s = float(os.getenv("FACE_AUTH_POLL_SEC", "0.1"))
    max_attempts = int(os.getenv("FACE_AUTH_MAX_ATTEMPTS", "60"))

    try:
        camera = CameraFaceMesh()
    except Exception as e:
        log(f"Camera/MediaPipe init error: {e}")
        camera = None

    def read_int(path: str):
        try:
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                txt = f.read().strip()
            if not txt:
                return None
            m = re.search(r"-?\d+", txt)
            return int(m.group(0)) if m else None
        except Exception:
            return None

    def write_int(path: str, val: int):
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(str(val))
        except Exception as e:
            log(f"Write error for {path}: {e}")

    write_int(out_path, 1)
    print("Initialized")
    last_in = read_int(in_path)
    if last_in is None:
        last_in = 1

    try:
        while True:
            time.sleep(poll_s)
            cur = read_int(in_path)
            if cur is None or cur == last_in:
                continue

            if last_in != 2 and cur == 2:
                write_int(out_path, 2)
                print("Starting face authentication...")
                ok = False
                if not profile_path:
                    log("FACE_AUTH_PROFILE_PATH is not set")
                else:
                    try:
                        indices, stored_vec = parse_profile(profile_path)
                        if camera is not None:
                            ok = match_profile(camera, indices, stored_vec, max_attempts)
                    except Exception as e:
                        log(f"Face recognition error: {e}")
                        ok = False
                write_int(out_path, 3 if ok else 4)
                print("Face authentication completed:", "Success" if ok else "Failure")

            elif last_in != 1 and cur == 1:
                write_int(out_path, 1)
                print("Switching to standby mode")

            elif last_in != 0 and cur == 0:
                write_int(out_path, 0)
                if camera:
                    camera.close()
                print("Program terminated")
                break

            last_in = cur
    except Exception:
        pass

if __name__ == "__main__":
    main()