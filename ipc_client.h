#pragma once
#include <QtCore>
#include <QtNetwork>

struct IpcMessage {
    QString topic;
    QString reqId;
    QJsonObject payload;
};

class IpcClient : public QObject {
    Q_OBJECT
public:
    explicit IpcClient(const QString& socketPath, QObject* parent=nullptr);

    void connectIfNeeded();

    QString send(const QString& topic, const QJsonObject& payload, const QString& reqId = QString());

signals:
    void messageReceived(const IpcMessage& msg);
    void connected();
    void disconnected();

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();

private:
    static QByteArray packJson(const QJsonObject& obj);
    void processBuffer();

    QString m_path;
    QLocalSocket* m_sock;
    QByteArray m_buf;
};
