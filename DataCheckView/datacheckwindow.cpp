#include "datacheckwindow.h"
#include "ui_datacheckwindow.h"
#include <QDateTime>

static const QString kSock = "/tmp/dcu.demo.sock";

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
    m_authPayload = QJsonObject{};     // 기본/빈 시작
    // ↓ 공통 시작 로직 호출
    beginWithAuth(m_authPayload);
}

void DataCheckWindow::beginWithAuth(const QJsonObject& authPayload) {
    m_phase = Phase::Checking;
    m_hasData = false;
    m_dataPayload = QJsonObject{};
    m_dataReqId.clear();
    m_powerReqId.clear();
    m_authPayload = authPayload;       

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
        break;

    case Phase::DataFoundMsg:
        m_phase = Phase::Applying;
        setMessage(QStringLiteral("적용중입니다..."), true);

        m_powerReqId = m_ipc->send("power/apply", m_dataPayload);

        m_waitTimer.start(4000);
        break;

    case Phase::NoDataMsg:
        m_phase = Phase::Measuring;
        setMessage(QStringLiteral("신체 측정 중입니다..."), true);
        m_timer.start(2200);
        break;

    case Phase::Measuring:
        m_phase = Phase::Calculating;
        setMessage(QStringLiteral("계산중입니다..."), true);
        m_timer.start(1600);
        break;

    case Phase::Calculating:
        m_phase = Phase::Applying;
        setMessage(QStringLiteral("적용중입니다..."), true);


        m_dataPayload = QJsonObject{
            {"sideL",    10},
            {"sideR",    0},
            {"seatTilt", 30},
            {"foreAft",  20},
            {"backMirror", 5}
        };
        emit profileResolved(m_dataPayload);
        m_powerReqId = m_ipc->send("power/apply", m_dataPayload);
        m_waitTimer.start(4000);
        break;

    case Phase::Applying:
        break;

    case Phase::Done:

        break;

    default:
        break;
    }
}

void DataCheckWindow::onIpcMessage(const IpcMessage& msg) {
    if (msg.topic == "data/result") {
        if (!m_dataReqId.isEmpty() && msg.reqId == m_dataReqId) {
            if (m_waitTimer.isActive()) m_waitTimer.stop();

            int err = msg.payload.value("error").toInt(-1);
            if (err == 0) {
                m_hasData = true;

                if (msg.payload.contains("data") && msg.payload.value("data").isObject()) {
                    m_dataPayload = msg.payload.value("data").toObject();
                } else {
                    m_dataPayload = msg.payload;
                    m_dataPayload.remove("error");
                }
                emit profileResolved(m_dataPayload);

                m_phase = Phase::DataFoundMsg;
                setMessage(QStringLiteral("데이터가 확인되었습니다."), false);
                setDeterminate(0);
                m_timer.start(1000);
            } else {
                m_hasData = false;
                m_phase = Phase::NoDataMsg;
                setMessage(QStringLiteral("데이터가 없습니다."), false);
                setDeterminate(0);
                m_timer.start(1200);
            }
        }
        return;
    }

    if (msg.topic == "power/apply/ack") {
        if (!m_powerReqId.isEmpty() && msg.reqId == m_powerReqId) {
            if (m_waitTimer.isActive()) m_waitTimer.stop();

            bool ok = msg.payload.value("ok").toBool(false);
            if (ok) {
                m_phase = Phase::Done;
                m_timer.stop();
                setMessage(QStringLiteral("적용이 완료되었습니다."), false);
                setDeterminate(100);
                emit dataCheckFinished();
            } else {
                setMessage(QStringLiteral("적용 실패. 재시도합니다..."), true);
                m_powerReqId = m_ipc->send("power/apply", m_dataPayload);
                m_waitTimer.start(4000);
            }
        }
        return;
    }
}

void DataCheckWindow::onWaitTimeout() {
    if (!m_dataReqId.isEmpty() && m_phase == Phase::Checking) {
        m_phase = Phase::NoDataMsg;
        setMessage(QStringLiteral("네트워크 지연으로 데이터 확인 실패. 측정으로 진행합니다."), false);
        setDeterminate(0);
        m_timer.start(1200);
        return;
    }

    if (!m_powerReqId.isEmpty() && m_phase == Phase::Applying) {
        setMessage(QStringLiteral("적용 응답 지연. 다시 시도합니다..."), true);
        m_powerReqId = m_ipc->send("power/apply", m_dataPayload);
        m_waitTimer.start(4000);
        return;
    }
}

void DataCheckWindow::setMessage(const QString& text, bool busy) {
    ui->loadingLabel->setText(text);
    if (busy) {
        ui->progressLine->setRange(0, 0);
    } else {
        ui->progressLine->setRange(0, 100);
    }
}

void DataCheckWindow::setDeterminate(int v) {
    if (ui->progressLine->maximum() == 0) {
        ui->progressLine->setRange(0, 100);
    }
    ui->progressLine->setValue(qBound(0, v, 100));
}
