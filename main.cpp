#include <QtCore>
#include "ipc.h"

#include <sys/stat.h>
extern "C" {
#include "can_api.h"
}

#include <sys/types.h>
#include <unistd.h>

// 콜백 user 구분용 라벨
static const char* kBusCan0 = "can0";
static const char* kBusCan1 = "can1";


// ── 소켓 경로 ────────────────────────────────────────────────────────────────
static const QString kSock = "/tmp/dcu.demo.sock";
static QPointer<IpcConnection> g_conn;

// ── 메시지 ID (표 기준) ──────────────────────────────────────────────────────
// Body CAN (DCU -> Power*)
static constexpr uint32_t ID_DCU_SEAT_ORDER   = 0x001; // DLC 4 (→ can1 TX)
static constexpr uint32_t ID_DCU_MIRROR_ORDER = 0x002; // DLC 6 (→ can1 TX)
static constexpr uint32_t ID_DCU_WHEEL_ORDER  = 0x003; // DLC 2 (→ can1 TX)
// Power* -> DCU (20ms) (← can1 RX)
static constexpr uint32_t ID_POW_SEAT_STATE   = 0x100; // DLC 5(+flags)
static constexpr uint32_t ID_POW_MIRROR_STATE = 0x200; // DLC 7
static constexpr uint32_t ID_POW_WHEEL_STATE  = 0x300; // DLC 3

// SCA/TCU 영역 (인증/프로필) (can0 TX/RX)
static constexpr uint32_t ID_DCU_SCA_USER_FACE_REQ       = 0x001; // DLC 1
static constexpr uint32_t ID_SCA_TCU_USER_INFO_REQ       = 0x002; // DLC 1
static constexpr uint32_t ID_SCA_DCU_AUTH_STATE          = 0x003; // DLC 2
static constexpr uint32_t ID_TCU_SCA_USER_INFO           = 0x004; // DLC 8 (미정)
static constexpr uint32_t ID_SCA_DCU_AUTH_RESULT         = 0x005; // DLC 8
static constexpr uint32_t ID_SCA_DCU_AUTH_RESULT_ADD     = 0x006; // DLC 8
static constexpr uint32_t ID_TCU_SCA_USER_INFO_NFC       = 0x007; // DLC 8
static constexpr uint32_t ID_TCU_SCA_USER_INFO_BLE       = 0x008; // DLC ? (미정)
static constexpr uint32_t ID_SCA_TCU_USER_INFO_ACK       = 0x014; // DLC 2
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_REQ    = 0x011; // DLC 1
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_SEAT   = 0x051; // DLC 4
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_MIRROR = 0x052; // DLC 6
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_WHEEL  = 0x053; // DLC 2
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_ACK    = 0x055; // DLC 2
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_SEAT_UPDATE   = 0x061; // DLC4
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_MIRROR_UPDATE = 0x062; // DLC6
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_WHEEL_UPDATE  = 0x063; // DLC2
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_UPDATE_ACK    = 0x065; // DLC2

// ── 내부 상태(IPC <-> CAN 공유 변수) ─────────────────────────────────────────
static int m_seatPosition     = 20;   // 0~100 (%)
static int m_seatAngle        = 10;   // 0~180 (deg)
static int m_seatFrontHeight  = 20;   // 0~100 (%)
static int m_seatRearHeight   = 20;   // 0~100 (%)
static int m_sideMirrorLeftYaw    = 10;  // -45~45
static int m_sideMirrorLeftPitch  = 20;  // -45~45
static int m_sideMirrorRightYaw   = 20;  // -45~45
static int m_sideMirrorRightPitch = 20;  // -45~45
static int m_roomMirrorYaw   = 0;    // -45~45
static int m_roomMirrorPitch = 0;    // -45~45
static int m_handlePosition  = 0;    // 0~100
static int m_handleAngle     = 0;    // -90~90

// 프로필 수신 완료 체크
static bool g_profSeatOK = false;
static bool g_profMirrorOK = false;
static bool g_profWheelOK = false;

// 구독 ID (종료 시 해제용)
static int g_canSubPowId = 0; // can1 (Power*)
static int g_canSubScaId = 0; // can0 (SCA/TCU)

// ── 유틸 ─────────────────────────────────────────────────────────────────────
static inline int clampInt(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }
static inline uint8_t encPercent100(int v) { return (uint8_t)clampInt(v, 0, 100); }
static inline uint8_t encAngle180_fromSigned(int degSigned, int offset) {
    return (uint8_t)clampInt(degSigned + offset, 0, 180);
}
static inline int decAngle180_toSigned(uint8_t raw, int offset) {
    return clampInt((int)raw - offset, -offset, 180 - offset);
}

// ── 전역에 추가 ─────────────────────────────────────────
static QString g_lastAuthReqId;
static QString g_lastDataReqId;
static QByteArray g_accUserId;

static inline QString bytesToHex(const uint8_t* d, int n) {
    QString s;
    for (int i = 0; i < n; ++i) {
        s += QString("%1").arg(d[i], 2, 16, QLatin1Char('0')).toUpper();
        if (i+1 < n) s += ' ';
    }
    return s;
}

// ── IPC 송신 ────────────────────────────────────────────────────────────────
static void sendSystemStart(IpcConnection* c, const QString& reqId = {}) {
    if (!c) return;
    c->send({"system/start", reqId, QJsonObject{
        {"ok", true},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    }});
}
static void sendSystemWarning(IpcConnection* c, const QString& code, const QString& msg, const QString& reqId = {}) {
    if (!c) return;
    c->send({"system/warning", reqId, QJsonObject{{"code", code}, {"message", msg}}});
}
static void sendAuthProcess(IpcConnection* c, const QString& text, const QString& reqId = {}) {
    if (!c) return;
    c->send({"auth/process", reqId, QJsonObject{
        {"message", text},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    }});
}
static void sendAuthResult(IpcConnection* c, const QString& userId, double score, int error = 0, const QString& reqId = {}) {
    if (!c) return;
    c->send({"auth/result", reqId, QJsonObject{{"error", error},{"userId", userId},{"score", score}}});
}
static void sendDataResult(IpcConnection* c, int error = 0, const QString& reqId = {}) {
    if (!c) return;
    QJsonObject data{
        {"seatPosition", m_seatPosition}, {"seatAngle", m_seatAngle},
        {"seatFrontHeight", m_seatFrontHeight}, {"seatRearHeight", m_seatRearHeight},
        {"sideMirrorLeftYaw", m_sideMirrorLeftYaw}, {"sideMirrorLeftPitch", m_sideMirrorLeftPitch},
        {"sideMirrorRightYaw", m_sideMirrorRightYaw}, {"sideMirrorRightPitch", m_sideMirrorRightPitch},
        {"roomMirrorYaw", m_roomMirrorYaw}, {"roomMirrorPitch", m_roomMirrorPitch},
        {"handlePosition", m_handlePosition}, {"handleAngle", m_handleAngle}
    };
    c->send({"data/result", reqId, QJsonObject{{"error", error},{"data", data}}});
}
static void sendPowerApplyAck(IpcConnection* c, bool ok, const QString& reqId = {}) {
    if (!c) return;
    c->send({"power/apply/ack", reqId, QJsonObject{{"ok", ok}}});
}
static void sendPowerState(IpcConnection* c) {
    if (!c) return;
    QJsonObject data{
        {"seatPosition", m_seatPosition}, {"seatAngle", m_seatAngle},
        {"seatFrontHeight", m_seatFrontHeight}, {"seatRearHeight", m_seatRearHeight},
        {"sideMirrorLeftYaw", m_sideMirrorLeftYaw}, {"sideMirrorLeftPitch", m_sideMirrorLeftPitch},
        {"sideMirrorRightYaw", m_sideMirrorRightYaw}, {"sideMirrorRightPitch", m_sideMirrorRightPitch},
        {"roomMirrorYaw", m_roomMirrorYaw}, {"roomMirrorPitch", m_roomMirrorPitch},
        {"handlePosition", m_handlePosition}, {"handleAngle", m_handleAngle}
    };
    c->send({"power/state", {}, data});
}

// ── CAN TX (경로 분리) ──────────────────────────────────────────────────────
static inline CanFrame mkFrame(uint32_t id, uint8_t dlc) {
    CanFrame f{}; f.id = id; f.dlc = dlc; f.flags = 0; memset(f.data, 0, 8); return f;
}

// → Power* 로 가는 오더는 can1
static void CAN_Tx_SEAT_ORDER() {
    CanFrame f = mkFrame(ID_DCU_SEAT_ORDER, 4);
    f.data[0] = encPercent100(m_seatPosition);
    f.data[1] = (uint8_t)clampInt(m_seatAngle, 0, 180);
    f.data[2] = encPercent100(m_seatFrontHeight);
    f.data[3] = encPercent100(m_seatRearHeight);
    qInfo() << "[CAN1 TX] SEAT_ORDER id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can1", f, 0);
}
static void CAN_Tx_MIRROR_ORDER() {
    CanFrame f = mkFrame(ID_DCU_MIRROR_ORDER, 6);
    f.data[0] = encAngle180_fromSigned(m_sideMirrorLeftYaw,   90);
    f.data[1] = encAngle180_fromSigned(m_sideMirrorLeftPitch, 90);
    f.data[2] = encAngle180_fromSigned(m_sideMirrorRightYaw,  90);
    f.data[3] = encAngle180_fromSigned(m_sideMirrorRightPitch,90);
    f.data[4] = encAngle180_fromSigned(m_roomMirrorYaw,       90);
    f.data[5] = encAngle180_fromSigned(m_roomMirrorPitch,     90);
    qInfo() << "[CAN1 TX] MIRROR_ORDER id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can1", f, 0);
}
static void CAN_Tx_WHEEL_ORDER() {
    CanFrame f = mkFrame(ID_DCU_WHEEL_ORDER, 2);
    f.data[0] = encPercent100(m_handlePosition);
    f.data[1] = encAngle180_fromSigned(m_handleAngle, 90);
    qInfo() << "[CAN1 TX] WHEEL_ORDER id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can1", f, 0);
}

// → SCA/TCU 로 가는 신호는 can0
static void CAN_Tx_USER_FACE_REQ() {
    CanFrame f = mkFrame(ID_DCU_SCA_USER_FACE_REQ, 1);
    f.data[0] = 1; // 트리거
    qInfo() << "[CAN0 TX] USER_FACE_REQ id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}
static void CAN_Tx_USER_PROFILE_REQ() {
    CanFrame f = mkFrame(ID_DCU_TCU_USER_PROFILE_REQ, 1);
    f.data[0] = 1; // 요청
    g_profSeatOK = g_profMirrorOK = g_profWheelOK = false;
    qInfo() << "[CAN0 TX] USER_PROFILE_REQ id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}
static void CAN_Tx_USER_PROFILE_ACK(uint8_t index /*1:Seat,2:Mirror,3:Wheel*/, uint8_t state /*0:OK,1:Partial*/) {
    CanFrame f = mkFrame(ID_DCU_TCU_USER_PROFILE_ACK, 2);
    f.data[0] = index;
    f.data[1] = state;
    qInfo() << "[CAN0 TX] USER_PROFILE_ACK id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}
static void CAN_Tx_USER_PROFILE_SEAT_UPDATE() {
    CanFrame f = mkFrame(ID_DCU_TCU_USER_PROFILE_SEAT_UPDATE, 4);
    f.data[0] = encPercent100(m_seatPosition);
    f.data[1] = (uint8_t)clampInt(m_seatAngle, 0, 180);
    f.data[2] = encPercent100(m_seatFrontHeight);
    f.data[3] = encPercent100(m_seatRearHeight);
    qInfo() << "[CAN0 TX] PROFILE_SEAT_UPDATE id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}
static void CAN_Tx_USER_PROFILE_MIRROR_UPDATE() {
    CanFrame f = mkFrame(ID_DCU_TCU_USER_PROFILE_MIRROR_UPDATE, 6);
    f.data[0] = encAngle180_fromSigned(m_sideMirrorLeftYaw,   90);
    f.data[1] = encAngle180_fromSigned(m_sideMirrorLeftPitch, 90);
    f.data[2] = encAngle180_fromSigned(m_sideMirrorRightYaw,  90);
    f.data[3] = encAngle180_fromSigned(m_sideMirrorRightPitch,90);
    f.data[4] = encAngle180_fromSigned(m_roomMirrorYaw,       90);
    f.data[5] = encAngle180_fromSigned(m_roomMirrorPitch,     90);
    qInfo() << "[CAN0 TX] PROFILE_MIRROR_UPDATE id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}
static void CAN_Tx_USER_PROFILE_WHEEL_UPDATE() {
    CanFrame f = mkFrame(ID_DCU_TCU_USER_PROFILE_WHEEL_UPDATE, 2);
    f.data[0] = encPercent100(m_handlePosition);
    f.data[1] = encAngle180_fromSigned(m_handleAngle, 90);
    qInfo() << "[CAN0 TX] PROFILE_WHEEL_UPDATE id=0x" << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}

// ── CAN RX (버스 구분은 ID로 충분하여 공용 콜백 사용) ───────────────────────
static void onCanRx(const CanFrame* fr, void* user) {
    const char* bus = (const char*)user;

    // 🔸요청한 로깅: can0에서 id 0x100 들어오면 한 번 출력하고 종료
    if (bus && strcmp(bus, "can0") == 0 && fr->id == 0x100) {
        qInfo() << "[CAN0 RX] DEBUG id=0x"
                << QString::number(fr->id, 16).toUpper()
                << "dlc=" << fr->dlc
                << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        return; // 좌석 상태 등 기존 분기 타지 않도록 조기 종료
    }
    // 좌석 상태 (can1)
    if (fr->id == ID_POW_SEAT_STATE && fr->dlc >= 4) {
        qInfo() << "[CAN1 RX] SEAT_STATE  id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_seatPosition    = fr->data[0];
        m_seatAngle       = fr->data[1];
        m_seatFrontHeight = fr->data[2];
        m_seatRearHeight  = fr->data[3];
        if (g_conn) QMetaObject::invokeMethod(g_conn, []{ sendPowerState(g_conn); }, Qt::QueuedConnection);
        return;
    }

    // 미러 상태 (can1)
    if (fr->id == ID_POW_MIRROR_STATE && fr->dlc >= 6) {
        qInfo() << "[CAN1 RX] MIRROR_STATE id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_sideMirrorLeftYaw    = decAngle180_toSigned(fr->data[0], 90);
        m_sideMirrorLeftPitch  = decAngle180_toSigned(fr->data[1], 90);
        m_sideMirrorRightYaw   = decAngle180_toSigned(fr->data[2], 90);
        m_sideMirrorRightPitch = decAngle180_toSigned(fr->data[3], 90);
        m_roomMirrorYaw        = decAngle180_toSigned(fr->data[4], 90);
        m_roomMirrorPitch      = decAngle180_toSigned(fr->data[5], 90);
        if (g_conn) QMetaObject::invokeMethod(g_conn, []{ sendPowerState(g_conn); }, Qt::QueuedConnection);
        return;
    }

    // 핸들 상태 (can1)
    if (fr->id == ID_POW_WHEEL_STATE && fr->dlc >= 2) {
        qInfo() << "[CAN1 RX] WHEEL_STATE id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_handlePosition = fr->data[0];
        m_handleAngle    = decAngle180_toSigned(fr->data[1], 90);
        if (g_conn) QMetaObject::invokeMethod(g_conn, []{ sendPowerState(g_conn); }, Qt::QueuedConnection);
        return;
    }

    // 인증 단계 상태 (can0)
    if (fr->id == ID_SCA_DCU_AUTH_STATE && fr->dlc >= 2) {
        qInfo() << "[CAN0 RX] AUTH_STATE  id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        const uint8_t step  = fr->data[0];
        const uint8_t state = fr->data[1];
        if (g_conn) {
            const QString msg = QString("Auth step=%1 state=%2").arg(step).arg(state);
            QMetaObject::invokeMethod(g_conn, [msg]{ sendAuthProcess(g_conn, msg); }, Qt::QueuedConnection);
        }
        return;
    }

    // 인증 결과 (can0)
    if (fr->id == ID_SCA_DCU_AUTH_RESULT && fr->dlc >= 1) {
        qInfo() << "[CAN0 RX] AUTH_RESULT id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        const uint8_t flag = fr->data[0];   // 0x01: fail, 그 외: ok

        if (flag == 0x01) {
            g_accUserId.clear();
            if (g_conn) QMetaObject::invokeMethod(g_conn, []{
                sendAuthResult(g_conn, QString(), 0.0, /*error=*/1, /*reqId=*/g_lastAuthReqId);
            }, Qt::QueuedConnection);
            return;
        }

        g_accUserId.clear();
        if (fr->dlc >= 8) g_accUserId.append((const char*)&fr->data[1], 7);
        if (g_conn) {
            const QString uid = QString::fromLatin1(g_accUserId);
            QMetaObject::invokeMethod(g_conn, [uid]{ sendAuthResult(g_conn, uid, 0.95, 0, g_lastAuthReqId); }, Qt::QueuedConnection);
        }
        return;
    }

    // 인증 결과 추가 (can0)
    if (fr->id == ID_SCA_DCU_AUTH_RESULT_ADD && fr->dlc >= 1) {
        qInfo() << "[CAN0 RX] AUTH_RESULT_ADD id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        g_accUserId.append((const char*)fr->data, std::min<int>(fr->dlc, 8));
        if (g_conn) {
            const QString uid = QString::fromLatin1(g_accUserId);
            QMetaObject::invokeMethod(g_conn, [uid]{ sendAuthResult(g_conn, uid, 0.95, 0, g_lastAuthReqId); }, Qt::QueuedConnection);
        }
        return;
    }

    // 프로필 Seat (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_SEAT && fr->dlc >= 4) {
        qInfo() << "[CAN0 RX] PROFILE_SEAT id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_seatPosition    = fr->data[0];
        m_seatAngle       = fr->data[1];
        m_seatFrontHeight = fr->data[2];
        m_seatRearHeight  = fr->data[3];
        g_profSeatOK = true;
        if (g_conn && g_profSeatOK && g_profMirrorOK && g_profWheelOK)
            QMetaObject::invokeMethod(g_conn, []{ sendDataResult(g_conn, 0, g_lastDataReqId); }, Qt::QueuedConnection);
        return;
    }

    // 프로필 Mirror (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_MIRROR && fr->dlc >= 6) {
        qInfo() << "[CAN0 RX] PROFILE_MIRROR id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_sideMirrorLeftYaw    = decAngle180_toSigned(fr->data[0], 90);
        m_sideMirrorLeftPitch  = decAngle180_toSigned(fr->data[1], 90);
        m_sideMirrorRightYaw   = decAngle180_toSigned(fr->data[2], 90);
        m_sideMirrorRightPitch = decAngle180_toSigned(fr->data[3], 90);
        m_roomMirrorYaw        = decAngle180_toSigned(fr->data[4], 90);
        m_roomMirrorPitch      = decAngle180_toSigned(fr->data[5], 90);
        g_profMirrorOK = true;
        if (g_conn && g_profSeatOK && g_profMirrorOK && g_profWheelOK)
            QMetaObject::invokeMethod(g_conn, []{ sendDataResult(g_conn, 0, g_lastDataReqId); }, Qt::QueuedConnection);
        return;
    }

    // 프로필 Wheel (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_WHEEL && fr->dlc >= 2) {
        qInfo() << "[CAN0 RX] PROFILE_WHEEL id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_handlePosition = fr->data[0];
        m_handleAngle    = decAngle180_toSigned(fr->data[1], 90);
        g_profWheelOK = true;
        if (g_conn && g_profSeatOK && g_profMirrorOK && g_profWheelOK)
            QMetaObject::invokeMethod(g_conn, []{ sendDataResult(g_conn, 0, g_lastDataReqId); }, Qt::QueuedConnection);
        return;
    }

    // 업데이트 ACK (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_UPDATE_ACK && fr->dlc >= 2) {
        const uint8_t ackIndex = fr->data[0]; // 1:Seat, 2:Mirror, 3:Wheel
        const uint8_t ackState = fr->data[1]; // 0:OK, 1:Partial
        qInfo() << "[CAN0 RX] PROFILE_UPDATE_ACK idx=" << ackIndex << "state=" << ackState;
        if (g_conn) {
            QJsonObject pay{{"index", (int)ackIndex}, {"state", (int)ackState}};
            QMetaObject::invokeMethod(g_conn, [pay]{ g_conn->send({"user/update/ack", {}, pay}); }, Qt::QueuedConnection);
        }
        return;
    }

    qInfo() << "[CAN  ? RX] Unknown     id=0x" << QString::number(fr->id,16).toUpper()
            << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
}

// ── CAN BEGIN (can0 + can1) ─────────────────────────────────────────────────
static bool startCAN(QString* errOut=nullptr) {
    // 공통 속도(예): 500k, 샘플포인트 환경에 맞게 조정
    CanConfig cfg0{0, 500000, 0.75f, 1, CAN_MODE_NORMAL};
    CanConfig cfg1{1, 500000, 0.75f, 1, CAN_MODE_NORMAL};

    if (can_init(CAN_DEVICE_LINUX) != CAN_OK) { if (errOut) *errOut = "can_init failed"; return false; }
    if (can_open("can0", cfg0) != CAN_OK)      { if (errOut) *errOut = "can_open(can0) failed"; return false; }
    if (can_open("can1", cfg1) != CAN_OK)      { if (errOut) *errOut = "can_open(can1) failed"; return false; }

    // can1: Power* 상태 수신
    static uint32_t ids_pow[] = {
        ID_POW_SEAT_STATE, ID_POW_MIRROR_STATE, ID_POW_WHEEL_STATE
    };
    CanFilter flt_pow{};
    flt_pow.type = CAN_FILTER_LIST;
    flt_pow.data.list.list  = ids_pow;
    flt_pow.data.list.count = (uint32_t)(sizeof(ids_pow)/sizeof(ids_pow[0]));
    g_canSubPowId = 0;
if (can_subscribe("can1", &g_canSubPowId, flt_pow, onCanRx, (void*)kBusCan1) != CAN_OK) {
    if (errOut) *errOut = "can_subscribe(can1) failed";
    return false;
}
    // can0: SCA/TCU 인증/프로필 수신
    static uint32_t ids_sca[] = {
        ID_SCA_DCU_AUTH_STATE, ID_SCA_DCU_AUTH_RESULT, ID_SCA_DCU_AUTH_RESULT_ADD,
        ID_TCU_DCU_USER_PROFILE_SEAT, ID_TCU_DCU_USER_PROFILE_MIRROR, ID_TCU_DCU_USER_PROFILE_WHEEL,
        ID_TCU_DCU_USER_PROFILE_UPDATE_ACK,0x100,
    };
    CanFilter flt_sca{};
    flt_sca.type = CAN_FILTER_LIST;
    flt_sca.data.list.list  = ids_sca;
    flt_sca.data.list.count = (uint32_t)(sizeof(ids_sca)/sizeof(ids_sca[0]));
    g_canSubScaId = 0;
if (can_subscribe("can0", &g_canSubScaId, flt_sca, onCanRx, (void*)kBusCan0) != CAN_OK) {
    if (errOut) *errOut = "can_subscribe(can0) failed";
    return false;
}

    qInfo() << "[can] started: can0(SCA/TCU) + can1(POWER)";
    return true;
}

// ── IPC ──────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // CAN 시작
    QString canErr;
    if (!startCAN(&canErr)) {
        qCritical() << "[can]" << canErr;
        return 1;
    }

    QLocalServer::removeServer(kSock);
    QFile::remove(kSock);

    ::umask(0);

    // IPC 서버
    IpcServer server(kSock);

#ifdef HAS_IPCSERVER_SET_SOCKET_OPTIONS
    server.setSocketOptions(QLocalServer::WorldAccessOption);
#endif

    QString err;
    if (!server.listen(&err)) {
        qCritical() << "listen failed:" << err;
        return 1;
    }

#ifdef __linux__
    ::chmod(kSock.toLocal8Bit().constData(), 0777);
#endif

    // UI → 인증 트리거
    server.addHandler("auth/result", [](const IpcMessage& m, IpcConnection* c) {
        g_lastAuthReqId = m.reqId;
        CAN_Tx_USER_FACE_REQ(); // can0
        sendAuthProcess(c, QStringLiteral("얼굴 인식 요청 전송 (CAN0)..."), g_lastAuthReqId);
    });

    // UI → 프로필 요청 트리거
    server.addHandler("data/result", [](const IpcMessage& m, IpcConnection* c) {
        g_lastDataReqId = m.reqId;
        CAN_Tx_USER_PROFILE_REQ(); // can0
        sendAuthProcess(c, QStringLiteral("프로필 요청 전송 (CAN0)..."), g_lastDataReqId);
    });

    server.addHandler("system/hello", [](const IpcMessage& m, IpcConnection* c){
        sendSystemStart(c, m.reqId);
    });

    server.addHandler("connect", [](const IpcMessage& m, IpcConnection* c){
        c->send({"system/start", m.reqId, QJsonObject{
            {"ok", true},
            {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
        }});
    });

    // UI → 현재 차량 제어 적용 (Power* 오더는 can1)
    server.addHandler("power/apply", [](const IpcMessage& m, IpcConnection* c) {
        const QJsonObject& p = m.payload;
        if (p.contains("seatPosition"))     m_seatPosition = clampInt(p.value("seatPosition").toInt(), 0, 100);
        if (p.contains("seatAngle"))        m_seatAngle = clampInt(p.value("seatAngle").toInt(), 0, 180);
        if (p.contains("seatFrontHeight"))  m_seatFrontHeight = clampInt(p.value("seatFrontHeight").toInt(), 0, 100);
        if (p.contains("seatRearHeight"))   m_seatRearHeight = clampInt(p.value("seatRearHeight").toInt(), 0, 100);

        if (p.contains("sideMirrorLeftYaw"))    m_sideMirrorLeftYaw = clampInt(p.value("sideMirrorLeftYaw").toInt(), -45, 45);
        if (p.contains("sideMirrorLeftPitch"))  m_sideMirrorLeftPitch = clampInt(p.value("sideMirrorLeftPitch").toInt(), -45, 45);
        if (p.contains("sideMirrorRightYaw"))   m_sideMirrorRightYaw = clampInt(p.value("sideMirrorRightYaw").toInt(), -45, 45);
        if (p.contains("sideMirrorRightPitch")) m_sideMirrorRightPitch = clampInt(p.value("sideMirrorRightPitch").toInt(), -45, 45);
        if (p.contains("roomMirrorYaw"))        m_roomMirrorYaw = clampInt(p.value("roomMirrorYaw").toInt(), -45, 45);
        if (p.contains("roomMirrorPitch"))      m_roomMirrorPitch = clampInt(p.value("roomMirrorPitch").toInt(), -45, 45);

        if (p.contains("handlePosition"))   m_handlePosition = clampInt(p.value("handlePosition").toInt(), 0, 100);
        if (p.contains("handleAngle"))      m_handleAngle = clampInt(p.value("handleAngle").toInt(), -90, 90);

        // → Power* 오더는 can1
        CAN_Tx_SEAT_ORDER();
        CAN_Tx_MIRROR_ORDER();
        CAN_Tx_WHEEL_ORDER();

        QTimer::singleShot(200, c, [c, reqId=m.reqId]{ sendPowerApplyAck(c, true, reqId); });
    });

    // UI → 사용자 프로필 업데이트 (TCU 쪽: can0)
    server.addHandler("user/update", [](const IpcMessage& m, IpcConnection* c){
        const QJsonObject& p = m.payload;

        // Seat
        if (p.contains("seatPosition"))     m_seatPosition = clampInt(p.value("seatPosition").toInt(), 0, 100);
        if (p.contains("seatAngle"))        m_seatAngle = clampInt(p.value("seatAngle").toInt(), 0, 180);
        if (p.contains("seatFrontHeight"))  m_seatFrontHeight = clampInt(p.value("seatFrontHeight").toInt(), 0, 100);
        if (p.contains("seatRearHeight"))   m_seatRearHeight = clampInt(p.value("seatRearHeight").toInt(), 0, 100);

        // Mirrors
        if (p.contains("sideMirrorLeftYaw"))    m_sideMirrorLeftYaw = clampInt(p.value("sideMirrorLeftYaw").toInt(), -45, 45);
        if (p.contains("sideMirrorLeftPitch"))  m_sideMirrorLeftPitch = clampInt(p.value("sideMirrorLeftPitch").toInt(), -45, 45);
        if (p.contains("sideMirrorRightYaw"))   m_sideMirrorRightYaw = clampInt(p.value("sideMirrorRightYaw").toInt(), -45, 45);
        if (p.contains("sideMirrorRightPitch")) m_sideMirrorRightPitch = clampInt(p.value("sideMirrorRightPitch").toInt(), -45, 45);
        if (p.contains("roomMirrorYaw"))        m_roomMirrorYaw = clampInt(p.value("roomMirrorYaw").toInt(), -45, 45);
        if (p.contains("roomMirrorPitch"))      m_roomMirrorPitch = clampInt(p.value("roomMirrorPitch").toInt(), -45, 45);

        // Wheel
        if (p.contains("handlePosition"))   m_handlePosition = clampInt(p.value("handlePosition").toInt(), 0, 100);
        if (p.contains("handleAngle"))      m_handleAngle = clampInt(p.value("handleAngle").toInt(), -90, 90);

        // → 프로필 업데이트는 can0
        CAN_Tx_USER_PROFILE_SEAT_UPDATE();
        CAN_Tx_USER_PROFILE_MIRROR_UPDATE();
        CAN_Tx_USER_PROFILE_WHEEL_UPDATE();

        if (c) c->send({"user/update/sent", m.reqId, QJsonObject{{"ok", true}}});
    });

    // 연결 상태 트래킹
    QObject::connect(&server, &IpcServer::clientConnected, [&](IpcConnection* c){
        g_conn = c;
        qInfo() << "[ipc] client connected";
    });
    QObject::connect(&server, &IpcServer::clientDisconnected, [&](IpcConnection* c){
        if (g_conn == c) g_conn = nullptr;
        qInfo() << "[ipc] client disconnected";
    });

    // 콘솔 키 입력
    auto notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, &app);
    QObject::connect(notifier, &QSocketNotifier::activated, [&]{
        char ch = 0; if (::read(STDIN_FILENO, &ch, 1) <= 0) return;
        if (!g_conn) { qWarning() << "No active IPC connection yet."; return; }
        if (ch=='a' || ch=='A') { sendSystemStart(g_conn, "req-a"); }
    });

    const int rc = app.exec();

    // 정리
    if (g_canSubPowId) can_unsubscribe("can1", g_canSubPowId);
    if (g_canSubScaId) can_unsubscribe("can0", g_canSubScaId);
    can_close("can1");
    can_close("can0");
    can_dispose();

    QLocalServer::removeServer(kSock);
    QFile::remove(kSock);

    return rc;
}

