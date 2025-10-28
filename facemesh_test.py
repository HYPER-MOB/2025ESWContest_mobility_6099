import cv2
import time
import mediapipe as mp

DEVICE_INDEX = 0
SELFIE_VIEW = True
REFINE_LANDMARKS = True
MIN_DET_CONF = 0.5
MIN_TRK_CONF = 0.5
MAX_NUM_FACES = 2

mp_drawing = mp.solutions.drawing_utils
mp_styles = mp.solutions.drawing_styles
mp_face_mesh = mp.solutions.face_mesh

landmark_spec = mp_drawing.DrawingSpec(thickness=1, circle_radius=1)
connection_spec = mp_drawing.DrawingSpec(thickness=1, circle_radius=1)

def main():
    cap = cv2.VideoCapture(DEVICE_INDEX)
    if not cap.isOpened():
        raise RuntimeError(f"Cannot open camera index {DEVICE_INDEX}")

    prev_time = time.time()
    fps = 0.0

    with mp_face_mesh.FaceMesh(
        max_num_faces=MAX_NUM_FACES,
        refine_landmarks=REFINE_LANDMARKS,
        min_detection_confidence=MIN_DET_CONF,
        min_tracking_confidence=MIN_TRK_CONF
    ) as face_mesh:

        while True:
            ok, frame = cap.read()
            if not ok:
                break

            if SELFIE_VIEW:
                frame = cv2.flip(frame, 1)

            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            result = face_mesh.process(rgb)
            vis = frame.copy()

            face_count = 0
            if result.multi_face_landmarks:
                face_count = len(result.multi_face_landmarks)
                for face_landmarks in result.multi_face_landmarks:
                    mp_drawing.draw_landmarks(
                        image=vis,
                        landmark_list=face_landmarks,
                        connections=mp_face_mesh.FACEMESH_TESSELATION,
                        landmark_drawing_spec=None,
                        connection_drawing_spec=mp_styles.get_default_face_mesh_tesselation_style()
                    )
                    mp_drawing.draw_landmarks(
                        image=vis,
                        landmark_list=face_landmarks,
                        connections=mp_face_mesh.FACEMESH_CONTOURS,
                        landmark_drawing_spec=None,
                        connection_drawing_spec=mp_styles.get_default_face_mesh_contours_style()
                    )
                    if REFINE_LANDMARKS:
                        mp_drawing.draw_landmarks(
                            image=vis,
                            landmark_list=face_landmarks,
                            connections=mp_face_mesh.FACEMESH_IRISES,
                            landmark_drawing_spec=None,
                            connection_drawing_spec=mp_styles.get_default_face_mesh_iris_connections_style()
                        )

            cur_time = time.time()
            dt = cur_time - prev_time
            if dt > 0:
                fps = 1.0 / dt
            prev_time = cur_time

            info_lines = [
                f"Faces: {face_count}",
                f"FPS: {fps:5.1f}",
                f"min_det_conf: {MIN_DET_CONF:.2f}  min_trk_conf: {MIN_TRK_CONF:.2f}",
                f"refine: {REFINE_LANDMARKS}  selfie_view: {SELFIE_VIEW}"
            ]
            y = 22
            for line in info_lines:
                cv2.putText(vis, line, (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2, cv2.LINE_AA)
                y += 24

            cv2.imshow("MediaPipe FaceMesh Test", vis)

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
