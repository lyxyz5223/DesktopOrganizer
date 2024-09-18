#include "MyFileListItem.h"
#include <QPainter>
#include <QFontMetrics>

QString elidedMultiLinesText(QWidget* widget, QString text, int lines, Qt::TextElideMode ElideMode);

MyFileListItem::MyFileListItem(QWidget* parent) : QPushButton(parent)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint);
	//itemTextSize.setHeight(25);
	resize(0, 0);
}

void MyFileListItem::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	QPen pen;
	//p.fillRect(this->rect(), QColor(255, 255, 255, 0));
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);//抗锯齿
	qreal xr, yr;
	xr = yr = (size().width() > size().height() ? size().height() / 5 : size().width() / 5);
	p.setBrush(QBrush(QColor(100, 100, 119, 255)));
	p.setPen(Qt::NoPen);
	QRect qrect1;
	qrect1 = this->rect();
	//qrect1.setWidth(qrect1.width() - 1);
	//qrect1.setHeight(qrect1.height() - 1);
	p.drawRoundedRect(qrect1, xr, yr, Qt::SizeMode::AbsoluteSize);
	QString Qtext = text();
	QTextOption qto;
	if (viewMode == ViewMode::Icon)
	{
		qto.setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
		pen.setColor(QColor(0, 0, 0,100));
		p.setPen(pen);
		double 缩放 = 3.0 / 4;
		p.drawRect(QRect((width() - width() * 缩放) / 2, (width() - width() * 缩放) / 2, size().width() * 缩放, size().width() * 缩放));
		p.drawImage(QRect((width() - width()* 缩放) / 2, (width() - width()* 缩放) / 2, size().width() * 缩放, size().width() * 缩放), itemImage);
		Qtext = elidedMultiLinesText(this, Qtext, 2, Qt::ElideRight);
		QFontMetrics fontMetrics1(font());
		int fontHeight = fontMetrics1.size(0, Qtext).height();
		itemTextSize.setHeight(fontHeight);
		if (viewMode == ViewMode::Icon)
			resize(MyIconSize, itemTextSize.height() + MyIconSize);
		qrect1 = rect();
	}
	else
	{
		
		qto.setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
		qrect1.setX(MyIconSize);
		qrect1.setWidth(size().width() - iconSize().width());
	}
	pen.setColor(QColor(0, 0, 0, 255));
	p.setPen(pen);
	p.drawText(qrect1, Qtext,qto);
	QWidget::paintEvent(e);
	//QPushButton::paintEvent(e);
}
#include <Windows.h>
void MyFileListItem::mouseDoubleClickEvent(QMouseEvent* e)
{
	MessageBox(0, L"DoubleClicked!!!", L"", 0);
	emit doubleClicked();
}

void MyFileListItem::setViewMode(ViewMode View_Mode)
{
	viewMode = View_Mode;
	if (size().isNull())
	{
		adjustSize();
	}
}
void MyFileListItem::adjustSize() {
	QString Qtext = text();
	QFontMetrics fontMetrics1(font());
	Qtext = elidedMultiLinesText(this, Qtext, 2, Qt::ElideRight);
	int fontHeight = fontMetrics1.size(0, Qtext).height();
	itemTextSize.setHeight(fontHeight);
	if (viewMode == ViewMode::Icon)
		resize(MyIconSize, itemTextSize.height() + MyIconSize);
}

QString elidedMultiLinesText(QWidget* widget,QString text, int lines, Qt::TextElideMode ElideMode)
{
	if (lines <= 0)
		return "";
	else if (lines == 1)
	{
		QFontMetrics fontMetrics1(widget->font());
		return fontMetrics1.elidedText(text, ElideMode, widget->width());
	}
	else {
		QStringList strList;
		QFontMetrics fontMetrics1(widget->font());
		for (int i = 1; i <= text.size(); i++)
		{
			if (fontMetrics1.size(0, text.left(i)).width() > widget->width())
			{
				strList.append(text.left(i - 1));
				text = text.right(text.size() - i + 1);
				if (strList.size() == lines)
				{
					break;
				}
				i = 0;
			}
		}
		if (strList.size() < lines)
		{
			strList.append(text);
			text = "";
		}
		if (text.size() > 0)
		{
			//执行到这里说明需要省略文字
			QString ElideText = "...";
			QString strLast= strList.last();
			strLast.remove(strLast.size() - ElideText.size(), ElideText.size());
			strLast.append(ElideText);
			//方法一：
			//strList.removeLast();
			//strList.append(strLast);
			//方法二：
			strList.replace(strList.size() - 1, strLast);
		}
		return strList.join("\n");
	}
}
