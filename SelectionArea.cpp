#include "SelectionArea.h"
#include <qpainter.h>

SelectionArea::SelectionArea(QWidget* parent) : QWidget(parent)
{
	setAttribute(Qt::WA_TransparentForMouseEvents, true);
	setAttribute(Qt::WA_AlwaysStackOnTop, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
	this->reset();
}
void SelectionArea::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.save();
	p.fillRect(rect(), backgroundColor);
	p.setPen(QPen(borderColor, borderWidth));
	p.drawRect(rect());
	p.restore();
	QWidget::paintEvent(e);
}
#include <qevent.h>
void SelectionArea::resizeEvent(QResizeEvent* e)
{
	emit resized(QRect(geometry().topLeft(), e->size()));
}
