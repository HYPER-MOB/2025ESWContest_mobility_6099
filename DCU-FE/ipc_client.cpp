#include "ipc_client.h"

static QString newReqId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

static bool unpackJsonFrames(QByteArray& buf, QList<QJsonObject>& outFrames) {
    int offset = 0;
    while (true) {
        if (buf.size() - offset < 4) break;
        quint32 lenLE;
        memcpy(&lenLE, buf.constData() + offset, 4);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        quint32 len = qFromLittleEndian(lenLE);
#else
        quint32 len = lenLE;
#endif
        if (buf.size() - offset - 4 < (int)len) break;
        QByteArray body = buf.mid(offset + 4, len);
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            outFrames.push_back(doc.object());
        }
        offset += 4 + (int)len;
    }
    if (offset > 0) buf.remove(0, offset);
    return !outFrames.isEmpty();
}

IpcClient::IpcClient(const QString& socketPath, QObject* parent)
    : QObject(parent), m_path(socketPath), m_sock(new QLocalSocket(this)) {
    connect(m_sock, &QLocalSocket::readyRead, this, &IpcClient::onReadyRead);
    connect(m_sock, &QLocalSocket::connected,  this, &IpcClient::onConnected);
    connect(m_sock, &QLocalSocket::disconnected, this, &IpcClient::onDisconnected);
}

void IpcClient::connectIfNeeded() {
    if (m_sock->state() == QLocalSocket::ConnectedState ||
        m_sock->state() == QLocalSocket::ConnectingState) return;
    m_sock->connectToServer(m_path);
}

QByteArray IpcClient::packJson(const QJsonObject& obj) {
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray out;
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 len = (quint32)body.size();
    ds << len;
    out.append(body);
    return out;
}

QString IpcClient::send(const QString& topic, const QJsonObject& payload, const QString& reqId) {
    connectIfNeeded();
    QString rid = reqId.isEmpty() ? newReqId() : reqId;
    QJsonObject obj{{"topic", topic}, {"payload", payload}, {"reqId", rid}};
    m_sock->write(packJson(obj));
    m_sock->flush();
    return rid;
}

void IpcClient::onReadyRead() {
    m_buf.append(m_sock->readAll());
    processBuffer();
}

void IpcClient::processBuffer() {
    QList<QJsonObject> frames;
    if (!unpackJsonFrames(m_buf, frames)) return;
    for (const auto& obj : frames) {
        IpcMessage m;
        m.topic = obj.value("topic").toString();
        m.reqId = obj.value("reqId").toString();
        m.payload = obj.value("payload").toObject();
        if (!m.topic.isEmpty()) emit messageReceived(m);
    }
}

void IpcClient::onConnected()  { emit connected(); }
void IpcClient::onDisconnected(){ emit disconnected(); }
