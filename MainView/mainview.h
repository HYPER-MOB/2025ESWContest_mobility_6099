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

namespace Ui {
class MainView;
}

class MainView : public QWidget {
    Q_OBJECT
public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView();

private slots:
    void updateClock();
    void on_pushButton_clicked();
    void onIpcMessage(const IpcMessage& msg);

private:
    void fadeToPage(QStackedWidget* stack, int nextIndex, int durationMs);
    void sendApply(const QJsonObject& changes);
    void updateSideMirrorLabel();

private:
    Ui::MainView *ui;
    QTimer* m_clockTimer = nullptr;

    int m_seatTiltDeg   = 0;
    int m_sideL         = 0;
    int m_sideR         = 0;
    int m_foreAft       = 0;
    int m_backMirrorDeg = 0;

    QButtonGroup* m_group = nullptr;

    IpcClient* m_ipc = nullptr;
    QString m_powerReqId;
};

#endif // MAINVIEW_H
