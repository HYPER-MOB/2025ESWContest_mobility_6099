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
#include <QJsonDocument>
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

// Side/Room Mirror, Handle Angle 범위
constexpr int ANGLE_MIRROR_MIN = -45;
constexpr int ANGLE_MIRROR_MAX =  45;
constexpr int ANGLE_HANDLE_MIN = -90;
constexpr int ANGLE_HANDLE_MAX =  90;

// Handle Position
constexpr int HANDLE_POS_MIN = 0;
constexpr int HANDLE_POS_MAX = 100;

const QString kSock = "/tmp/dcu.demo.sock";

static inline int clampInt(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }

const char* kToggleQSS =
    "QPushButton {"
    "  border: 1px solid #B0B0B0; border-radius: 8px; padding: 6px 10px;"
    "}"
    "QPushButton:checked {"
    "  background: #2F6FED; color: #FFFFFF; border: 1px solid #2F6FED;"
    "}";

// 버튼 주기 전송 간격 (ms) — daemon 쪽 기본 50ms와 맞춤
constexpr int kBtnTxPeriodMs = 50;

} // namespace

MainView::MainView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainView)
{
    ui->setupUi(this);

    // ===== IPC =====
    m_ipc = new IpcClient(kSock, this);
    connect(m_ipc, &IpcClient::messageReceived, this, &MainView::onIpcMessage);

    QJsonObject hello{
        {"who", "ui"},
        {"page", "MainView"},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };
    m_ipc->send("connect", hello);

    // ===== 버튼 IPC 주기 타이머 =====
    m_btnTxTimer = new QTimer(this);
    m_btnTxTimer->setTimerType(Qt::PreciseTimer);
    m_btnTxTimer->setInterval(kBtnTxPeriodMs);
    connect(m_btnTxTimer, &QTimer::timeout, this, &MainView::txButtonFlagsIfActive);

    zeroAllButtonFlags(); // 초기화

    // ===== 메인 페이지 전환 버튼 =====
    ui->stackedCenter->setCurrentWidget(ui->pageSettings);
    ui->btnMenu->setText("주행");
    connect(ui->btnMenu, &QToolButton::clicked, this, [this]{
        auto stack = ui->stackedCenter;
        const bool isSettings = (stack->currentWidget() == ui->pageSettings);
        const int nextIndex = isSettings
                            ? stack->indexOf(ui->pageDashboard)
                            : stack->indexOf(ui->pageSettings);

        sendDriveCommand(isSettings);

        // 전환 애니메이션 실행
        fadeToPage(stack, nextIndex, 320);

        // 페이지 전환 시 모든 버튼 플래그 해제(유령 입력 방지)
        zeroAllButtonFlags();
        txButtonFlagsImmediate();
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
        zeroAllButtonFlags(); txButtonFlagsImmediate();
        updateHandleLabel();
    });
    connect(ui->btnSideMirror, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 1, 240);
        if (!ui->radioLeft->isChecked() && !ui->radioRight->isChecked())
            ui->radioLeft->setChecked(true);
        if (m_groupSide) highlightChecked(m_groupSide);
        zeroAllButtonFlags(); txButtonFlagsImmediate();
        updateSideMirrorLabel();
    });
    connect(ui->btnSeat, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 2, 240);
        if (m_groupSeat) highlightChecked(m_groupSeat);
        zeroAllButtonFlags(); txButtonFlagsImmediate();
        updateSeatLabel();
    });
    connect(ui->btnRoomMirror, &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 3, 240);
        if (m_groupRoom) highlightChecked(m_groupRoom);
        zeroAllButtonFlags(); txButtonFlagsImmediate();
        updateRoomMirrorLabel();
    });

    // Handle page
    m_groupHandle = new QButtonGroup(this);
    m_groupHandle->setExclusive(true);
    initButtonAsToggle(ui->btnHandleAngle);
    initButtonAsToggle(ui->btnHandlePosition);
    m_groupHandle->addButton(ui->btnHandleAngle);
    m_groupHandle->addButton(ui->btnHandlePosition);
    ui->btnHandleAngle->setChecked(true);      // 기본 선택
    connect(m_groupHandle, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){
                highlightChecked(m_groupHandle);
                zeroAllButtonFlags(); txButtonFlagsImmediate();
                updateHandleLabel();
            });

    // RoomMirror page
    m_groupRoom = new QButtonGroup(this);
    m_groupRoom->setExclusive(true);
    initButtonAsToggle(ui->btnRoomMirrorPitch);
    initButtonAsToggle(ui->btnRoomMirrorYaw);
    m_groupRoom->addButton(ui->btnRoomMirrorPitch);
    m_groupRoom->addButton(ui->btnRoomMirrorYaw);
    ui->btnRoomMirrorPitch->setChecked(true);  // 기본 선택
    connect(m_groupRoom, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){
                highlightChecked(m_groupRoom);
                zeroAllButtonFlags(); txButtonFlagsImmediate();
                updateRoomMirrorLabel();
            });

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
            this, [this](QAbstractButton*){
                highlightChecked(m_groupSeat);
                zeroAllButtonFlags(); txButtonFlagsImmediate();
                updateSeatLabel();
            });

    // SideMirror page
    m_groupSide = new QButtonGroup(this);
    m_groupSide->setExclusive(true);
    initButtonAsToggle(ui->btnSideMirrorYaw);
    initButtonAsToggle(ui->btnSideMirrorPitch);
    m_groupSide->addButton(ui->btnSideMirrorYaw);
    m_groupSide->addButton(ui->btnSideMirrorPitch);
    ui->btnSideMirrorYaw->setChecked(true);    // 기본 선택
    connect(m_groupSide, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton*){
                highlightChecked(m_groupSide);
                zeroAllButtonFlags(); txButtonFlagsImmediate();
                updateSideMirrorLabel();
            });

    // SideMirror 좌/우 라디오
    ui->radioLeft->setChecked(true);
    connect(ui->radioLeft,  &QRadioButton::toggled, this, [this](bool){
        zeroAllButtonFlags(); txButtonFlagsImmediate();
        updateSideMirrorLabel();
    });
    connect(ui->radioRight, &QRadioButton::toggled, this, [this](bool){
        zeroAllButtonFlags(); txButtonFlagsImmediate();
        updateSideMirrorLabel();
    });

    // ===== Up / Down 버튼: 값 증감 제거, 플래그만 전송 =====
    // 누르는 동안: +면 1, -면 2. 떼면 0.
    connect(ui->btnUp, &QToolButton::pressed,  this, [this]{ setActiveControlFlag(/*plus=*/true,  /*pressed=*/true);  });
    connect(ui->btnUp, &QToolButton::released, this, [this]{ setActiveControlFlag(/*plus=*/true,  /*pressed=*/false); });

    connect(ui->btnDown, &QToolButton::pressed,  this, [this]{ setActiveControlFlag(/*plus=*/false, /*pressed=*/true);  });
    connect(ui->btnDown, &QToolButton::released, this, [this]{ setActiveControlFlag(/*plus=*/false, /*pressed=*/false); });

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

    // ===== 사이렌(경고음) =====
    m_sirenList = new QMediaPlaylist(this);
    m_sirenList->setPlaybackMode(QMediaPlaylist::Loop);
    m_sirenList->addMedia(QUrl("qrc:/images/MainView/siren.mp3"));
    m_sirenList->setCurrentIndex(0);

    m_sirenPlayer = new QMediaPlayer(this);
    m_sirenPlayer->setPlaylist(m_sirenList);
    m_sirenPlayer->setVolume(100);

    qInfo() << "[siren] exists?" << QFile(":/images/MainView/siren.mp3").exists();

    // ===== SAVE 버튼 연결 =====
    if (ui->btnSave) {
        ui->btnSave->setEnabled(true);
        ui->btnSave->setCheckable(false);
        connect(ui->btnSave, &QAbstractButton::clicked, this, [this]{
            qInfo() << "[UI] btnSave clicked";
            onSaveClicked();
        });
    } else {
        qWarning() << "[UI] btnSave is nullptr (objectName mismatch or not placed)";
    }
}

MainView::~MainView() { delete ui; }

// ====== 버튼 플래그 송신 로직 ======

void MainView::zeroAllButtonFlags()
{
    m_btnSeat = {0,0,0,0};
    m_btnMirror = {0,0,0,0,0,0};
    m_btnWheel = {0,0};
}

bool MainView::anyButtonActive() const
{
    for (auto v: m_btnSeat)   if (v) return true;
    for (auto v: m_btnMirror) if (v) return true;
    for (auto v: m_btnWheel)  if (v) return true;
    return false;
}

void MainView::txButtonFlagsImmediate()
{
    if (!m_ipc) return;

    // 좌석 버튼
    {
        QJsonObject p{
            {"position", (int)m_btnSeat[0]},
            {"angle",    (int)m_btnSeat[1]},
            {"front",    (int)m_btnSeat[2]},
            {"rear",     (int)m_btnSeat[3]},
        };
        m_ipc->send("button/seat", p);
    }
    // 미러 버튼
    {
        QJsonObject p{
            {"leftYaw",   (int)m_btnMirror[0]},
            {"leftPitch", (int)m_btnMirror[1]},
            {"rightYaw",  (int)m_btnMirror[2]},
            {"rightPitch",(int)m_btnMirror[3]},
            {"roomYaw",   (int)m_btnMirror[4]},
            {"roomPitch", (int)m_btnMirror[5]},
        };
        m_ipc->send("button/mirror", p);
    }
    // 휠 버튼
    {
        QJsonObject p{
            {"position", (int)m_btnWheel[0]},
            {"angle",    (int)m_btnWheel[1]},
        };
        m_ipc->send("button/wheel", p);
    }
}

void MainView::txButtonFlagsIfActive()
{
    if (!anyButtonActive()) {
        // 모두 0이면 타이머 정지(불필요한 IPC 방지)
        m_btnTxTimer->stop();
        return;
    }
    txButtonFlagsImmediate();
}

void MainView::setActiveControlFlag(bool plus, bool pressed)
{
    // 현재 상세 페이지 및 토글 상태로 어떤 신호를 보낼지 결정
    const int page = ui->stkDetail->currentIndex();
    const uint8_t flag = pressed ? (plus ? 1 : 2) : 0;

    switch (page) {
    case 0: { // Handle
        if (ui->btnHandleAngle->isChecked()) {
            m_btnWheel[1] = flag;   // angle
        } else {
            m_btnWheel[0] = flag;   // position
        }
        break;
    }
    case 1: { // Side Mirror
        const bool isLeft = ui->radioLeft->isChecked();
        if (ui->btnSideMirrorYaw->isChecked()) {
            if (isLeft) m_btnMirror[0] = flag;   // left yaw
            else        m_btnMirror[2] = flag;   // right yaw
        } else { // pitch
            if (isLeft) m_btnMirror[1] = flag;   // left pitch
            else        m_btnMirror[3] = flag;   // right pitch
        }
        break;
    }
    case 2: { // Seat
        if      (ui->btnSeatPosition->isChecked()) m_btnSeat[0] = flag; // position
        else if (ui->btnSeatAngle->isChecked())    m_btnSeat[1] = flag; // angle
        else if (ui->btnSeatFront->isChecked())    m_btnSeat[2] = flag; // front
        else                                       m_btnSeat[3] = flag; // rear
        break;
    }
    case 3: { // Room Mirror
        if (ui->btnRoomMirrorPitch->isChecked()) m_btnMirror[5] = flag; // room pitch
        else                                     m_btnMirror[4] = flag; // room yaw
        break;
    }
    default: break;
    }

    // 즉시 한 번 보내고, 누르고 있는 동안 주기적으로 계속 보냄
    txButtonFlagsImmediate();
    if (pressed && !m_btnTxTimer->isActive())
        m_btnTxTimer->start();
    if (!pressed && !anyButtonActive())
        m_btnTxTimer->stop();
}

// ====== 사이렌 ======
void MainView::startSiren() {
    if (!m_sirenPlayer) return;
    if (m_sirenPlayer->state() != QMediaPlayer::PlayingState) {
        m_sirenPlayer->play();
        qInfo() << "[siren] started";
    }
}

void MainView::stopSiren() {
    if (!m_sirenPlayer) return;
    if (m_sirenPlayer->state() != QMediaPlayer::StoppedState) {
        m_sirenPlayer->stop();
        qInfo() << "[siren] stopped";
    }
}

void MainView::initButtonAsToggle(QAbstractButton* btn)
{
    if (!btn) return;
    btn->setCheckable(true);
    btn->setStyleSheet(kToggleQSS);
}

void MainView::highlightChecked(QButtonGroup* group)
{
    if (!group) return;
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

    // 라벨 반영 (표시만)
    updateHandleLabel();
    updateSeatLabel();
    updateRoomMirrorLabel();
    updateSideMirrorLabel();

    m_initializing = false;
    syncDashboard();

    m_ipc->send("connect", {});
}

void MainView::on_pushButton_clicked()
{
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
    if (ui->lblSeatValue)
        ui->lblSeatValue->setText(text);
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

void MainView::syncDashboard()
{
    if (ui->dashHandleValue)
        ui->dashHandleValue->setText(
            QString("Pos %1 / Angle %2°")
                .arg(m_handlePosition)
                .arg(m_handleAngle));

    if (ui->dashRoomMirrorValue)
        ui->dashRoomMirrorValue->setText(
            QString("Yaw %1°\nPitch %2°")
                .arg(m_roomMirrorYaw)
                .arg(m_roomMirrorPitch));

    if (ui->dashSeatValue)
        ui->dashSeatValue->setText(
            QString("Pos %1%\nAngle %2°\nFront %3%\nRear %4%")
                .arg(m_seatPosition)
                .arg(m_seatAngle)
                .arg(m_seatFrontHeight)
                .arg(m_seatRearHeight));

    if (ui->dashSideMirrorValue)
        ui->dashSideMirrorValue->setText(
            QString("L: Y%1°/P%2°\nR: Y%3°/P%4°")
                .arg(m_sideMirrorLeftYaw)
                .arg(m_sideMirrorLeftPitch)
                .arg(m_sideMirrorRightYaw)
                .arg(m_sideMirrorRightPitch));
}

QJsonObject MainView::buildApplyPayload(const QJsonObject& extra) const
{
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

    for (auto it = extra.begin(); it != extra.end(); ++it)
        data.insert(it.key(), it.value());

    data.insert("ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    data.insert("source", "ui");
    return data;
}

// ===== IPC =====

// (값 즉시 적용은 이제 사용하지 않지만, 함수 시그니처 유지)
void MainView::sendApply(const QJsonObject& /*changes*/)
{
    if (m_initializing) return;

    // 버튼 기반 제어로 변경되었으므로, 여기선 아무 것도 하지 않음.
    // 값은 daemon의 CAN 수신 후 power/state로만 갱신됨.
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

        startSiren();

        QMetaObject::invokeMethod(this, [=]{
            auto* dlg = new WarningDialog(title, message, this);
            dlg->setGeometry((width()-400)/2, (height()-200)/2, 400, 200);
            connect(dlg, &QDialog::finished, this, [this](int){ stopSiren(); });
            dlg->show();
        }, Qt::QueuedConnection);
    }

    if (msg.topic == "user/update/sent" && (!m_updateReqId.isEmpty() && msg.reqId == m_updateReqId)) {
        const bool ok = msg.payload.value("ok").toBool(false);
        qInfo() << "[user/update/sent]" << ok << msg.payload;
    }

    if (msg.topic == "user/update/ack") {
        const int idx   = msg.payload.value("index").toInt(-1); // 1:Seat 2:Mirror 3:Wheel
        const int state = msg.payload.value("state").toInt(-1); // 0:OK 1:PartialMissing
        auto nameByIdx = [](int i)->QString {
            switch(i){ case 1: return "Seat"; case 2: return "Mirror"; case 3: return "Wheel"; }
            return "Unknown";
        };
        auto stateText = [](int s)->QString {
            switch(s){ case 0: return "OK"; case 1: return "DATA_PARTIAL"; }
            return "UNKNOWN";
        };
        qInfo() << "[user/update/ack]" << nameByIdx(idx) << stateText(state) << msg.payload;
    }

    // ===== 여기서만 값 갱신 =====
    if (msg.topic == "power/state") {
        const auto &p = msg.payload;

        // Seat
        if (p.contains("seatPosition"))     m_seatPosition     = clampInt(p.value("seatPosition").toInt(), SEAT_POS_MIN,   SEAT_POS_MAX);
        if (p.contains("seatAngle"))        m_seatAngle        = clampInt(p.value("seatAngle").toInt(),    SEAT_ANGLE_MIN, SEAT_ANGLE_MAX);
        if (p.contains("seatFrontHeight"))  m_seatFrontHeight  = clampInt(p.value("seatFrontHeight").toInt(), SEAT_HT_MIN, SEAT_HT_MAX);
        if (p.contains("seatRearHeight"))   m_seatRearHeight   = clampInt(p.value("seatRearHeight").toInt(),  SEAT_HT_MIN, SEAT_HT_MAX);

        // Side Mirror
        if (p.contains("sideMirrorLeftYaw"))    m_sideMirrorLeftYaw    = clampInt(p.value("sideMirrorLeftYaw").toInt(),   ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("sideMirrorLeftPitch"))  m_sideMirrorLeftPitch  = clampInt(p.value("sideMirrorLeftPitch").toInt(), ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("sideMirrorRightYaw"))   m_sideMirrorRightYaw   = clampInt(p.value("sideMirrorRightYaw").toInt(),  ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("sideMirrorRightPitch")) m_sideMirrorRightPitch = clampInt(p.value("sideMirrorRightPitch").toInt(),ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);

        // Room Mirror
        if (p.contains("roomMirrorYaw"))   m_roomMirrorYaw   = clampInt(p.value("roomMirrorYaw").toInt(),   ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("roomMirrorPitch")) m_roomMirrorPitch = clampInt(p.value("roomMirrorPitch").toInt(), ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);

        // Wheel
        if (p.contains("handlePosition"))  m_handlePosition = clampInt(p.value("handlePosition").toInt(), HANDLE_POS_MIN, HANDLE_POS_MAX);
        if (p.contains("handleAngle"))     m_handleAngle    = clampInt(p.value("handleAngle").toInt(),    ANGLE_HANDLE_MIN, ANGLE_HANDLE_MAX);

        updateHandleLabel();
        updateSeatLabel();
        updateRoomMirrorLabel();
        updateSideMirrorLabel();
        syncDashboard();
    }
    if (msg.topic == "state/seat") {
        const auto &p = msg.payload;
        if (p.contains("seatPosition"))     m_seatPosition     = clampInt(p.value("seatPosition").toInt(), SEAT_POS_MIN,   SEAT_POS_MAX);
        if (p.contains("seatAngle"))        m_seatAngle        = clampInt(p.value("seatAngle").toInt(),    SEAT_ANGLE_MIN, SEAT_ANGLE_MAX);
        if (p.contains("seatFrontHeight"))  m_seatFrontHeight  = clampInt(p.value("seatFrontHeight").toInt(), SEAT_HT_MIN, SEAT_HT_MAX);
        if (p.contains("seatRearHeight"))   m_seatRearHeight   = clampInt(p.value("seatRearHeight").toInt(),  SEAT_HT_MIN, SEAT_HT_MAX);
        updateSeatLabel(); syncDashboard();
    }
    if (msg.topic == "state/mirror") {
        const auto &p = msg.payload;
        if (p.contains("sideMirrorLeftYaw"))    m_sideMirrorLeftYaw    = clampInt(p.value("sideMirrorLeftYaw").toInt(),   ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("sideMirrorLeftPitch"))  m_sideMirrorLeftPitch  = clampInt(p.value("sideMirrorLeftPitch").toInt(), ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("sideMirrorRightYaw"))   m_sideMirrorRightYaw   = clampInt(p.value("sideMirrorRightYaw").toInt(),  ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("sideMirrorRightPitch")) m_sideMirrorRightPitch = clampInt(p.value("sideMirrorRightPitch").toInt(),ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("roomMirrorYaw"))        m_roomMirrorYaw        = clampInt(p.value("roomMirrorYaw").toInt(),       ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        if (p.contains("roomMirrorPitch"))      m_roomMirrorPitch      = clampInt(p.value("roomMirrorPitch").toInt(),     ANGLE_MIRROR_MIN, ANGLE_MIRROR_MAX);
        updateSideMirrorLabel(); updateRoomMirrorLabel(); syncDashboard();
    }
    if (msg.topic == "state/wheel") {
        const auto &p = msg.payload;
        if (p.contains("handlePosition"))  m_handlePosition = clampInt(p.value("handlePosition").toInt(), HANDLE_POS_MIN, HANDLE_POS_MAX);
        if (p.contains("handleAngle"))     m_handleAngle    = clampInt(p.value("handleAngle").toInt(),    ANGLE_HANDLE_MIN, ANGLE_HANDLE_MAX);
        updateHandleLabel(); syncDashboard();
    }
}

void MainView::onSaveClicked()
{
    if (m_initializing) return;

    // 저장은 기존 프로필 업데이트 경로 유지
    QJsonObject payload = buildApplyPayload();
    m_updateReqId = m_ipc->send("user/update", payload);

    const QByteArray compact = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    qInfo() << "[user/update] sent reqId=" << m_updateReqId << compact;
}

void MainView::sendDriveCommand(bool start)
{
    if (!m_ipc) return;
    QJsonObject payload{
        {"state", start ? "drive" : "stop"},
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"source", "ui"}
    };
    m_driveReqId = m_ipc->send("tcu/drive", payload);
    qInfo() << "[tcu/drive] sent reqId=" << m_driveReqId << payload;
}
