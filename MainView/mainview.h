#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QWidget>
#include <QDebug>
#include <QButtonGroup>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QStackedWidget>
namespace Ui {
class MainView;
}

class MainView : public QWidget
{
    Q_OBJECT

public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView();

private:
    Ui::MainView *ui;
    void sendHello();
    QButtonGroup* m_group;
    QTimer* m_clockTimer = nullptr;
        void updateClock();
        bool m_animating = false;
        void fadeToPage(QStackedWidget* stack, int nextIndex, int durationMs = 180);

private slots:
    void on_pushButton_clicked();

private:
    int m_seatTiltDeg   = 50;   
    int m_sideL         = 0;    
    int m_sideR         = 0;    
    int m_foreAft       = 0;    
    int m_backMirrorDeg = 0;  
};

#endif // MAINVIEW_H
