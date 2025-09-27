#include "ipc.h"

static QByteArray packJson(const QJsonObject& obj) {
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray out;
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 len = static_cast<quint32>(body.size());
    ds << len;
    out.append(body);
    return out;
}

static bool unpackJsonFrames(QByteArray& buf, QList<QJsonObject>& outFrames) {
    QDataStream ds(buf);
    ds.setByteOrder(QDataStream::LittleEndian);

    int offset = 0;
    while (true) {
        if (buf.size() - offset < 4) break; // need length
        quint32 len;
        memcpy(&len, buf.constData() + offset, 4);
        // LittleEndian
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        len = qFromLittleEndian(len);
#endif
        if (buf.size() - offset - 4 < static_cast<int>(len)) break; // wait more

        QByteArray body = buf.mid(offset + 4, len);
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            outFrames.push_back(doc.object());
        }
        offset += 4 + static_cast<int>(len);
    }
    if (offset > 0) {
        buf.remove(0, offset);
    }
    return !outFrames.isEmpty();
}

IpcConnection::IpcConnection(QLocalSocket* socket, QObject* parent)
    : QObject(parent), m_sock(socket) {
    connect(m_sock, &QLocalSocket::readyRead, this, &IpcConnection::onReadyRead);
    connect(m_sock, &QLocalSocket::disconnected, this, &IpcConnection::onDisconnected);
}

void IpcConnection::send(const IpcMessage& msg) {
    QJsonObject obj{
        {"topic", msg.topic},
    };
    if (!msg.reqId.isEmpty()) obj.insert("reqId", msg.reqId);
    obj.insert("payload", msg.payload);

    QByteArray packed = packJson(obj);
    m_sock->write(packed);
    m_sock->flush();
}

void IpcConnection::onReadyRead() {
    m_buf.append(m_sock->readAll());
    processBuffer();
}

void IpcConnection::processBuffer() {
    QList<QJsonObject> frames;
    if (!unpackJsonFrames(m_buf, frames)) return;

    for (const auto& obj : frames) {
        IpcMessage m;
        m.topic = obj.value("topic").toString();
        m.reqId = obj.value("reqId").toString();
        m.payload = obj.value("payload").toObject();

        if (!m.topic.isEmpty()) {
            emit messageReceived(m, this);
        }
    }
}

void IpcConnection::onDisconnected() {
    emit disconnected(this);
    m_sock->deleteLater();
    deleteLater();
}

// ---------- IpcServer ----------
IpcServer::IpcServer(const QString& socketPath, QObject* parent)
    : QObject(parent), m_path(socketPath), m_server(new QLocalServer(this)) {
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
}

void IpcServer::addHandler(const QString& topicOrPrefix, Handler h) {
    if (topicOrPrefix.endsWith("/*")) {
        m_prefix.push_back({topicOrPrefix.left(topicOrPrefix.size()-2), std::move(h)});
    } else {
        m_exact.insert(topicOrPrefix, std::move(h));
    }
}

bool IpcServer::listen(QString* err) {
    QFile::remove(m_path);
    if (!m_server->listen(m_path)) {
        if (err) *err = m_server->errorString();
        return false;
    }
    qInfo() << "[ipc] listening on" << m_path;
    return true;
}

void IpcServer::close() {
    for (auto* c : std::as_const(m_conns)) {
        c->disconnect(this);
        c->socket()->disconnectFromServer();
    }
    m_conns.clear();
    m_server->close();
    QFile::remove(m_path);
}

void IpcServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        auto* sock = m_server->nextPendingConnection();
        auto* conn = new IpcConnection(sock, this);
        m_conns.insert(conn);
        connect(conn, &IpcConnection::messageReceived, this, &IpcServer::onMessage);
        connect(conn, &IpcConnection::disconnected, this, &IpcServer::onConnGone);
        qInfo() << "[ipc] client connected";
        emit clientConnected(conn);                 
    }
}
void IpcServer::onConnGone(IpcConnection* conn) {
    m_conns.remove(conn);
    qInfo() << "[ipc] client disconnected";
    emit clientDisconnected(conn);                  
}

IpcServer::Handler IpcServer::findHandler(const QString& topic) const {
    if (m_exact.contains(topic)) return m_exact.value(topic);

    for (const auto& p : m_prefix) {
        const QString& pref = p.first;
        if (topic.startsWith(pref + "/") || topic == pref) {
            return p.second;
        }
    }
    return nullptr;
}

void IpcServer::onMessage(const IpcMessage& msg, IpcConnection* conn) {
    auto h = findHandler(msg.topic);
    if (h) {
        h(msg, conn);
        return;
    }
    
    QJsonObject err{
        {"error", "unknown_topic"},
        {"topic", msg.topic}
    };
    conn->send(IpcMessage{.topic = "system/warning", .reqId = msg.reqId, .payload = err});
}

