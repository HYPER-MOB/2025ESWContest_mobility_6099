#pragma once
#include <QWidget>
#include <QTimer>

namespace Ui { class DataCheckWindow; }

class DataCheckWindow : public QWidget
{
    Q_OBJECT
public:
    explicit DataCheckWindow(QWidget *parent = nullptr);
    ~DataCheckWindow();

    Q_INVOKABLE void begin(bool hasData);

signals:
    void dataCheckFinished();

private slots:
    void advance();

private:
    enum class Phase {
        None,
        Checking,        
        DataFoundMsg,   
        NoDataMsg,       
        Measuring,       
        Calculating,   
        Applying,        
        Done
    };

    void setMessage(const QString& text, bool busy);
    void setDeterminate(int v); 

private:
    Ui::DataCheckWindow *ui;
    QTimer m_timer;
    Phase  m_phase = Phase::None;
    bool   m_hasData = false;
};
