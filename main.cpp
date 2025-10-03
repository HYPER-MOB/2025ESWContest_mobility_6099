#include <QtCore>
#include "ipc.h"

// POSIX 입력용
#include <sys/types.h>   // ssize_t
#include <unistd.h>      // read(), STDIN_FILENO

static const QString kSock = "/tmp/dcu.demo.sock";

static QPointer<IpcConnection> g_conn;

// Seat
int m_seatPosition = 0;      // 0~100 (%)
int m_seatAngle = 0;         // 0~180 (deg)
int m_seatFrontHeight = 0;   // 0~100 (%)
int m_seatRearHeight = 0;    // 0~100 (%)

// Side Mirror (Left/Right × Yaw/Pitch)
int m_sideMirrorLeftYaw = 0;  // -45~45 (deg)
int m_sideMirrorLeftPitch = 0;  // -45~45 (deg)
int m_sideMirrorRightYaw = 0;  // -45~45 (deg)
int m_sideMirrorRightPitch = 0;  // -45~45 (deg)

// Room Mirror
int m_roomMirrorYaw = 0;      // -45~45 (deg)
int m_roomMirrorPitch = 0;      // -45~45 (deg)

// Handle
int m_handlePosition = 0;    // 0~100 (arbitrary unit)
int m_handleAngle = 0;    // -90~90 (deg)

static void sendSystemStart(IpcConnection* c, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "system/start", reqId, QJsonObject{
        {"ok", true},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    } });
}

static void sendSystemWarning(IpcConnection* c, const QString& code, const QString& msg, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "system/warning", reqId, QJsonObject{
        {"code", code},
        {"message", msg}
    } });
}

static void sendAuthResult(IpcConnection* c, const QString& userId, double score, int error = 0, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "auth/result", reqId, QJsonObject{
        {"error", error},
        {"userId", userId},
        {"score", score}
    } });
}

static void sendDataResult(IpcConnection* c, int error = 0, const QString& reqId = {}) {
    if (!c) return;

    QJsonObject data{
        // Seat
        {"seatPosition",    m_seatPosition},
        {"seatAngle",       m_seatAngle},
        {"seatFrontHeight", m_seatFrontHeight},
        {"seatRearHeight",  m_seatRearHeight},

        // Side Mirror
        {"sideMirrorLeftYaw",    m_sideMirrorLeftYaw},
        {"sideMirrorLeftPitch",  m_sideMirrorLeftPitch},
        {"sideMirrorRightYaw",   m_sideMirrorRightYaw},
        {"sideMirrorRightPitch", m_sideMirrorRightPitch},

        // Room Mirror
        {"roomMirrorYaw",   m_roomMirrorYaw},
        {"roomMirrorPitch", m_roomMirrorPitch},

        // Handle
        {"handlePosition",  m_handlePosition},
        {"handleAngle",     m_handleAngle}
    };

    c->send({
        "data/result",
        reqId,
        QJsonObject{
            {"error", error},
            {"data",  data}
        }
        });
}


static void sendPowerApplyAck(IpcConnection* c, bool ok, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "power/apply/ack", reqId, QJsonObject{{"ok", ok}} });
}

static void sendAuthProcess(IpcConnection* c, const QString& text, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "auth/process", reqId, QJsonObject{
        {"message", text},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    }});
}

//
/*

CAN MESSAGE RECEIVE 핸들러

{
SCA_인증단계정보 
현재 Auth 단계가 무엇인지 확인
그리고 ipc로 송신해야함
}

{
SCA 결과
인증결과 정보와 사용자 ID가 옴
그것을 sendAuthResult 함수를 통해 send
}

{
Data요청 결과
이제 개인 프로필에 따른 설정값이 옴
그걸 m_ 변수들에 저장
이것을 sendDataResult를 통해서 ipc로 전달
}

BodyCAN 자동 조작 상태
{
Power/apply시 현재 어떤 상태인지 CAN으로 옴
}



*/
//


/*

BODY CAN 및 DATA 결과 포함값

Message Name    ID  DLC Signal Name	Length	Unit	Signal Description	DCU	Power Seat	Power Mirror	T-T Steering Wheel	주기/비주기	비고
DCU_SEAT_ORDER	0x001	4	sig_seat_position	8	퍼센트	목표로 하는 시트의 앞뒤 위치를 0~100 사이의 값으로 나타냅니다.	Tx	Rx			비주기
                            sig_seat_angle	8	Angle	목표로 하는 시트의 등받이 각도를 0 ~ 180 사이의 값으로 나타냅니다.
                            sig_seat_front_height	8	퍼센트	목표로 하는 시트의 앞부분 높이를 0 ~ 100 사이의 값으로 나타냅니다.
                            sig_seat_rear_height	8	퍼센트	목표로 하는 시트의 뒷부분 높이를 0 ~ 100 사이의 값으로 나타냅니다.
DCU_MIRROR_ORDER	0x002	6	sig_mirror_left_yaw	8	Angle	목표로 하는 운전석 사이드미러 좌우 회전축의 각도를 0 ~ 180 사이의 값으로 나타냅니다.	Tx		Rx		비주기
                                sig_mirror_left_pitch	8	Angle	목표로 하는 운전석 사이드미러 상하 회전축의 각도를 0 ~ 180 사이의 값으로 나타냅니다.
                                sig_mirror_right_yaw	8	Angle	목표로 하는 조수석 사이드미러 좌우 회전축의 각도를 0 ~ 180 사이의 값으로 나타냅니다.
                                sig_mirror_right_pitch	8	Angle	목표로 하는 조수석 사이드미러 상하 회전축의 각도를 0 ~ 180 사이의 값으로 나타냅니다.
                                sig_mirror_room_yaw	8	Angle	목표로 하는 룸미러 좌우 회전축의 각도를 0 ~ 180 사이의 값으로 나타냅니다.
                                sig_mirror_room_pitch	8	Angle	목표로 하는 룸미러 좌우 회전축의 각도를 0 ~ 180 사이의 값으로 나타냅니다.
DCU_WHEEL_ORDER	0x003	2	sig_wheel_position	8	퍼센트	목표로 하는 핸들의 앞뒤 위치를 0 ~ 100 사이의 값으로 나타냅니다.	Tx			Rx	비주기
                            sig_wheel_angle	8	Angle	목표로 하는 핸들의 각도를 0 ~ 180 사이의 값으로 나타냅니다.
 
 */

// ---- main ----
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    IpcServer server(kSock);
    QString err;
    if (!server.listen(&err)) {
        qCritical() << "listen failed:" << err;
        return 1;
    }

server.addHandler("auth/result", [](const IpcMessage& m, IpcConnection* c) {

    QTimer::singleShot(0, c, [m, c] {
        sendAuthProcess(c, QStringLiteral("블루투스 연결 확인 중..."), m.reqId);
        qInfo() << "[auth/process] bluetooth step sent";
    });


    QTimer::singleShot(3000, c, [m, c] {
        sendAuthProcess(c, QStringLiteral("NFC 인증 준비 중..."), m.reqId);
        qInfo() << "[auth/process] nfc step sent";
    });


    QTimer::singleShot(6000, c, [m, c] {
        sendAuthResult(c, "u123", 0.93, 0, m.reqId);
        qInfo() << "[auth/result] final reply sent after staged process";
    });

    /*
      TODO:
      - 여기에서 실제로는 DCU_USER_FACE_REQ 를 CAN으로 송신해
        Bluetooth/NFC 과정을 트리거하고,
        중간 상태를 수신할 때마다 auth/process 로 흘려보내면 됩니다.
    */
});

    server.addHandler("data/result", [](const IpcMessage& m, IpcConnection* c) {
        QTimer::singleShot(1200, c, [=] {
            sendDataResult(c, 0, m.reqId);  
            qInfo() << "[data/result] reply sent after delay";
            });
        // TODO: TCU_USER_DATA_REQ CAN 송신 로직
        });

    server.addHandler("power/apply", [](const IpcMessage& m, IpcConnection* c) {
         QTimer::singleShot(800, c, [=]{
             sendPowerApplyAck(c, true, m.reqId);
             qInfo() << "[power/apply] reply sent after delay";
         });
        /*
        여기서는 온 데이터를 BODY CAN에 적용하도록 보내는 로직 추가해야함
        */
        
        
        
        });
        
    server.addHandler("connect", [](const IpcMessage& m, IpcConnection* c) {
         });


    QObject::connect(&server, &IpcServer::clientConnected, [&](IpcConnection* c){
        g_conn = c;
        qInfo() << "[ipc] active conn set";
    });
    QObject::connect(&server, &IpcServer::clientDisconnected, [&](IpcConnection* c){
        if (g_conn == c) g_conn = nullptr;
    });

    auto notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, &app);
    QObject::connect(notifier, &QSocketNotifier::activated, [&]{
        char ch = 0;
        const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n <= 0) return;

        if (!g_conn) {
            qWarning() << "No active IPC connection yet.";
            return;
        }

        switch (ch) {
        case 'a': case 'A':
            sendSystemStart(g_conn, "req-a");
            qInfo() << "[key a] system/start sent";
            break;
        case 'd': case 'D':
            sendSystemWarning(g_conn, "W001", "졸음운전 발견!", "req-d");
            qInfo() << "[key d] system/warning sent";
            break;
        default:
            break;
        }
    });

    return app.exec();
}

