#include "SelectionArea.h"
#include <qpainter.h>
#include <qevent.h>

SelectionArea::SelectionArea(QWidget* parent) : QWidget(parent)
{
	resize(0, 0);
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_AlwaysStackOnTop, true);
	setAttribute(Qt::WA_TransparentForMouseEvents, true);
	setAttribute(Qt::WA_PaintOnScreen, true);


}

void SelectionArea::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(10, 123, 212, 100));
	p.setPen(QColor(10, 123, 212));
	p.drawRect(rect());
	QWidget::paintEvent(e);
}

void SelectionArea::resizeEvent(QResizeEvent* e)
{
	//if(!e->size().isNull())
		//emit resized(e);
}
