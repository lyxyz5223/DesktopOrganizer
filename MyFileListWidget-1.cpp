#include "MyFileListWidget-1.h"
#include <QPainter>


MyFileListWidget::MyFileListWidget(QWidget* parent)// : QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint);
	//setAttribute(Qt::WA_PaintOnScreen);
	this->setStyleSheet(".QLabel{border: 3px solid #000000;}");

}
#include <Windows.h>
void MyFileListWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(this->rect(), QColor(255,255,255, 0));
	//if (iter == itemsList.end())
	//{	//}
	QWidget::paintEvent(e);
}

void MyFileListWidget::addItem(MyFileListItem* item,std::string id)
{
	item->setParent(this);
	itemsList[id] = item;
	XCoordinate[id] = 0;
	YCoordinate[id] = 0;
	item->setViewMode(viewMode);
	QSize SelfSize = this->size();
	std::map<std::string, MyFileListItem*>::iterator iter;
	std::string nowId = "";
	std::string lastId = "";
	intptr_t nowCoordX;
	intptr_t nowCoordY;
	intptr_t maxWidth = 0;
	nowCoordX = HorizontalSpacing;
	nowCoordY = VerticalSpacing;
	for (iter = itemsList.begin(); iter != itemsList.end(); iter++)
	{
		MyFileListItem* item1 = iter->second;
		item1->setViewMode(viewMode);//setVM(item1);
		QSize itemSize = item1->size();
		int item1Width = item1->size().width();
		if (item1Width > maxWidth)
			maxWidth = item1Width;
		if (nowCoordY >= this->size().height() - item1->size().height() - VerticalSpacing)
		{
			nowCoordX += maxWidth + HorizontalSpacing;
			//nowCoordX += itemsList[nowId]->size().width() + HorizontalSpacing;
			nowCoordY = VerticalSpacing;
			maxWidth = 0;
		}
		item1->move(nowCoordX, nowCoordY);
		nowId = iter->first;
		XCoordinate[nowId] = nowCoordX;
		YCoordinate[nowId] = nowCoordY;
		nowCoordY += item1->geometry().height() + VerticalSpacing;

	}

}
void MyFileListWidget::deleteItem(std::string id)
{
	itemsList.erase(id);
}

MyFileListItem* MyFileListWidget::getItemById(std::string id)
{
	return itemsList[id];
}
