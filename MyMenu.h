#pragma once
#include <qmenu.h>
#include <qpropertyanimation.h>
#include <MyMenuAction.h>

class MyMenu :
    public QMenu
{
public:
    ~MyMenu() {}
    MyMenu(QWidget* parent = nullptr);



    bool hasChildMenu() const {
        auto actions = this->actions();
        for (auto action : actions)
        {
            if (action->menu())
                return true;
        }
        return false;
    }
    bool hasIcon() const {
        auto actions = this->actions();
        for (auto action : actions)
        {
            if (!action->icon().isNull())
                return true;
        }
        return false;
    }
    MyMenuAction* actionAt(const QPoint& pos) const {
        return (MyMenuAction*)QMenu::actionAt(pos);
    }
    bool eventFilter(QObject* watched, QEvent* e) override;
    //bool close() override {

    //}
public slots:
    void startCloseAnimation();
    void startShowAnimation();

protected:
    void paintEvent(QPaintEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    bool event(QEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    bool hasPlayShowAni = false;
    bool hasPlayCloseAni = false;
};

