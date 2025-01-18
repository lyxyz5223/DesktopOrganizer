#include "SelectionArea.h"
#include <qpainter.h>
SelectionArea::SelectionArea(QWidget* parent) : QWidget(parent)
{

}

void SelectionArea::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(10, 123, 212, 100));
	p.setPen(QColor(10, 123, 212));
	p.drawRect(rect());
	QWidget::paintEvent(e);
}
