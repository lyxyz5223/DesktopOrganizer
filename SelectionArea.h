#pragma once
#include <qwidget.h>
#include <thread>

class SelectionArea :
    public QWidget
{
    Q_OBJECT
public:
    ~SelectionArea() {}
    SelectionArea(QWidget* parent = nullptr);
    void resize(QSize size) {
        QWidget::resize(size);
        emit resized();
    }
    void resize(int x, int y) {
        resize(QSize(x, y));
    }
    void reset() {
        resize(0, 0);
    }
    void mouseMoveProc(QRect selectionRect) {
        if (asyncMouseMoveProcConnection)
            disconnect(asyncMouseMoveProcConnection);
        asyncMouseMoveProcConnection = connect(this, &SelectionArea::asyncMouseMoveProcSignal, this, [=]() {
            auto selectionArea = this;
            selectionArea->move((selectionRect.left() < selectionRect.right() ? selectionRect.left() : selectionRect.right()),
                (selectionRect.top() < selectionRect.bottom() ? selectionRect.top() : selectionRect.bottom()));
            selectionArea->resize(selectionRect.width() >= 0 ? selectionRect.width() : -selectionRect.width(),
                selectionRect.height() >= 0 ? selectionRect.height() : -selectionRect.height());
            selectionArea->update();
            });
        std::thread th([&]() { emit asyncMouseMoveProcSignal(); });
        th.detach();
    }
signals:
    //void resized(QResizeEvent* e);
    void resized();
    void asyncMouseMoveProcSignal();

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);

private:
    QMetaObject::Connection asyncMouseMoveProcConnection;
};

