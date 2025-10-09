#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QWidget>
#include <QDebug>
#include <QButtonGroup>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QStackedWidget>
#include <QJsonObject>
#include "../ipc_client.h"
#include <QMediaPlayer>
#include <QMediaPlaylist>
namespace Ui {
class MainView;
}

class MainView : public QWidget {
    Q_OBJECT
public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView();

public slots:
    void loadInitialProfile(const QJsonObject& profile);
    void updateClock();
    void on_pushButton_clicked();          // (테스트 버튼: 필요시 삭제)
    void onIpcMessage(const IpcMessage& msg);

private:
    void onSaveClicked();
    // UI 전환/동기화
    void fadeToPage(QStackedWidget* stack, int nextIndex, int durationMs);
    void syncDashboard();

    // 페이지별 표시 라벨 업데이트
    void updateHandleLabel();
    void updateRoomMirrorLabel();
    void updateSeatLabel();
    void updateSideMirrorLabel();

    // IPC 송신
    void sendApply(const QJsonObject& changes);

    // 버튼 하이라이트 유틸
    void initButtonAsToggle(QAbstractButton* btn);
    void highlightChecked(QButtonGroup* group);

    QJsonObject buildApplyPayload(const QJsonObject& extra = {}) const;

private:
    Ui::MainView *ui = nullptr;
    QTimer* m_clockTimer = nullptr;
    bool m_initializing = false;

    // Seat
    int m_seatPosition = 0;      // 0~100 (%)
    int m_seatAngle = 0;         // 0~180 (deg)
    int m_seatFrontHeight = 0;   // 0~100 (%)
    int m_seatRearHeight = 0;    // 0~100 (%)

    // Side Mirror (Left/Right × Yaw/Pitch)
    int m_sideMirrorLeftYaw   = 0;  // -45~45 (deg)
    int m_sideMirrorLeftPitch = 0;  // -45~45 (deg)
    int m_sideMirrorRightYaw  = 0;  // -45~45 (deg)
    int m_sideMirrorRightPitch= 0;  // -45~45 (deg)

    // Room Mirror
    int m_roomMirrorYaw   = 0;      // -45~45 (deg)
    int m_roomMirrorPitch = 0;      // -45~45 (deg)

    // Handle
    int m_handlePosition = 0;    // 0~100 (arbitrary unit)
    int m_handleAngle    = 0;    // -90~90 (deg)

    // 메인 네비 및 서브 버튼 그룹
    QButtonGroup* m_groupMain = nullptr;       // btnHandle/btnSideMirror/btnSeat/btnRoomMirror
    QButtonGroup* m_groupHandle = nullptr;     // btnHandleAngle/btnHandlePosition
    QButtonGroup* m_groupRoom = nullptr;       // btnRoomMirrorPitch/btnRoomMirrorYaw
    QButtonGroup* m_groupSeat = nullptr;       // btnSeatPosition/btnSeatAngle/btnSeatFront/btnSeatRear
    QButtonGroup* m_groupSide = nullptr;       // btnSideMirrorYaw/btnSideMirrorPitch

    // IPC
    IpcClient* m_ipc = nullptr;
    QString m_powerReqId;

    QMediaPlayer*   m_sirenPlayer = nullptr;
    QMediaPlaylist* m_sirenList   = nullptr;
    void startSiren();
    void stopSiren();

    QString m_updateReqId;
};

#endif // MAINVIEW_H
