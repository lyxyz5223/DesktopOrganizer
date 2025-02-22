#include "MyMenuAction.h"
#include <qevent.h>
#include <QPainter>
#include <qwidget.h>

MyMenuAction::MyMenuAction(QObject* parent) : QAction(parent)
{

}

bool MyMenuAction::event(QEvent* e)
{
	
	return false;
}
