#include <QtCore>
#include "ipc.h"

static const QString kSock = "/tmp/dcu.demo.sock";

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    IpcServer server(kSock);
    QString err;
    if (!server.listen(&err)) {
        qCritical() << "listen failed:" << err;
        return 1;
    }

    server.addHandler("system/start", [](const IpcMessage& m, IpcConnection* c){
        qInfo() << "[system/start]" << m.payload;
        c->send(IpcMessage{
            .topic = "system/started",
            .reqId = m.reqId,
            .payload = QJsonObject{{"ok", true}}
        });
    });

server.addHandler("auth/result", [](const IpcMessage& m, IpcConnection* c){
    QTimer::singleShot(2000, c, [m, c] {
        QJsonObject payload{
            {"error", 0},            
            {"userId", "u123"},
            {"score", 0.93}
        };
        c->send(IpcMessage{
            .topic = "auth/result",
            .reqId = m.reqId,
            .payload = payload
        });
        qInfo() << "[auth/result] reply sent after delay";
    });
});

server.addHandler("data/result", [](const IpcMessage& m, IpcConnection* c){
    QJsonObject resp{
        {"error", 0},
        {"data", QJsonObject{
            {"sidemirror", 10},
            {"seatTilt", 30}
        }}
    };
    qInfo() << "[data/result] reply sent after delay";
    QTimer::singleShot(1200, c, [=]{ c->send({ "data/result", m.reqId, resp }); });
});

server.addHandler("power/apply", [](const IpcMessage& m, IpcConnection* c){
    bool ok = true; 
qInfo() << "[power/apply] reply sent after delay";
    QTimer::singleShot(800, c, [=]{
        c->send({ "power/apply/ack", m.reqId, QJsonObject{{"ok", ok}} });
    });
});
    server.addHandler("system/warning", [](const IpcMessage& m, IpcConnection*){
        qWarning() << "[system/warning]" << m.payload;
    });

    return app.exec();
}

