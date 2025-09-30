// MainView.cpp
#include "mainview.h"
#include "ui_mainview.h"

#include <QDateTime>
#include <QTimer>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QToolButton>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QEasingCurve>
#include <QJsonObject>
#include <QDebug>
#include <QLocalSocket>
#include <algorithm>

#include "../ipc_client.h"
#include "../WarningDialog.h"

namespace {

// ===== 값 범위 정의 =====
// Seat
constexpr int SEAT_POS_MIN   = 0;
constexpr int SEAT_POS_MAX   = 100;
constexpr int SEAT_ANGLE_MIN = 0;
constexpr int SEAT_ANGLE_MAX = 180;
constexpr int SEAT_HT_MIN    = 0;
constexpr int SEAT_HT_MAX    = 100;
constexpr int SEAT_STEP      = 1;

// Side/Room Mirror, Handle Angle 범위
constexpr int ANGLE_MIRROR_MIN = -45;
constexpr int ANGLE_MIRROR_MAX =  45;
constexpr int ANGLE_HANDLE_MIN = -90;
constexpr int ANGLE_HANDLE_MAX =  90;
constexpr int ANGLE_STEP       = 1;

// Handle Position
constexpr int HANDLE_POS_MIN = 0;
constexpr int HANDLE_POS_MAX = 100;
constexpr int HANDLE_POS_STEP= 1;

const QString kSock = "/tmp/dcu.demo.sock";

// 버튼 선택 하이라이트 스타일 (checked 시 채우기)
const char* kToggleQSS =
    "QPushButton {"
    "  border: 1px solid #B0B0B0; border-radius: 8px; padding: 6px 10px;"
    "}"
    "QPushButton:checked {"
    "  background: #2F6FED; color: #FFFFFF; border: 1px solid #2F6FED;"
    "}";

} // namespace

MainView::MainView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainView)
{
    ui->setupUi(this);

    // ===== IPC =====
    m_ipc = new IpcClient(kSock, this);
    connect(m_ipc, &IpcClient::messageReceived, this, &MainView::onIpcMessage);

    // ===== 메인 페이지 전환 버튼 =====
    ui->stackedCenter->setCurrentWidget(ui->pageSettings);
    ui->btnMenu->setText("주행");
    connect(ui->btnMenu, &QToolButton::clicked, this, [this]{
        auto stack = ui->stackedCenter;
        const bool isSettings = (stack->currentWidget() == ui->pageSettings);
        const int nextIndex = isSettings
                            ? stack->indexOf(ui->pageDashboard)
                            : stack->indexOf(ui->pageSettings);
        fadeToPage(stack, nextIndex, 320);
    });

    // ===== 메인 네비 그룹 (Handle, SideMirror, Seat, RoomMirror) =====
    m_groupMain = new QButtonGroup(this);
    m_groupMain->setExclusive(true);
    m_groupMain->addButton(ui->btnHandle);
    m_groupMain->addButton(ui->btnSideMirror);
    m_groupMain->addButton(ui->btnSeat);
    m_groupMain->addButton(ui->btnRoomMirror);

    // 기본 Handle 선택
    ui->btnHandle->setChecked(true);
    ui->stkDetail->setCurrentIndex(0);

    connect(ui->btnHandle, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 0, 240);
        if (m_groupHandle) highlightChecked(m_groupHandle);
        updateHandleLabel();
    });
    connect(ui->btnSideMirror, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 1, 240);
        if (!ui->radioLeft->isChecked() && !ui->radioRight->isChecked())
            ui->radioLeft->setChecked(true);
        if (m_groupSide) highlightChecked(m_groupSide);
        updateSideMirrorLabel();
    });
    connect(ui->btnSeat, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 2, 240);
        if (m_groupSeat) highlightChecked(m_groupSeat);
        updateSeatLabel();
    });
    connect(ui->btnRoomMirror, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 3, 240);
        if (m_groupRoom) highlightChecked(m_groupRoom);
        updateRoomMirrorLabel();
    });

    // ===== 서브 버튼 그룹 구성 & 토글 스타일 =====
    // Handle page
    m_groupHandle = new QButtonGroup(this);
    m_groupHandle->setExclusive(true);
    initButtonAsToggle(ui->btnHandleAngle);
    initButtonAsToggle(ui->btnHandlePosition);
    m_groupHandle->addButton(ui->btnHandleAngle);
    m_groupHandle->addButton(ui->btnHandlePosition);
    ui->btnHandleAngle->setChecked(true);      // 기본 선택
    connect(m_groupHandle, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){ highlightChecked(m_groupHandle); updateHandleLabel(); });

    // RoomMirror page
    m_groupRoom = new QButtonGroup(this);
    m_groupRoom->setExclusive(true);
    initButtonAsToggle(ui->btnRoomMirrorPitch);
    initButtonAsToggle(ui->btnRoomMirrorYaw);
    m_groupRoom->addButton(ui->btnRoomMirrorPitch);
    m_groupRoom->addButton(ui->btnRoomMirrorYaw);
    ui->btnRoomMirrorPitch->setChecked(true);  // 기본 선택
    connect(m_groupRoom, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){ highlightChecked(m_groupRoom); updateRoomMirrorLabel(); });

    // Seat page
    m_groupSeat = new QButtonGroup(this);
    m_groupSeat->setExclusive(true);
    initButtonAsToggle(ui->btnSeatPosition);
    initButtonAsToggle(ui->btnSeatAngle);
    initButtonAsToggle(ui->btnSeatFront);
    initButtonAsToggle(ui->btnSeatRear);
    m_groupSeat->addButton(ui->btnSeatPosition);
    m_groupSeat->addButton(ui->btnSeatAngle);
    m_groupSeat->addButton(ui->btnSeatFront);
    m_groupSeat->addButton(ui->btnSeatRear);
    ui->btnSeatPosition->setChecked(true);     // 기본 선택
    connect(m_groupSeat, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){ highlightChecked(m_groupSeat); updateSeatLabel(); });

    // SideMirror page
    m_groupSide = new QButtonGroup(this);
    m_groupSide->setExclusive(true);
    initButtonAsToggle(ui->btnSideMirrorYaw);
    initButtonAsToggle(ui->btnSideMirrorPitch);
    m_groupSide->addButton(ui->btnSideMirrorYaw);
    m_groupSide->addButton(ui->btnSideMirrorPitch);
    ui->btnSideMirrorYaw->setChecked(true);    // 기본 선택
    connect(m_groupSide, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){ highlightChecked(m_groupSide); updateSideMirrorLabel(); });

    // SideMirror 좌/우 라디오
    ui->radioLeft->setChecked(true);
    connect(ui->radioLeft,  &QRadioButton::toggled, this, [this](bool){ updateSideMirrorLabel(); });
    connect(ui->radioRight, &QRadioButton::toggled, this, [this](bool){ updateSideMirrorLabel(); });

    // ===== Up / Down 버튼 =====
    connect(ui->btnUp, &QToolButton::clicked, this, [this]{
        switch (ui->stkDetail->currentIndex()) {
        case 0: { // Handle
            if (ui->btnHandleAngle->isChecked()) {
                m_handleAngle = std::min(ANGLE_HANDLE_MAX, m_handleAngle + ANGLE_STEP);
                updateHandleLabel();
                sendApply(QJsonObject{{"handleAngle", m_handleAngle}});
            } else {
                m_handlePosition = std::min(HANDLE_POS_MAX, m_handlePosition + HANDLE_POS_STEP);
                updateHandleLabel();
                sendApply(QJsonObject{{"handlePosition", m_handlePosition}});
            }
            break;
        }
        case 1: { // Side Mirror
            const bool isLeft = ui->radioLeft->isChecked();
            if (ui->btnSideMirrorYaw->isChecked()) {
                if (isLeft) {
                    m_sideMirrorLeftYaw = std::min(ANGLE_MIRROR_MAX, m_sideMirrorLeftYaw + ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorLeftYaw", m_sideMirrorLeftYaw}});
                } else {
                    m_sideMirrorRightYaw = std::min(ANGLE_MIRROR_MAX, m_sideMirrorRightYaw + ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorRightYaw", m_sideMirrorRightYaw}});
                }
            } else {
                if (isLeft) {
                    m_sideMirrorLeftPitch = std::min(ANGLE_MIRROR_MAX, m_sideMirrorLeftPitch + ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorLeftPitch", m_sideMirrorLeftPitch}});
                } else {
                    m_sideMirrorRightPitch = std::min(ANGLE_MIRROR_MAX, m_sideMirrorRightPitch + ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorRightPitch", m_sideMirrorRightPitch}});
                }
            }
            updateSideMirrorLabel();
            break;
        }
        case 2: { // Seat
            if (ui->btnSeatPosition->isChecked()) {
                m_seatPosition = std::min(SEAT_POS_MAX, m_seatPosition + SEAT_STEP);
                sendApply(QJsonObject{{"seatPosition", m_seatPosition}});
            } else if (ui->btnSeatAngle->isChecked()) {
                m_seatAngle = std::min(SEAT_ANGLE_MAX, m_seatAngle + ANGLE_STEP);
                sendApply(QJsonObject{{"seatAngle", m_seatAngle}});
            } else if (ui->btnSeatFront->isChecked()) {
                m_seatFrontHeight = std::min(SEAT_HT_MAX, m_seatFrontHeight + SEAT_STEP);
                sendApply(QJsonObject{{"seatFrontHeight", m_seatFrontHeight}});
            } else {
                m_seatRearHeight = std::min(SEAT_HT_MAX, m_seatRearHeight + SEAT_STEP);
                sendApply(QJsonObject{{"seatRearHeight", m_seatRearHeight}});
            }
            updateSeatLabel();
            break;
        }
        case 3: { // Room Mirror
            if (ui->btnRoomMirrorPitch->isChecked()) {
                m_roomMirrorPitch = std::min(ANGLE_MIRROR_MAX, m_roomMirrorPitch + ANGLE_STEP);
                sendApply(QJsonObject{{"roomMirrorPitch", m_roomMirrorPitch}});
            } else {
                m_roomMirrorYaw = std::min(ANGLE_MIRROR_MAX, m_roomMirrorYaw + ANGLE_STEP);
                sendApply(QJsonObject{{"roomMirrorYaw", m_roomMirrorYaw}});
            }
            updateRoomMirrorLabel();
            break;
        }
        }
        syncDashboard();
    });

    connect(ui->btnDown, &QToolButton::clicked, this, [this]{
        switch (ui->stkDetail->currentIndex()) {
        case 0: { // Handle
            if (ui->btnHandleAngle->isChecked()) {
                m_handleAngle = std::max(ANGLE_HANDLE_MIN, m_handleAngle - ANGLE_STEP);
                updateHandleLabel();
                sendApply(QJsonObject{{"handleAngle", m_handleAngle}});
            } else {
                m_handlePosition = std::max(HANDLE_POS_MIN, m_handlePosition - HANDLE_POS_STEP);
                updateHandleLabel();
                sendApply(QJsonObject{{"handlePosition", m_handlePosition}});
            }
            break;
        }
        case 1: { // Side Mirror
            const bool isLeft = ui->radioLeft->isChecked();
            if (ui->btnSideMirrorYaw->isChecked()) {
                if (isLeft) {
                    m_sideMirrorLeftYaw = std::max(ANGLE_MIRROR_MIN, m_sideMirrorLeftYaw - ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorLeftYaw", m_sideMirrorLeftYaw}});
                } else {
                    m_sideMirrorRightYaw = std::max(ANGLE_MIRROR_MIN, m_sideMirrorRightYaw - ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorRightYaw", m_sideMirrorRightYaw}});
                }
            } else {
                if (isLeft) {
                    m_sideMirrorLeftPitch = std::max(ANGLE_MIRROR_MIN, m_sideMirrorLeftPitch - ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorLeftPitch", m_sideMirrorLeftPitch}});
                } else {
                    m_sideMirrorRightPitch = std::max(ANGLE_MIRROR_MIN, m_sideMirrorRightPitch - ANGLE_STEP);
                    sendApply(QJsonObject{{"sideMirrorRightPitch", m_sideMirrorRightPitch}});
                }
            }
            updateSideMirrorLabel();
            break;
        }
        case 2: { // Seat
            if (ui->btnSeatPosition->isChecked()) {
                m_seatPosition = std::max(SEAT_POS_MIN, m_seatPosition - SEAT_STEP);
                sendApply(QJsonObject{{"seatPosition", m_seatPosition}});
            } else if (ui->btnSeatAngle->isChecked()) {
                m_seatAngle = std::max(SEAT_ANGLE_MIN, m_seatAngle - ANGLE_STEP);
                sendApply(QJsonObject{{"seatAngle", m_seatAngle}});
            } else if (ui->btnSeatFront->isChecked()) {
                m_seatFrontHeight = std::max(SEAT_HT_MIN, m_seatFrontHeight - SEAT_STEP);
                sendApply(QJsonObject{{"seatFrontHeight", m_seatFrontHeight}});
            } else {
                m_seatRearHeight = std::max(SEAT_HT_MIN, m_seatRearHeight - SEAT_STEP);
                sendApply(QJsonObject{{"seatRearHeight", m_seatRearHeight}});
            }
            updateSeatLabel();
            break;
        }
        case 3: { // Room Mirror
            if (ui->btnRoomMirrorPitch->isChecked()) {
                m_roomMirrorPitch = std::max(ANGLE_MIRROR_MIN, m_roomMirrorPitch - ANGLE_STEP);
                sendApply(QJsonObject{{"roomMirrorPitch", m_roomMirrorPitch}});
            } else {
                m_roomMirrorYaw = std::max(ANGLE_MIRROR_MIN, m_roomMirrorYaw - ANGLE_STEP);
                sendApply(QJsonObject{{"roomMirrorYaw", m_roomMirrorYaw}});
            }
            updateRoomMirrorLabel();
            break;
        }
        }
        syncDashboard();
    });

    // ===== 시계 =====
    updateClock();
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &MainView::updateClock);
    m_clockTimer->start();

    // ===== 초기 라벨 세팅 =====
    updateHandleLabel();
    updateSeatLabel();
    updateRoomMirrorLabel();
    updateSideMirrorLabel();
    syncDashboard();
}

MainView::~MainView() { delete ui; }

void MainView::initButtonAsToggle(QAbstractButton* btn)
{
    if (!btn) return;
    btn->setCheckable(true);
    btn->setStyleSheet(kToggleQSS);
}

void MainView::highlightChecked(QButtonGroup* group)
{
    if (!group) return;
    // 스타일은 QSS로 처리하므로 여기서는 별도 동작 불필요.
    // 필요하다면 그룹 내 버튼 상태를 검사해 추가 로직 가능.
    (void)group;
}

void MainView::loadInitialProfile(const QJsonObject& profile)
{
    m_initializing = true;

    auto getInt = [&](const char* key, int defVal) {
        return profile.contains(key) ? profile.value(key).toInt(defVal) : defVal;
    };

    // Seat
    m_seatPosition   = std::clamp(getInt("seatPosition",   m_seatPosition),   SEAT_POS_MIN,   SEAT_POS_MAX);
    m_seatAngle      = std::clamp(getInt("seatAngle",      m_seatAngle),      SEAT_ANGLE_MIN, SEAT_ANGLE_MAX);
    m_seatFrontHeight= std::clamp(getInt("seatFrontHeight",m_seatFrontHeight),SEAT_HT_MIN,    SEAT_HT_MAX);
    m_seatRearHeight = std::clamp(getInt("seatRearHeight", m_seatRearHeight), SEAT_HT_MIN,    SEAT_HT_MAX);

    // Side Mirror
    m_sideMirrorLeftYaw   = std::clamp(getInt("sideMirrorLeftYaw",   m_sideMirrorLeftYaw),   ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
    m_sideMirrorLeftPitch = std::clamp(getInt("sideMirrorLeftPitch", m_sideMirrorLeftPitch), ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
    m_sideMirrorRightYaw  = std::clamp(getInt("sideMirrorRightYaw",  m_sideMirrorRightYaw),  ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
    m_sideMirrorRightPitch= std::clamp(getInt("sideMirrorRightPitch",m_sideMirrorRightPitch),ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);

    // Room Mirror
    m_roomMirrorYaw   = std::clamp(getInt("roomMirrorYaw",   m_roomMirrorYaw),   ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
    m_roomMirrorPitch = std::clamp(getInt("roomMirrorPitch", m_roomMirrorPitch), ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);

    // Handle
    m_handlePosition = std::clamp(getInt("handlePosition", m_handlePosition), HANDLE_POS_MIN, HANDLE_POS_MAX);
    m_handleAngle    = std::clamp(getInt("handleAngle",    m_handleAngle),    ANGLE_HANDLE_MIN, ANGLE_HANDLE_MAX);

    // 라벨 반영
    updateHandleLabel();
    updateSeatLabel();
    updateRoomMirrorLabel();
    updateSideMirrorLabel();

    m_initializing = false;
    syncDashboard();
}

void MainView::on_pushButton_clicked()
{
    // (기존 테스트용) 필요 없으면 삭제 가능
    QLocalSocket sock;
    sock.connectToServer(kSock);
    if (!sock.waitForConnected(1000)) {
        qDebug() << "connect failed:" << sock.errorString();
        return;
    }
    sock.write("hello");
    sock.flush();
    sock.waitForBytesWritten(1000);
    sock.disconnectFromServer();
}

void MainView::updateClock()
{
    ui->lblClock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd (ddd) HH:mm:ss"));
}

void MainView::fadeToPage(QStackedWidget* stack, int nextIndex, int durationMs)
{
    if (!stack) return;
    if (nextIndex == stack->currentIndex()) return;

    QWidget* cur  = stack->currentWidget();
    QWidget* next = stack->widget(nextIndex);
    if (!next || next == cur) return;

    if (stack->property("isAnimating").toBool()) return;
    stack->setProperty("isAnimating", true);

    auto* outEff = new QGraphicsOpacityEffect(cur);
    auto* inEff  = new QGraphicsOpacityEffect(next);
    cur->setGraphicsEffect(outEff);
    next->setGraphicsEffect(inEff);
    inEff->setOpacity(0.0);

    auto* fadeOut = new QPropertyAnimation(outEff, "opacity", this);
    fadeOut->setDuration(durationMs);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutQuad);

    auto* fadeIn = new QPropertyAnimation(inEff, "opacity", this);
    fadeIn->setDuration(durationMs);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    auto* seq = new QSequentialAnimationGroup(this);
    stack->setEnabled(false);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this, stack, next](){
        stack->setCurrentWidget(next);
        next->update();

        if (stack == ui->stackedCenter) {
            if (stack->currentWidget() == ui->pageSettings)
                ui->btnMenu->setText("주행");
            else
                ui->btnMenu->setText("정지");
        }

        if (next == ui->pageDashboard) {
            syncDashboard();
        }
    });

    connect(seq, &QSequentialAnimationGroup::finished, this, [=](){
        if (cur->graphicsEffect())  cur->setGraphicsEffect(nullptr);
        if (next->graphicsEffect()) next->setGraphicsEffect(nullptr);
        stack->setEnabled(true);
        stack->setProperty("isAnimating", false);
    });

    seq->addAnimation(fadeOut);
    seq->addAnimation(fadeIn);
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

// ===== 페이지 라벨 업데이트 =====

void MainView::updateHandleLabel()
{
    if (ui->btnHandleAngle->isChecked()) {
        if (ui->lblHandleValue)
            ui->lblHandleValue->setText(QString::number(m_handleAngle) + u8"°");
    } else {
        if (ui->lblHandleValue)
            ui->lblHandleValue->setText(QString("%1").arg(m_handlePosition));
    }
}

void MainView::updateRoomMirrorLabel()
{
    // 현재 서브 선택에 따라 pitch 또는 yaw
    if (ui->btnRoomMirrorPitch->isChecked()) {
        if (ui->lblRoomMirrorValue)
            ui->lblRoomMirrorValue->setText(QString("Pitch %1°").arg(m_roomMirrorPitch));
    } else {
        if (ui->lblRoomMirrorValue)
            ui->lblRoomMirrorValue->setText(QString("Yaw %1°").arg(m_roomMirrorYaw));
    }
}

void MainView::updateSeatLabel()
{
    QString text;
    if (ui->btnSeatPosition->isChecked()) {
        text = QString("Pos %1%").arg(m_seatPosition);
    } else if (ui->btnSeatAngle->isChecked()) {
        text = QString("Angle %1°").arg(m_seatAngle);
    } else if (ui->btnSeatFront->isChecked()) {
        text = QString("Front %1%").arg(m_seatFrontHeight);
    } else { // Rear
        text = QString("Rear %1%").arg(m_seatRearHeight);
    }
    if (ui->lblSeatForeAftValue) // 기존 라벨 재사용
        ui->lblSeatForeAftValue->setText(text);
}

void MainView::updateSideMirrorLabel()
{
    const bool isLeft = ui->radioLeft->isChecked();
    QString text;
    if (ui->btnSideMirrorYaw->isChecked()) {
        if (isLeft) text = QString("L-Yaw %1°").arg(m_sideMirrorLeftYaw);
        else        text = QString("R-Yaw %1°").arg(m_sideMirrorRightYaw);
    } else {
        if (isLeft) text = QString("L-Pitch %1°").arg(m_sideMirrorLeftPitch);
        else        text = QString("R-Pitch %1°").arg(m_sideMirrorRightPitch);
    }
    if (ui->lblSideMirrorValue)
        ui->lblSideMirrorValue->setText(text);
}

// ===== 대시보드 요약 =====
// 기존 대시보드 라벨 이름을 유지하면서, 새 값들에서 대표값을 골라 요약 표기
void MainView::syncDashboard()
{
    if (ui->dashRoomMirrorValue)
        ui->dashRoomMirrorValue->setText(
            QString("Yaw %1° / Pitch %2°").arg(m_roomMirrorYaw).arg(m_roomMirrorPitch));

    if (ui->dashSeatTiltValue)
        ui->dashSeatTiltValue->setText(
            QString("Angle %1°").arg(m_seatAngle));

    if (ui->dashSideMirrorValue)
        ui->dashSideMirrorValue->setText(
            QString("L(Y:%1/P:%2)  R(Y:%3/P:%4)")
                .arg(m_sideMirrorLeftYaw)
                .arg(m_sideMirrorLeftPitch)
                .arg(m_sideMirrorRightYaw)
                .arg(m_sideMirrorRightPitch));

    if (ui->dashSeatFwdValue)
        ui->dashSeatFwdValue->setText(
            QString("Pos %1% / F %2% / R %3%")
                .arg(m_seatPosition)
                .arg(m_seatFrontHeight)
                .arg(m_seatRearHeight));
}

// ===== IPC =====

void MainView::sendApply(const QJsonObject& changes)
{
    if (m_initializing) return;
    QJsonObject payload = changes;
    payload.insert("ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_powerReqId = m_ipc->send("power/apply", payload);
}

void MainView::onIpcMessage(const IpcMessage& msg)
{
    if (msg.topic == "power/apply/ack" && (!m_powerReqId.isEmpty() && msg.reqId == m_powerReqId)) {
        const bool ok = msg.payload.value("ok").toBool(false);
        qInfo() << "[power/apply/ack]" << ok << msg.payload;
    }
    if (msg.topic == "system/warning") {
        const QString title   = msg.payload.value("title").toString(u8"경고");
        const QString message = msg.payload.value("message").toString(u8"warnings");
        QMetaObject::invokeMethod(this, [=]{
            auto* dlg = new WarningDialog(title, message, this);
            dlg->setGeometry((width()-400)/2, (height()-200)/2, 400, 200);
            dlg->show();
        }, Qt::QueuedConnection);
    }
}
