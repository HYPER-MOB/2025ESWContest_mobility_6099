#pragma once
#include <QtCore>
#include <QtNetwork>
#include <functional>

struct IpcMessage {
    QString topic;
    QString reqId;         
    QJsonObject payload;   
};

class IpcConnection : public QObject {
    Q_OBJECT
public:
    explicit IpcConnection(QLocalSocket* socket, QObject* parent = nullptr);

    void send(const IpcMessage& msg);

    QLocalSocket* socket() const { return m_sock; }

signals:
    void messageReceived(const IpcMessage& msg, IpcConnection* conn);
    void disconnected(IpcConnection* conn);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processBuffer();

    QLocalSocket* m_sock;
    QByteArray m_buf; 
};

class IpcServer : public QObject {
    Q_OBJECT
public:
    explicit IpcServer(const QString& socketPath, QObject* parent = nullptr);

    using Handler = std::function<void(const IpcMessage&, IpcConnection*)>;

    void addHandler(const QString& topicOrPrefix, Handler h);

    bool listen(QString* err = nullptr);
    void close();

private slots:
    void onNewConnection();
    void onMessage(const IpcMessage& msg, IpcConnection* conn);
    void onConnGone(IpcConnection* conn);

private:
    Handler findHandler(const QString& topic) const;

    QString m_path;
    QLocalServer* m_server;
    QSet<IpcConnection*> m_conns;

    // 등록 순서 보존 + 빠른 조회를 위해 두 맵 병행
    QHash<QString, Handler> m_exact;  
    QList<QPair<QString, Handler>> m_prefix; 
};

