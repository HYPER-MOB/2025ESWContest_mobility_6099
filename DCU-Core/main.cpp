// main.cpp
#include <QtCore>
#include "ipc.h"

#include <sys/stat.h>
extern "C" {
#include "can_api.h"
}

#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <algorithm>
#include <functional>
#include <cstring>

// 콜백 user 구분용 라벨
static const char* kBusCan0 = "can0";
static const char* kBusCan1 = "can1";

// ── 소켓 경로 ────────────────────────────────────────────────────────────────
static const QString kSock = "/tmp/dcu.demo.sock";

// 모든 IPC 클라이언트 관리 (브로드캐스트용)
static QSet<IpcConnection*> g_clients;
static constexpr int kPowerApplyAckDelayMs = 10000; // 10초
// ── 메시지 ID (표 기준) ──────────────────────────────────────────────────────
// Body CAN (DCU -> Power*)
static constexpr uint32_t ID_DCU_SEAT_ORDER   = 0x101; // DLC 4 (→ can1 TX)
static constexpr uint32_t ID_DCU_MIRROR_ORDER = 0x102; // DLC 6 (→ can1 TX)
static constexpr uint32_t ID_DCU_WHEEL_ORDER  = 0x103; // DLC 2 (→ can1 TX)
// Power* -> DCU (20ms) (← can1 RX)
static constexpr uint32_t ID_POW_SEAT_STATE   = 0x201; // DLC 5(+flags)
static constexpr uint32_t ID_POW_MIRROR_STATE = 0x202; // DLC 7
static constexpr uint32_t ID_POW_WHEEL_STATE  = 0x203; // DLC 3

// SCA/TCU 영역 (인증/프로필) (can0 TX/RX)
static constexpr uint32_t ID_DCU_RESET                          = 0x001; // DLC 1
static constexpr uint32_t ID_DCU_RESET_ACK                      = 0x002; // DLC 2
static constexpr uint32_t ID_DCU_TCU_DRIVE_CMD                  = 0x005; // DLC 1  (drive:1, stop:0)
static constexpr uint32_t ID_DCU_SCA_USER_FACE_REQ              = 0x101; // DLC 1
static constexpr uint32_t ID_SCA_DCU_AUTH_STATE                 = 0x103; // DLC 2
static constexpr uint32_t ID_SCA_DCU_AUTH_RESULT                = 0x112; // DLC 8
static constexpr uint32_t ID_SCA_DCU_AUTH_RESULT_ADD            = 0x113; // DLC 8
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_REQ           = 0x201; // DLC 1
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_SEAT          = 0x202; // DLC 4
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_MIRROR        = 0x203; // DLC 6
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_WHEEL         = 0x204; // DLC 2
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_ACK           = 0x205; // DLC 2
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_SEAT_UPDATE   = 0x206; // DLC4
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_MIRROR_UPDATE = 0x207; // DLC6
static constexpr uint32_t ID_DCU_TCU_USER_PROFILE_WHEEL_UPDATE  = 0x208; // DLC2
static constexpr uint32_t ID_TCU_DCU_USER_PROFILE_UPDATE_ACK    = 0x209; // DLC2

static constexpr uint32_t ID_CAN_SYSTEM_WARNING = 0x003;
static constexpr uint32_t ID_CAN_SYSTEM_START   = 0x600;

// ── 버튼 메시지 상수 (주기 송신) ─────────────────────────────────────────────
static constexpr int kBtnTxPeriodMs = 50; // 20Hz 기본
static constexpr uint32_t ID_DCU_SEAT_BUTTON   = 0x301; // DLC 4
static constexpr uint32_t ID_DCU_MIRROR_BUTTON = 0x302; // DLC 6
static constexpr uint32_t ID_DCU_WHEEL_BUTTON  = 0x303; // DLC 2

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

int m_iSystemStart = 0;

// 프로필 수신 완료 체크
static bool g_profSeatOK = false;
static bool g_profMirrorOK = false;
static bool g_profWheelOK = false;

// 구독 ID (종료 시 해제용)
static int g_canSubPowId = 0; // can1 (Power*)
static int g_canSubScaId = 0; // can0 (SCA/TCU)

// ── 전역에 추가(인증/프로필/유저) ───────────────────────────────────────────
static QString g_lastAuthReqId;
static QString g_lastDataReqId;
static QByteArray g_accUserId;
static QString g_currentUserId;

// ── 버튼 상태 버퍼 및 타이머 ────────────────────────────────────────────────
// (0: none, 1: +, 2: -)
static std::array<uint8_t, 4> g_btnSeat   {{0,0,0,0}};       // pos, angle, front, rear
static std::array<uint8_t, 6> g_btnMirror {{0,0,0,0,0,0}};   // L_yaw, L_pitch, R_yaw, R_pitch, room_yaw, room_pitch
static std::array<uint8_t, 2> g_btnWheel  {{0,0}};           // pos, angle
static QTimer* g_btnTxTimer = nullptr;

// ── 유틸 ─────────────────────────────────────────────────────────────────────
static inline int clampInt(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }
static inline uint8_t encPercent100(int v) { return (uint8_t)clampInt(v, 0, 100); }
static inline uint8_t encAngle180_fromSigned(int degSigned, int offset) {
    return (uint8_t)clampInt(degSigned + offset, 0, 180);
}
static inline int decAngle180_toSigned(uint8_t raw, int offset) {
    return clampInt((int)raw - offset, -offset, 180 - offset);
}
static inline QString bytesToHex(const uint8_t* d, int n) {
    QString s;
    for (int i = 0; i < n; ++i) {
        s += QString("%1").arg(d[i], 2, 16, QLatin1Char('0')).toUpper();
        if (i+1 < n) s += ' ';
    }
    return s;
}

// 모든 클라이언트에 Queue로 함수 실행 (쓰레드/수명 안전)
static void sendToAll(const std::function<void(IpcConnection*)>& fn) {
    for (auto* c : std::as_const(g_clients)) {
        if (!c) continue;
        QMetaObject::invokeMethod(c, [c, fn]{ if (c) fn(c); }, Qt::QueuedConnection);
    }
}
static bool hasClients() { return !g_clients.isEmpty(); }

// ── IPC 송신 ────────────────────────────────────────────────────────────────
static void sendSystemStart(IpcConnection* c, const QString& reqId = {}) {
    if (!c) return;
    c->send({"system/start", reqId, QJsonObject{
        {"ok", true},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    }});
}
static void sendSystemReset(IpcConnection* c, const QString& reason = {}, const QString& reqId = {}) {
    if (!c) return;
    c->send({"system/reset", reqId, QJsonObject{
        {"reason", reason},
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
static void sendSeatState(IpcConnection* c) {
    if (!c) return;
    QJsonObject data{
        {"seatPosition", m_seatPosition},
        {"seatAngle", m_seatAngle},
        {"seatFrontHeight", m_seatFrontHeight},
        {"seatRearHeight", m_seatRearHeight}
    };
    c->send({"state/seat", {}, data});
}
static void sendMirrorState(IpcConnection* c) {
    if (!c) return;
    QJsonObject data{
        {"sideMirrorLeftYaw", m_sideMirrorLeftYaw},
        {"sideMirrorLeftPitch", m_sideMirrorLeftPitch},
        {"sideMirrorRightYaw", m_sideMirrorRightYaw},
        {"sideMirrorRightPitch", m_sideMirrorRightPitch},
        {"roomMirrorYaw", m_roomMirrorYaw},
        {"roomMirrorPitch", m_roomMirrorPitch}
    };
    c->send({"state/mirror", {}, data});
}
static void sendWheelState(IpcConnection* c) {
    if (!c) return;
    QJsonObject data{
        {"handlePosition", m_handlePosition},
        {"handleAngle", m_handleAngle}
    };
    c->send({"state/wheel", {}, data});
}

// ── CAN TX (공통) ───────────────────────────────────────────────────────────
static inline CanFrame mkFrame(uint32_t id, uint8_t dlc) {
    CanFrame f{}; f.id = id; f.dlc = dlc; f.flags = 0; std::memset(f.data, 0, 8); return f;
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

// → 버튼 상태 주기 송신 (can1)
static void CAN_Tx_SEAT_BUTTONS() {
    CanFrame f = mkFrame(ID_DCU_SEAT_BUTTON, 4);
    for (int i=0;i<4;++i) f.data[i] = g_btnSeat[i];
    qInfo() << "[CAN1 TX] SEAT_BUTTON id=0x" << QString::number(f.id,16).toUpper()
            << " dlc=" << f.dlc << " data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can1", f, 0);
}
static void CAN_Tx_MIRROR_BUTTONS() {
    CanFrame f = mkFrame(ID_DCU_MIRROR_BUTTON, 6);
    for (int i=0;i<6;++i) f.data[i] = g_btnMirror[i];
    qInfo() << "[CAN1 TX] MIRROR_BUTTON id=0x" << QString::number(f.id,16).toUpper()
            << " dlc=" << f.dlc << " data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can1", f, 0);
}
static void CAN_Tx_WHEEL_BUTTONS() {
    CanFrame f = mkFrame(ID_DCU_WHEEL_BUTTON, 2);
    for (int i=0;i<2;++i) f.data[i] = g_btnWheel[i];
    qInfo() << "[CAN1 TX] WHEEL_BUTTON id=0x" << QString::number(f.id,16).toUpper()
            << " dlc=" << f.dlc << " data=[" << bytesToHex(f.data, f.dlc) << "]";
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

// ── CAN TX: RESET ───────────────────────────────────────────────────────────
static void CAN_Tx_RESET_on(const char* bus) {
    CanFrame f = mkFrame(ID_DCU_RESET, 1);
    f.data[0] = 1; // 트리거
    qInfo() << "[" << bus << " TX] SYSTEM_RESET id=0x"
            << QString::number(f.id,16).toUpper()
            << "dlc=" << f.dlc << "data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send(bus, f, 0);
}
static void CAN_Tx_RESET_BOTH() {
    CAN_Tx_RESET_on("can0");
    CAN_Tx_RESET_on("can1");
}

// → SCA/TCU 로 가는 주행 상태 명령 (can0)
static void CAN_Tx_DRIVE_CMD(uint8_t v /* 1:drive, 0:stop */) {
    CanFrame f = mkFrame(ID_DCU_TCU_DRIVE_CMD, 1);
    f.data[0] = v;
    qInfo() << "[CAN0 TX] DRIVE_CMD id=0x" << QString::number(f.id,16).toUpper()
            << " dlc=" << f.dlc << " data=[" << bytesToHex(f.data, f.dlc) << "]";
    can_send("can0", f, 0);
}

// ── CAN RX (버스 구분은 ID로 충분하여 공용 콜백 사용) ───────────────────────
static void onCanRx(const CanFrame* fr, void* user) {
    const char* bus = (const char*)user;

    // can0에서 id 0x100 디버그 출력
    if (bus && strcmp(bus, "can0") == 0 && fr->id == 0x100) {
        qInfo() << "[CAN0 RX] DEBUG id=0x"
                << QString::number(fr->id, 16).toUpper()
                << "dlc=" << fr->dlc
                << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        return;
    }

    // 좌석 상태 (can1)
    if (strcmp(bus, "can1") == 0 && fr->id == ID_POW_SEAT_STATE && fr->dlc >= 4) {
        qInfo() << "[CAN1 RX] SEAT_STATE  id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_seatPosition    = fr->data[0];
        m_seatAngle       = fr->data[1];
        m_seatFrontHeight = fr->data[2];
        m_seatRearHeight  = fr->data[3];

        // 5번째 바이트로 system/start 트리거
        if (fr->dlc >= 5) {
            const uint8_t flagByte = fr->data[4];
            if (flagByte == 0x01 && m_iSystemStart == 0) {
                m_iSystemStart = 1;
                if (hasClients()) {
                    sendToAll([](IpcConnection* c){ sendSystemStart(c, {}); });
                }
            }
        }

        if (hasClients()) sendToAll([](IpcConnection* c){ sendSeatState(c); });
        return;
    }

    // 미러 상태 (can1)
    if (strcmp(bus, "can1") == 0 && fr->id == ID_POW_MIRROR_STATE && fr->dlc >= 6) {
        qInfo() << "[CAN1 RX] MIRROR_STATE id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_sideMirrorLeftYaw    = decAngle180_toSigned(fr->data[0], 90);
        m_sideMirrorLeftPitch  = decAngle180_toSigned(fr->data[1], 90);
        m_sideMirrorRightYaw   = decAngle180_toSigned(fr->data[2], 90);
        m_sideMirrorRightPitch = decAngle180_toSigned(fr->data[3], 90);
        m_roomMirrorYaw        = decAngle180_toSigned(fr->data[4], 90);
        m_roomMirrorPitch      = decAngle180_toSigned(fr->data[5], 90);

        if (hasClients()) sendToAll([](IpcConnection* c){ sendMirrorState(c); });
        return;
    }

    // 핸들 상태 (can1)
    if (strcmp(bus, "can1") == 0 && fr->id == ID_POW_WHEEL_STATE && fr->dlc >= 2) {
        qInfo() << "[CAN1 RX] WHEEL_STATE id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_handlePosition = fr->data[0];
        m_handleAngle    = decAngle180_toSigned(fr->data[1], 90);

        if (hasClients()) sendToAll([](IpcConnection* c){ sendWheelState(c); });
        return;
    }

    // 인증 단계 상태 (can0)
if (fr->id == ID_SCA_DCU_AUTH_STATE && fr->dlc >= 2) {
    qInfo() << "[CAN0 RX] AUTH_STATE  id=0x" << QString::number(fr->id,16).toUpper()
            << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";

    const uint8_t step  = fr->data[0];
    const uint8_t state = fr->data[1];

    if (hasClients()) {
        QString msg;

        // step 값에 따라 메시지 분기
        switch (step) {
        case 1:
            msg = "NFC인증 중입니다..";
            break;
        case 2:
            msg = "Bluetooth 인증중입니다..";
            break;
        case 3:
            msg = "얼굴 인증중입니다..";
            break;
        default:
            msg = QString("Auth step=%1 state=%2").arg(step).arg(state);
            break;
        }

        sendToAll([msg](IpcConnection* c){
            sendAuthProcess(c, msg);
        });
    }

    return;
}
    // 인증 결과 (can0)
    if (fr->id == ID_SCA_DCU_AUTH_RESULT && fr->dlc >= 1) {
        qInfo() << "[CAN0 RX] AUTH_RESULT id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";

        const uint8_t flag = fr->data[0];   // 0x01: fail, 그 외: ok
        g_accUserId.clear(); // 새 결과 시작이므로 버퍼 리셋

        if (flag == 0x01) {
            g_currentUserId.clear();
            qInfo() << "[auth] result FAIL (flag=0x01)";
            if (hasClients()) {
                sendToAll([](IpcConnection* c){
                    sendAuthResult(c, QString(), 0.0, /*error=*/1, g_lastAuthReqId);
                });
            }
            return;
        }

        if (fr->dlc > 1) {
            g_accUserId.append(reinterpret_cast<const char*>(fr->data + 1),
                               fr->dlc - 1);
        }
        const QString uid = QString::fromLatin1(g_accUserId);
        g_currentUserId = uid;
        qInfo() << "[auth] result OK  userId=" << (uid.isEmpty() ? "<partial/empty>" : uid);

        if (hasClients()) {
            const QString uidCopy = uid;
            sendToAll([uidCopy](IpcConnection* c){
                sendAuthResult(c, uidCopy, 0.95, 0, g_lastAuthReqId);
            });
        }
        return;
    }

    // 인증 결과 추가 (can0)
    if (fr->id == ID_SCA_DCU_AUTH_RESULT_ADD && fr->dlc >= 1) {
        qInfo() << "[CAN0 RX] AUTH_RESULT_ADD id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        g_accUserId.append((const char*)fr->data, std::min<int>(fr->dlc, 8));
        if (hasClients()) {
            const QString uid = QString::fromLatin1(g_accUserId);
            sendToAll([uid](IpcConnection* c){ sendAuthResult(c, uid, 0.95, 0, g_lastAuthReqId); });
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
        if (hasClients() && g_profSeatOK && g_profMirrorOK && g_profWheelOK)
            sendToAll([](IpcConnection* c){ sendDataResult(c, 0, g_lastDataReqId); });
        return;
    }

    // 프로필 Mirror (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_MIRROR && fr->dlc >= 6) {
        qInfo() << "[CAN0 RX] PROFILE_MIRROR id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
		m_sideMirrorLeftYaw    = clampInt(fr->data[0], 0, 180);
		m_sideMirrorLeftPitch  = clampInt(fr->data[1], 0, 180);
		m_sideMirrorRightYaw   = clampInt(fr->data[2], 0, 180);
		m_sideMirrorRightPitch = clampInt(fr->data[3], 0, 180);
		m_roomMirrorYaw        = clampInt(fr->data[4], 0, 180);
		m_roomMirrorPitch      = clampInt(fr->data[5], 0, 180);	
        g_profMirrorOK = true;
        if (hasClients() && g_profSeatOK && g_profMirrorOK && g_profWheelOK)
            sendToAll([](IpcConnection* c){ sendDataResult(c, 0, g_lastDataReqId); });
        return;
    }

    // 프로필 Wheel (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_WHEEL && fr->dlc >= 2) {
        qInfo() << "[CAN0 RX] PROFILE_WHEEL id=0x" << QString::number(fr->id,16).toUpper()
                << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
        m_handlePosition = fr->data[0];
        m_handleAngle    = decAngle180_toSigned(fr->data[1], 90);
        g_profWheelOK = true;
        if (hasClients() && g_profSeatOK && g_profMirrorOK && g_profWheelOK)
            sendToAll([](IpcConnection* c){ sendDataResult(c, 0, g_lastDataReqId); });
        return;
    }

    // 업데이트 ACK (can0)
    if (fr->id == ID_TCU_DCU_USER_PROFILE_UPDATE_ACK && fr->dlc >= 2) {
        const uint8_t ackIndex = fr->data[0]; // 1:Seat, 2:Mirror, 3:Wheel
        const uint8_t ackState = fr->data[1]; // 0:OK, 1:Partial
        qInfo() << "[CAN0 RX] PROFILE_UPDATE_ACK idx=" << ackIndex << "state=" << ackState;
        if (hasClients()) {
            QJsonObject pay{{"index", (int)ackIndex}, {"state", (int)ackState}};
            sendToAll([pay](IpcConnection* c){ c->send({"user/update/ack", {}, pay}); });
        }
        return;
    }

    // 외부 경고 (can0)
    if (fr->id == ID_CAN_SYSTEM_WARNING && fr->dlc >= 1) {
        if (hasClients()) {
            const QString code = QStringLiteral("CAN_WARN_0x500");
            const QString msg  = QStringLiteral("졸음 운전 감지!!")
                                    .arg(bus ? bus : "?");
            sendToAll([code, msg](IpcConnection* c){ sendSystemWarning(c, code, msg); });
        }
        return;
    }

    // 외부 start (can0)
    if (fr->id == ID_CAN_SYSTEM_START && fr->dlc >= 1) {
        if (hasClients()) {
            sendToAll([](IpcConnection* c){ sendSystemStart(c, {}); });
        }
        return;
    }

    qInfo() << "[CAN  ? RX] Unknown     id=0x" << QString::number(fr->id,16).toUpper()
            << "dlc=" << fr->dlc << "data=[" << bytesToHex(fr->data, fr->dlc) << "]";
}

// ── CAN BEGIN (can0 + can1) ─────────────────────────────────────────────────
static bool startCAN(QString* errOut=nullptr) {
    // 예: 500k, 샘플포인트는 환경에 맞게
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

    // can0: SCA/TCU 인증/프로필/디버그/경고 수신
    static uint32_t ids_sca[] = {
        ID_SCA_DCU_AUTH_STATE, ID_SCA_DCU_AUTH_RESULT, ID_SCA_DCU_AUTH_RESULT_ADD,
        ID_TCU_DCU_USER_PROFILE_SEAT, ID_TCU_DCU_USER_PROFILE_MIRROR, ID_TCU_DCU_USER_PROFILE_WHEEL,
        ID_TCU_DCU_USER_PROFILE_UPDATE_ACK, 0x100, ID_CAN_SYSTEM_WARNING, ID_CAN_SYSTEM_START,
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

// ── main ────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // CAN 시작
    QString canErr;
    if (!startCAN(&canErr)) {
        qCritical() << "[can]" << canErr;
        return 1;
    }

    // 버튼 주기 송신 타이머 시작 (항상 현재 상태 전송; 누르지 않으면 0 유지)
    g_btnTxTimer = new QTimer(&app);
    g_btnTxTimer->setTimerType(Qt::PreciseTimer);
    g_btnTxTimer->setInterval(kBtnTxPeriodMs);
    QObject::connect(g_btnTxTimer, &QTimer::timeout, [&]{
        CAN_Tx_SEAT_BUTTONS();
        CAN_Tx_MIRROR_BUTTONS();
        CAN_Tx_WHEEL_BUTTONS();
    });
    g_btnTxTimer->start();

    // IPC 서버 준비
    QLocalServer::removeServer(kSock);
    QFile::remove(kSock);
    ::umask(0);

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

    // UI → 프로필 요청
    server.addHandler("data/result", [](const IpcMessage& m, IpcConnection* c) {
        g_lastDataReqId = m.reqId;
        const QString uidForLog = g_currentUserId.isEmpty() ? QStringLiteral("<none>") : g_currentUserId;
        qInfo() << "[profile] request sent  reqId=" << g_lastDataReqId
                << " userId=" << uidForLog;
        CAN_Tx_USER_PROFILE_REQ(); // can0
        sendAuthProcess(c, QStringLiteral("프로필 요청 전송 (CAN0)..."), g_lastDataReqId);
    });

    // 핸드셰이크
    server.addHandler("system/hello", [](const IpcMessage& m, IpcConnection* c){
        // 필요시 start 이벤트 단건 응답 가능
        // sendSystemStart(c, m.reqId);
    });
    server.addHandler("connect", [](const IpcMessage& m, IpcConnection* c){
        c->send({"system/start", m.reqId, QJsonObject{
            {"ok", true},
            {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
        }});
    });

    // UI → Power 즉시적용 (can1)
    server.addHandler("power/apply", [](const IpcMessage& m, IpcConnection* c) {
        const QJsonObject& p = m.payload;
        bool seatChanged=false, mirrorChanged=false, wheelChanged=false;

        // Seat
        if (p.contains("seatPosition"))    { m_seatPosition     = clampInt(p.value("seatPosition").toInt(), 0, 100); seatChanged = true; }
        if (p.contains("seatAngle"))       { m_seatAngle        = clampInt(p.value("seatAngle").toInt(), 0, 180);    seatChanged = true; }
        if (p.contains("seatFrontHeight")) { m_seatFrontHeight  = clampInt(p.value("seatFrontHeight").toInt(), 0, 100); seatChanged = true; }
        if (p.contains("seatRearHeight"))  { m_seatRearHeight   = clampInt(p.value("seatRearHeight").toInt(), 0, 100); seatChanged = true; }

        // Mirrors
        if (p.contains("sideMirrorLeftYaw"))    { m_sideMirrorLeftYaw    = clampInt(p.value("sideMirrorLeftYaw").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("sideMirrorLeftPitch"))  { m_sideMirrorLeftPitch  = clampInt(p.value("sideMirrorLeftPitch").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("sideMirrorRightYaw"))   { m_sideMirrorRightYaw   = clampInt(p.value("sideMirrorRightYaw").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("sideMirrorRightPitch")) { m_sideMirrorRightPitch = clampInt(p.value("sideMirrorRightPitch").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("roomMirrorYaw"))        { m_roomMirrorYaw        = clampInt(p.value("roomMirrorYaw").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("roomMirrorPitch"))      { m_roomMirrorPitch      = clampInt(p.value("roomMirrorPitch").toInt(), 0, 180); mirrorChanged = true; }

        // Wheel
        if (p.contains("handlePosition"))  { m_handlePosition = clampInt(p.value("handlePosition").toInt(), 0, 100); wheelChanged = true; }
        if (p.contains("handleAngle"))     { m_handleAngle    = clampInt(p.value("handleAngle").toInt(), -90, 90);  wheelChanged = true; }

        if (seatChanged)   CAN_Tx_SEAT_ORDER();
        if (mirrorChanged) CAN_Tx_MIRROR_ORDER();
        if (wheelChanged)  CAN_Tx_WHEEL_ORDER();

        // 브로드캐스트 상태
        if (seatChanged)   sendToAll([](IpcConnection* cc){ sendSeatState(cc); });
        if (mirrorChanged) sendToAll([](IpcConnection* cc){ sendMirrorState(cc); });
        if (wheelChanged)  sendToAll([](IpcConnection* cc){ sendWheelState(cc); });
        


	if (c) {
	    QPointer<IpcConnection> pc = c;   
	    const QString req = m.reqId;

	    QTimer::singleShot(kPowerApplyAckDelayMs, qApp, [pc, req]{
		if (!pc) return; 
		pc->send({"power/apply/ack", req, QJsonObject{{"ok", true}}});
	    });
	}
    });

    // UI → 사용자 프로필 업데이트 (can0)
    server.addHandler("user/update", [](const IpcMessage& m, IpcConnection* c){
        const QJsonObject& p = m.payload;
        bool seatChanged=false, mirrorChanged=false, wheelChanged=false;

        // Seat
        if (p.contains("seatPosition"))    { m_seatPosition     = clampInt(p.value("seatPosition").toInt(), 0, 100); seatChanged = true; }
        if (p.contains("seatAngle"))       { m_seatAngle        = clampInt(p.value("seatAngle").toInt(), 0, 180);    seatChanged = true; }
        if (p.contains("seatFrontHeight")) { m_seatFrontHeight  = clampInt(p.value("seatFrontHeight").toInt(), 0, 100); seatChanged = true; }
        if (p.contains("seatRearHeight"))  { m_seatRearHeight   = clampInt(p.value("seatRearHeight").toInt(), 0, 100); seatChanged = true; }

        // Mirrors
        if (p.contains("sideMirrorLeftYaw"))    { m_sideMirrorLeftYaw    = clampInt(p.value("sideMirrorLeftYaw").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("sideMirrorLeftPitch"))  { m_sideMirrorLeftPitch  = clampInt(p.value("sideMirrorLeftPitch").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("sideMirrorRightYaw"))   { m_sideMirrorRightYaw   = clampInt(p.value("sideMirrorRightYaw").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("sideMirrorRightPitch")) { m_sideMirrorRightPitch = clampInt(p.value("sideMirrorRightPitch").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("roomMirrorYaw"))        { m_roomMirrorYaw        = clampInt(p.value("roomMirrorYaw").toInt(), 0, 180); mirrorChanged = true; }
        if (p.contains("roomMirrorPitch"))      { m_roomMirrorPitch      = clampInt(p.value("roomMirrorPitch").toInt(), 0, 180); mirrorChanged = true; }

        // Wheel
        if (p.contains("handlePosition"))  { m_handlePosition = clampInt(p.value("handlePosition").toInt(), 0, 100); wheelChanged = true; }
        if (p.contains("handleAngle"))     { m_handleAngle    = clampInt(p.value("handleAngle").toInt(), -90, 90);  wheelChanged = true; }

        if (seatChanged)   CAN_Tx_USER_PROFILE_SEAT_UPDATE();
        if (mirrorChanged) CAN_Tx_USER_PROFILE_MIRROR_UPDATE();
        if (wheelChanged)  CAN_Tx_USER_PROFILE_WHEEL_UPDATE();

        if (c) c->send({"user/update/sent", m.reqId, QJsonObject{{"ok", true}}});

        if (seatChanged)   sendToAll([](IpcConnection* cc){ sendSeatState(cc); });
        if (mirrorChanged) sendToAll([](IpcConnection* cc){ sendMirrorState(cc); });
        if (wheelChanged)  sendToAll([](IpcConnection* cc){ sendWheelState(cc); });
    });

    // UI → 주행 상태 전환 (drive/stop)
    server.addHandler("tcu/drive", [](const IpcMessage& m, IpcConnection* /*c*/) {
        const QJsonObject& p = m.payload;
        const QString stateStr = p.value("state").toString().toLower();
        const bool startFlagFromBool = p.contains("start") ? p.value("start").toBool(false) : false;

        int v = 0;  // 0: stop, 1: drive
        if (stateStr == "drive" || stateStr == "start")      v = 1;
        else if (stateStr == "stop")                          v = 0;
        else if (!p.contains("state"))                        v = startFlagFromBool ? 1 : 0;
        else { qWarning() << "[tcu/drive] unknown state =" << stateStr; return; }

        CAN_Tx_DRIVE_CMD(static_cast<uint8_t>(v));
    });

    // UI → 버튼 플래그 설정 (0/1/2) : 좌석
    server.addHandler("button/seat", [](const IpcMessage& m, IpcConnection* c){
        const auto& p = m.payload;
        auto clampFlag = [](int v){ return std::clamp(v, 0, 2); };

        if (p.contains("position")) g_btnSeat[0] = (uint8_t)clampFlag(p.value("position").toInt());
        if (p.contains("angle"))    g_btnSeat[1] = (uint8_t)clampFlag(p.value("angle").toInt());
        if (p.contains("front"))    g_btnSeat[2] = (uint8_t)clampFlag(p.value("front").toInt());
        if (p.contains("rear"))     g_btnSeat[3] = (uint8_t)clampFlag(p.value("rear").toInt());

        if (c) c->send({"button/seat/ack", m.reqId, QJsonObject{{"ok", true}}});
    });

    // UI → 버튼 플래그 설정 (0/1/2) : 미러
    server.addHandler("button/mirror", [](const IpcMessage& m, IpcConnection* c){
        const auto& p = m.payload;
        auto clampFlag = [](int v){ return std::clamp(v, 0, 2); };

        if (p.contains("leftYaw"))     g_btnMirror[0] = (uint8_t)clampFlag(p.value("leftYaw").toInt());
        if (p.contains("leftPitch"))   g_btnMirror[1] = (uint8_t)clampFlag(p.value("leftPitch").toInt());
        if (p.contains("rightYaw"))    g_btnMirror[2] = (uint8_t)clampFlag(p.value("rightYaw").toInt());
        if (p.contains("rightPitch"))  g_btnMirror[3] = (uint8_t)clampFlag(p.value("rightPitch").toInt());
        if (p.contains("roomYaw"))     g_btnMirror[4] = (uint8_t)clampFlag(p.value("roomYaw").toInt());
        if (p.contains("roomPitch"))   g_btnMirror[5] = (uint8_t)clampFlag(p.value("roomPitch").toInt());

        if (c) c->send({"button/mirror/ack", m.reqId, QJsonObject{{"ok", true}}});
    });

    // UI → 버튼 플래그 설정 (0/1/2) : 휠
    server.addHandler("button/wheel", [](const IpcMessage& m, IpcConnection* c){
        const auto& p = m.payload;
        auto clampFlag = [](int v){ return std::clamp(v, 0, 2); };

        if (p.contains("position")) g_btnWheel[0] = (uint8_t)clampFlag(p.value("position").toInt());
        if (p.contains("angle"))    g_btnWheel[1] = (uint8_t)clampFlag(p.value("angle").toInt());

        if (c) c->send({"button/wheel/ack", m.reqId, QJsonObject{{"ok", true}}});
    });

    // 연결 상태 트래킹
    QObject::connect(&server, &IpcServer::clientConnected, [&](IpcConnection* c){
        if (!c) return;
        g_clients.insert(c);
        qInfo() << "[ipc] client connected, total =" << g_clients.size();
        QObject::connect(c, &QObject::destroyed, &app, [c]{
            g_clients.remove(c);
            qInfo() << "[ipc] client destroyed, total =" << g_clients.size();
        });
    });
    QObject::connect(&server, &IpcServer::clientDisconnected, [&](IpcConnection* c){
        g_clients.remove(c);
        qInfo() << "[ipc] client disconnected, total =" << g_clients.size();
    });

    // 콘솔 키 입력 (A: start 알림, B: reset 브로드캐스트 + CAN reset)
    auto notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, &app);
    QObject::connect(notifier, &QSocketNotifier::activated, [&]{
        char ch = 0; if (::read(STDIN_FILENO, &ch, 1) <= 0) return;
        if (!hasClients()) { qWarning() << "No active IPC connection yet."; return; }
        if (ch=='a' || ch=='A') {
            sendToAll([](IpcConnection* c){ sendSystemStart(c, "req-a"); });
        } else if (ch=='b' || ch=='B') {
            sendToAll([](IpcConnection* c){ sendSystemReset(c, "manual-keypress"); });
            m_iSystemStart = 0;
            CAN_Tx_RESET_BOTH();
        }
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

