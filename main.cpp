#include <QtCore>
#include "ipc.h"

static const QString kSock = "/tmp/dcu.demo.sock";

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

static void sendDataResult(IpcConnection* c, int sidemirror, int seatTilt, int error = 0, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "data/result", reqId, QJsonObject{
        {"error", error},
        {"data", QJsonObject{
            {"sidemirror", sidemirror},
            {"seatTilt", seatTilt}
        }}
    } });
}

static void sendPowerApplyAck(IpcConnection* c, bool ok, const QString& reqId = {}) {
    if (!c) return;
    c->send({ "power/apply/ack", reqId, QJsonObject{{"ok", ok}} });
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    IpcServer server(kSock);
    QString err;
    if (!server.listen(&err)) {
        qCritical() << "listen failed:" << err;
        return 1;
    }

    server.addHandler("auth/result", [](const IpcMessage& m, IpcConnection* c) {
        // QTimer::singleShot(2000, c, [m, c] {
        //     sendAuthResult(c, "u123", 0.93, 0, m.reqId);
        //     qInfo() << "[auth/result] reply sent after delay";
        // });
        });

    server.addHandler("data/result", [](const IpcMessage& m, IpcConnection* c) {
        // QTimer::singleShot(1200, c, [=]{
        //     sendDataResult(c, 10, 30, 0, m.reqId);
        //     qInfo() << "[data/result] reply sent after delay";
        // });
        });

    server.addHandler("power/apply", [](const IpcMessage& m, IpcConnection* c) {
        // QTimer::singleShot(800, c, [=]{
        //     sendPowerApplyAck(c, true, m.reqId);
        //     qInfo() << "[power/apply] reply sent after delay";
        // });
        });


    return app.exec();
}
