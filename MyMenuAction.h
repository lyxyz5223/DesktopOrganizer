#pragma once
#include <qaction.h>

class MyMenuAction :
    public QAction
{
    Q_OBJECT
public:
    ~MyMenuAction() {}
    MyMenuAction(QObject* parent = nullptr);
    MyMenuAction(const QString& text, QObject* parent = nullptr)
        : QAction(text, parent) {}
    MyMenuAction(const QIcon& icon, const QString& text, QObject* parent = nullptr)
        : QAction(icon, text, parent) {}
    
    //没啥用
    void setTipText(QString text) {
        tipText = text;
    }
    QString getTipText() const {
        return tipText;
    }
protected:
    MyMenuAction(QActionPrivate& dd, QObject* parent) : QAction(dd, parent) {}
    bool event(QEvent* e) override;
private:
    QString tipText;
};

