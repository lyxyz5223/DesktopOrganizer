#pragma once
#include <qwidget.h>
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
signals:
    //void resized(QResizeEvent* e);
    void resized();
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);

private:
};

