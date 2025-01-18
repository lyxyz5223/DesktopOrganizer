#pragma once
#include <qwidget.h>
class SelectionArea :
    public QWidget
{
public:
    ~SelectionArea() {}
    SelectionArea(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* e);

private:
};

