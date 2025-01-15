//Qt
#include "MyFileListItem.h"
#include <QPainter>
#include <QFontMetrics>
#include <QMenu>
#include <qevent.h>

//C++
#include <iostream>
#include "stringProcess.h"

//Windows
#include <Windows.h>

QString elidedMultiLinesText(QWidget* widget, QString text, int lines, Qt::TextElideMode ElideMode);

MyFileListItem::MyFileListItem(QWidget* parent, QSize defaultSize) : QPushButton(parent)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint);
	setMouseTracking(true);
	installEventFilter(this);
	//itemTextSize.setHeight(25);
	setCheckable(true);
	setAutoExclusive(true);
	if (!defaultSize.isEmpty())// isEmpty():Returns true if either of the width and height is less than or equal to 0; otherwise returns false.
	{
		// defaultSize的width和height均不为0时设置
		resize(defaultSize);
	}
	QString Qtext = text();
	if (Qtext.right(4) == ".lnk" || Qtext.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
		Qtext = Qtext.left(Qtext.size() - 4);
	QFontMetrics fontMetrics1(font());
	int fontHeight = fontMetrics1.size(0, Qtext).height();
	itemTextSize.setHeight(fontHeight);
	connect(this, &MyFileListItem::doubleClicked, this, [=]() {
#ifdef _DEBUG
		std::cout << UTF8ToANSI(wstr2str_2UTF8(getPath()) + text().toStdString()).c_str() << std::endl;
#endif // _DEBUG
		ShellExecute(0, L"open", (getPath() + text().toStdWString()).c_str(), L"", 0, SW_NORMAL);
		});
	connect(this, &MyFileListItem::removeSelfSignal, this, &MyFileListItem::removeSelfSlot);
	connect(this, &MyFileListItem::moveSignal, this, [=](QPoint pos) { move(pos); });
}

void MyFileListItem::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	QPen pen;
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);//抗锯齿
	qreal xr, yr;
	xr = yr = (size().width() > size().height() ? size().height() / 5 : size().width() / 5);
	QRect qrect1;
	qrect1 = this->rect();
	//qrect1.setWidth(qrect1.width() - 1);
	//qrect1.setHeight(qrect1.height() - 1);
	p.setPen(Qt::NoPen);
	if (bgBrush != bgBrush_MouseMove)
		if (isChecked())
			bgBrush = bgBrush_Selected;
		else if (!isChecked())
			bgBrush = bgBrush_Default;
	p.setBrush(bgBrush);
	//p.setBrush(QBrush(QColor(100, 100, 119, 100)));
	p.drawRoundedRect(qrect1, xr, yr, Qt::SizeMode::AbsoluteSize);
	QString Qtext = text();
	if (Qtext.right(4) == ".lnk" || Qtext.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
		Qtext = Qtext.left(Qtext.size() - 4);
	QTextOption qto;
	switch (viewMode)
	{
	default:
	case ViewMode::Icon:
	{
		qto.setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
		pen.setColor(QColor(0, 0, 0, 100));
		p.setPen(pen);
		/*文字处理*/
		Qtext = elidedMultiLinesText(this, Qtext, 1, Qt::ElideRight);
		QFontMetrics fontMetrics1(font());
		int textHeight = fontMetrics1.size(0, Qtext).height();//两行(如有，否则为一行)的总高度
		itemTextSize.setHeight(textHeight);
		qrect1 = rect();
		/*图标处理并绘制*/
		QRect imageRect;
		double zoom = 3.0 / 4;//图标的缩放比例
		QSize itemImageZoneSize = size();
		itemImageZoneSize.setHeight(itemImageZoneSize.height() - textHeight);
		if (itemImageZoneSize.width() <= itemImageZoneSize.height())
		//宽小，用宽度计算
			imageRect.setSize(QSize(itemImageZoneSize.width() * zoom, itemImageZoneSize.width() * zoom));
		else
		//高大，用高度计算
			imageRect.setSize(QSize(itemImageZoneSize.height() * zoom, itemImageZoneSize.height() * zoom));
		imageRect.moveTo(QPoint((itemImageZoneSize.width() - imageRect.width()) / 2, (itemImageZoneSize.height() - imageRect.height()) / 2));

		//QRect imageRect((width() - width() * zoom) / 2,
		//	(width() - width() * zoom) / 2,
		//	width() * zoom,
		//	width() * zoom);
		//p.drawRect(imageRect);//正方形边框
		//if (itemImage.width() >= itemImage.height())//宽大高小
		//	imageRect.setSize(QSize(width() * zoom, width() * zoom * itemImage.height() / itemImage.width()));
		//else//宽小高大
		//	imageRect.setSize(QSize(width() * zoom * itemImage.width() / itemImage.height(), width() * zoom));
		//imageRect.setRect((width() - imageRect.width()) / 2,
		//	(width() + width() * zoom) / 2 - imageRect.height(),
		//	imageRect.width(), imageRect.height());
		
		p.drawRect(imageRect);//正方形边框
		p.drawImage(imageRect, itemImage);
	}
		break;
	case ViewMode::List:
	{
		qto.setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
		qrect1.setX(itemImage.width());
		qrect1.setWidth(size().width() - iconSize().width());
	}
		break;
	}
	pen.setColor(QColor(0, 0, 0, 255));
	p.setPen(pen);
	p.drawText(qrect1, Qtext,qto);
	QWidget::paintEvent(e);
	//QPushButton::paintEvent(e);
}
void MyFileListItem::mouseDoubleClickEvent(QMouseEvent* e)
{
	//MessageBox(0, L"DoubleClicked!!!", L"", 0);
	emit doubleClicked();
}

bool MyFileListItem::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::Enter)
		bgBrush = bgBrush_MouseMove;
	else if (event->type() == QEvent::Leave && !isChecked())
		bgBrush = bgBrush_Default;
	else if (event->type() == QEvent::Leave && isChecked())
		bgBrush = bgBrush_Selected;

	return false;
}

void MyFileListItem::setViewMode(ViewMode View_Mode)
{
	viewMode = View_Mode;
}
void MyFileListItem::adjustSize() {
	QString Qtext = text();
	if (Qtext.right(4) == ".lnk" || Qtext.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
		Qtext = Qtext.left(Qtext.size() - 4);
	QFontMetrics fontMetrics1(font());
	Qtext = elidedMultiLinesText(this, Qtext, 1, Qt::ElideRight);
	int fontHeight = fontMetrics1.size(Qt::TextExpandTabs, Qtext).height();
	itemTextSize.setHeight(fontHeight);
	switch (viewMode)
	{
	default:
	case ViewMode::Icon:
		resize(itemImage.width() + 2 * fontMetrics1.averageCharWidth(), itemImage.height() + fontHeight);
		break;
	case ViewMode::List:
		resize(parentWidget()->width(), 2 * fontHeight);
		break;
	}
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
					break;
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

void MyFileListItem::mousePressEvent(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::MouseButton::RightButton:
	{
		QMenu* menu1 = new QMenu(this);
		menu1->addAction(QIcon(), "打开");
		menu1->addSeparator();
		menu1->addAction(QIcon(), "打开方式");
		menu1->addAction(QIcon(), "刷新");
		menu1->addAction(QIcon(), "删除");
		connect(menu1, SIGNAL(triggered(QAction*)), this, SLOT(MenuClickedProc(QAction*)));
		menu1->exec(QCursor::pos());
		break;
	}
	case Qt::MouseButton::LeftButton:
		bgBrush = bgBrush_Selected;
		setChecked(true);
		emit selected();
		update();
		break;
	default:
		break;
	}
}
void MyFileListItem::mouseReleaseEvent(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::MouseButton::LeftButton:
		break;
	default:
		break;
	}
}

void MyFileListItem::MenuClickedProc(QAction* action)
{
	if (action->text() == "打开")       
		emit doubleClicked();
	else if (action->text() == "删除")
		emit removeSelfSignal();
}

void MyFileListItem::removeSelfSlot()
{
	//MessageBox(0, (getPath() + text().toStdWString()).c_str(), 0, 0);
	std::wstring nameWithPath = getPath() + text().toStdWString();
	WCHAR* CNameWithPath = new WCHAR[nameWithPath.size() + 2];
	for (size_t i = 0; i < nameWithPath.size(); i++)
		CNameWithPath[i] = nameWithPath[i];
	CNameWithPath[nameWithPath.size()] = L'\0';
	CNameWithPath[nameWithPath.size() + 1] = L'\0';
	SHFILEOPSTRUCT fileOpStruct;
	ZeroMemory(&fileOpStruct, sizeof(fileOpStruct));
	fileOpStruct.fFlags = FOF_ALLOWUNDO;
	fileOpStruct.wFunc = FO_DELETE;
	fileOpStruct.pFrom = CNameWithPath;
	int fileOpResult = SHFileOperation(&fileOpStruct);
	std::wcout << L"用户删除：" << fileOpStruct.pFrom << std::endl;
	if (fileOpResult)
	{
		std::wcout << L"删除失败！删除文件：" << fileOpStruct.pFrom << L"时出现问题，错误代码：" << fileOpResult << std::endl;
		MessageBox(0, (L"删除失败！\n删除" + std::wstring() + fileOpStruct.pFrom + std::wstring() + L"时出现问题，\n错误代码：" + std::to_wstring(fileOpResult)).c_str(), L"error", 0);
	}
	//MessageBox(0, fileOpStruct.pFrom, (L"函数返回："+std::to_wstring(fileOpResult)).c_str(), 0);
	delete[] CNameWithPath;
	//_wremove((getPath() + L"\\" + text().toStdWString()).c_str());
}