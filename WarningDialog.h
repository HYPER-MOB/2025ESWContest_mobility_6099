#ifndef WARNINGDIALOG_H
#define WARNINGDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

class WarningDialog : public QDialog {
    Q_OBJECT
public:
    explicit WarningDialog(const QString& title, const QString& message, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(title);
        setModal(true);
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint); // 테두리 제거, 중앙 배치용
        setAttribute(Qt::WA_DeleteOnClose);

        auto* layout = new QVBoxLayout(this);

        m_label = new QLabel(message, this);
        m_label->setAlignment(Qt::AlignCenter);
        m_label->setStyleSheet("QLabel { color: red; font-size: 20pt; font-weight: bold; }");
        layout->addWidget(m_label);

        auto* btn = new QPushButton("확인", this);
        connect(btn, &QPushButton::clicked, this, &WarningDialog::accept);
        layout->addWidget(btn, 0, Qt::AlignCenter);

        // 깜빡임 효과 (red ↔ transparent)
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this] {
            if (m_blink) {
                m_label->setStyleSheet("QLabel { color: red; font-size: 20pt; font-weight: bold; }");
            } else {
                m_label->setStyleSheet("QLabel { color: transparent; font-size: 20pt; font-weight: bold; }");
            }
            m_blink = !m_blink;
        });
        m_timer->start(500); // 0.5초마다 깜빡임
    }

private:
    QLabel* m_label;
    QTimer* m_timer;
    bool m_blink = false;
};

#endif // WARNINGDIALOG_H
