#include "datacheckwindow.h"
#include "ui_datacheckwindow.h"
#include <QDateTime>

static const QString kSock = "/tmp/dcu.demo.sock";

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

static inline QString asJsonCompact(const QJsonObject& o) {
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

DataCheckWindow::DataCheckWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataCheckWindow)
{
    ui->setupUi(this);

    ui->progressLine->setRange(0, 0);
    ui->progressLine->setTextVisible(false);
    ui->loadingLabel->setText(QString());

    connect(&m_timer, &QTimer::timeout, this, &DataCheckWindow::advance);

    m_ipc = new IpcClient(kSock, this);
    connect(m_ipc, &IpcClient::messageReceived, this, &DataCheckWindow::onIpcMessage);

    m_waitTimer.setSingleShot(true);
    connect(&m_waitTimer, &QTimer::timeout, this, &DataCheckWindow::onWaitTimeout);
}

DataCheckWindow::~DataCheckWindow() { delete ui; }

void DataCheckWindow::begin(bool /*hasData*/) {
    m_authPayload = QJsonObject{};
    beginWithAuth(m_authPayload);
}

void DataCheckWindow::beginWithAuth(const QJsonObject& authPayload) {
    m_phase = Phase::Checking;
    m_hasData = false;
    m_dataPayload = QJsonObject{};
    m_dataReqId.clear();
    m_powerReqId.clear();
    m_authPayload = authPayload;

    qInfo().noquote() << "[DATACHECK] beginWithAuth: authPayload=" << asJsonCompact(m_authPayload);

    setMessage(QStringLiteral("데이터가 있는지 확인합니다..."), true);
    ui->progressLine->setRange(0, 0);

    QJsonObject payload{
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"query", "user-profile"},
        {"auth", m_authPayload}
    };

    m_dataReqId = m_ipc->send("data/result", payload);
    m_waitTimer.start(5000);
}

void DataCheckWindow::advance() {
    switch (m_phase)
    {
    case Phase::Checking:
        // 계속 대기. 타이머는 onWaitTimeout에서만 다룸.
        break;

    case Phase::DataFoundMsg:
        m_phase = Phase::Applying;
        setMessage(QStringLiteral("적용중입니다..."), true);
        m_powerReqId = m_ipc->send("power/apply", m_dataPayload);
        m_waitTimer.start(4000);
        break;

    case Phase::NoDataMsg:
        // 기존: 측정/계산 플로우로 전환했지만, 이제는 사용하지 않음.
        // 안전을 위해 아무것도 하지 않음.
        break;

    case Phase::Measuring:
    case Phase::Calculating:
        // 더 이상 사용하지 않음.
        break;

    case Phase::Applying:
        // 계속 대기. 타이머는 onWaitTimeout에서만 다룸.
        break;

    case Phase::Done:
        // 완료 상태
        break;

    default:
        break;
    }
}

void DataCheckWindow::onIpcMessage(const IpcMessage& msg) {
    qInfo().noquote() << "[DATACHECK] recv <- topic=" << msg.topic
                      << " reqId=" << msg.reqId
                      << " payload=" << asJsonCompact(msg.payload);

    if (msg.topic == "data/result") {
        if (!m_dataReqId.isEmpty() && msg.reqId == m_dataReqId) {
            if (m_waitTimer.isActive()) m_waitTimer.stop();

            const int err = msg.payload.value("error").toInt(-1);
            if (err == 0) {
                m_hasData = true;

                if (msg.payload.contains("data") && msg.payload.value("data").isObject()) {
                    m_dataPayload = msg.payload.value("data").toObject();
                } else {
                    // 하위호환: error 외 필드들이 곧바로 data인 경우 지원
                    m_dataPayload = msg.payload;
                    m_dataPayload.remove("error");
                }
                emit profileResolved(m_dataPayload);

                m_phase = Phase::DataFoundMsg;
                setMessage(QStringLiteral("데이터가 확인되었습니다."), false);
                setDeterminate(0);
                m_timer.start(1000);
            } else {
                // 변경점: 실패/없음이어도 측정으로 넘어가지 않고 계속 대기
                m_hasData = false;
                // m_phase 유지(Checking)로 두고, 메시지 갱신 + 타이머 재시작
                setMessage(QStringLiteral("프로필 데이터를 수신 대기 중..."), true);
                m_waitTimer.start(5000);
            }
        }
        return;
    }

    if (msg.topic == "power/apply/ack") {
        if (!m_powerReqId.isEmpty() && msg.reqId == m_powerReqId) {
            if (m_waitTimer.isActive()) m_waitTimer.stop();

            const bool ok = msg.payload.value("ok").toBool(false);
            if (ok) {
                m_phase = Phase::Done;
                m_timer.stop();
                setMessage(QStringLiteral("적용이 완료되었습니다."), false);
                setDeterminate(100);
                emit dataCheckFinished();
            } else {
                // 변경점: 재전송하지 않고 계속 대기
                setMessage(QStringLiteral("적용 확인 중..."), true);
                m_waitTimer.start(4000);
            }
        }
        return;
    }
}

void DataCheckWindow::onWaitTimeout() {
    if (!m_dataReqId.isEmpty() && m_phase == Phase::Checking) {
        // 변경점: 측정으로 전환하지 않고 계속 대기
        setMessage(QStringLiteral("프로필 데이터를 수신 대기 중..."), true);
        m_waitTimer.start(5000);
        return;
    }

    if (!m_powerReqId.isEmpty() && m_phase == Phase::Applying) {
        // 변경점: 재전송하지 않고 계속 대기
        setMessage(QStringLiteral("적용 확인 중..."), true);
        m_waitTimer.start(4000);
        return;
    }
}

void DataCheckWindow::setMessage(const QString& text, bool busy) {
    ui->loadingLabel->setText(text);
    if (busy) {
        ui->progressLine->setRange(0, 0); // 인디터미닛
    } else {
        ui->progressLine->setRange(0, 100); // 디터미닛
    }
}

void DataCheckWindow::setDeterminate(int v) {
    if (ui->progressLine->maximum() == 0) {
        ui->progressLine->setRange(0, 100);
    }
    ui->progressLine->setValue(qBound(0, v, 100));
}
