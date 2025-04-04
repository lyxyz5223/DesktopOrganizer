//Qt
#include "MyFileListItem.h"
#include <QPainter>
#include <QFontMetrics>
#include <QMenu>
#include <qevent.h>
#include <qdrag.h>
#include <qmimedata.h>
#include <qapplication.h>

//C++
#include <iostream>
#include "StringProcess.h"

//Windows
#include <Windows.h>

QString elidedMultiLinesText(QWidget* widget, QString text, int lines, Qt::TextElideMode ElideMode);
void MyFileListItem::initialize(QWidget* parent, QSize defaultSize, bool isShadow)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
	setMouseTracking(true);
	installEventFilter(this);
	//itemTextSize.setHeight(25);
	setCheckable(true);
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
	connect(this, &MyFileListItem::adjustSizeSignal, this, &MyFileListItem::adjustSize);
}
MyFileListItem::MyFileListItem(QWidget* parent, QSize defaultSize) : QPushButton(parent)
{
	initialize(parent, defaultSize, false);//初始化
}

MyFileListItem::MyFileListItem(const MyFileListItem& item, bool isShadow) : QPushButton(item.parentWidget())
{
	if (!isShadow)
		initialize(item.parentWidget(), item.size(), true);//初始化函数
	else
	{
		setAttribute(Qt::WA_TransparentForMouseEvents, true);
		setAttribute(Qt::WA_InputMethodTransparent, true);
		setWindowFlags(windowFlags() | Qt::WindowTransparentForInput);
	}

	// 复制原有成员
	viewMode = item.viewMode;
	itemImage = item.itemImage;
	itemTextSize = item.itemTextSize;
	MyPath = item.MyPath;
	//startPosOffset = item.startPosOffset;
	if (isShadow)
	{
		bgBrush = item.bgBrush_Shadow;//background
		isShadowItem = true;
		setWindowOpacity(0.6);//无效
	}
	else
		bgBrush = item.bgBrush;
	setText(item.text());
	resize(item.size());

}
void MyFileListItem::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	QPen pen;
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);//抗锯齿
	qreal xr, yr;
	xr = yr = (size().width() > size().height() ? size().height() / 7 : size().width() / 7);
	QRect qrect1;
	qrect1 = this->rect();
	//qrect1.setWidth(qrect1.width() - 1);
	//qrect1.setHeight(qrect1.height() - 1);
	p.setPen(Qt::NoPen);
	if (bgBrush != bgBrush_MouseMove)
	{
		if (isChecked())
			bgBrush = bgBrush_Selected;
		else if (!isChecked())
			bgBrush = bgBrush_Default;
	}
	if (isShadowItem)
		bgBrush = bgBrush_Shadow;
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
		double zoom = iconZoom;//图标的缩放比例
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


bool MyFileListItem::eventFilter(QObject* watched, QEvent* event)
{

	if (event->type() == QEvent::Enter)
		bgBrush = bgBrush_MouseMove;
	else if (event->type() == QEvent::Leave && !isChecked())
		bgBrush = bgBrush_Default;
	else if (event->type() == QEvent::Leave && isChecked())
		bgBrush = bgBrush_Selected;
	return QPushButton::eventFilter(watched, event);
}
void MyFileListItem::onSelectionAreaResize()
{
	bool checked = true;
	//for (int i = 0; i < 2; i++)
	if (selectionArea)
	{
		QRect thisRect = geometry();
		QRect selectionAreaRect = selectionArea->geometry();
		if (thisRect.right() < selectionAreaRect.left()
			|| thisRect.left() > selectionAreaRect.right()
			|| thisRect.top() > selectionAreaRect.bottom()
			|| thisRect.bottom() < selectionAreaRect.top())
			checked = false;
	}
	if (checked)
		setChecked(true);
	else
		setChecked(false);
}
void MyFileListItem::adjustSize()
{
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
		disconnect(menu1, SIGNAL(triggered(QAction*)), this, SLOT(MenuClickedProc(QAction*)));

		break;
	}
	case Qt::MouseButton::LeftButton:
	{
		//bgBrush = bgBrush_Selected;
		//setChecked(true);
		//update();

		//QPoint p = mapToParent(e->pos());
		//selectionArea->move(p);
		//selectionArea->reset();
		QCursor cursor;
		QPoint startPos = cursor.pos();

		drag = new QDrag(this);
		drag->setHotSpot(e->pos());
		QImage dragImage(1, 1, QImage::Format_ARGB32);
		dragImage.fill(QColor(255, 255, 255, 0));
		drag->setPixmap(QPixmap::fromImage(dragImage));


		if (isChecked())
		{
			Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
			if (modifiers & Qt::ControlModifier)//是否按下Ctrl键
			{
				if (!isChecked())
					setChecked(true);
				else
					setChecked(false);
			}

			//如果已经选中，则准备拖动事宜
			if (dragArea)
			{
				dragArea->correctPosition();
				QPoint leftTop = relativePosTransition(nullptr, dragArea->pos(), parentWidget());
				startPosOffset.setX(startPos.x() - leftTop.x());
				startPosOffset.setY(startPos.y() - leftTop.y());
				dragArea->setCursorPosOffsetWhenMousePress(startPosOffset);

				//	dragArea->show();
				QList<QUrl> urls;
				const auto sels = dragArea->getSelectedItemsKeys();
				for (auto iter = sels.begin(); iter != sels.end(); iter++)
				{
					QUrl url = QUrl::fromLocalFile(QString::fromStdWString(*iter));
					urls.push_back(url);
				}
				QMimeData* mimeData = new QMimeData();
				mimeData->setUrls(urls);
				drag->setMimeData(mimeData);
			}
		}
		else
		{
			//未选中则选中（单选）
			Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
			if (modifiers & Qt::ControlModifier)//是否按下Ctrl键
			{
				if (!isChecked())
					setChecked(true);
				else
					setChecked(false);
			}
			else if (selectionArea)
			{
				selectionArea->move(mapToParent(e->pos()));
				selectionArea->reset();
			}
			if (dragArea)
			{
				startPosOffset.setX(startPos.x() - pos().x() + dragArea->getItemSpacing().column);
				startPosOffset.setY(startPos.y() - pos().y() + dragArea->getItemSpacing().line);
				dragArea->moveRelative(
					QPoint(
						pos().x() - dragArea->getItemSpacing().column,
						pos().y() - dragArea->getItemSpacing().line
					),
					parentWidget(), nullptr
					);
				dragArea->setCursorPosOffsetWhenMousePress(startPosOffset);

				QList<QUrl> urls;
				QUrl url = QUrl::fromLocalFile(QString::fromStdWString(this->MyPath) + this->text());
				urls.push_back(url);
				QMimeData* mimeData = new QMimeData();
				mimeData->setUrls(urls);
				drag->setMimeData(mimeData);
			}
		}
		//drag->exec(Qt::DropAction::CopyAction | Qt::DropAction::MoveAction | Qt::DropAction::LinkAction | Qt::DropAction::TargetMoveAction | Qt::DropAction::IgnoreAction);
	}
		break;
	default:
		break;
	}
}
void MyFileListItem::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::MouseButton::LeftButton)// 左键拖动
	{
		//if (shadowItem)
		//{
		//	shadowItem->show();
		//	QPoint pos(e->globalPosition().x(), e->globalPosition().y());
		//	//std::cout << pos.x() << "," << pos.y() << std::endl;
		//	shadowItem->move(pos.x() - startPosOffset.x(), pos.y() - startPosOffset.y());
		//}
		if (drag)
		{
			dragArea->show();
			drag->exec(Qt::DropAction::CopyAction | Qt::DropAction::MoveAction | Qt::DropAction::LinkAction | Qt::DropAction::TargetMoveAction | Qt::DropAction::IgnoreAction);
			drag->deleteLater();
			drag = nullptr;
		}
	}
}
void MyFileListItem::dragMoveEvent(QDragMoveEvent* e)
{

}
void MyFileListItem::dragEnterEvent(QDragEnterEvent* event)
{

}
void MyFileListItem::mouseReleaseEvent(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::MouseButton::LeftButton:
	{
		if (drag)
		{
			drag->deleteLater();
			drag = nullptr;
			//并未拖动，所以多选变为单选
			Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
			if (modifiers & Qt::ControlModifier)//是否按下Ctrl键
			{

			}
			else if (selectionArea)
			{
				selectionArea->move(mapToParent(e->pos()));
				selectionArea->reset();
			}
		}
		if(dragArea)
			dragArea->hide();
	}
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
	WCHAR* cNameWithPath = new WCHAR[nameWithPath.size() + 2];
	for (size_t i = 0; i < nameWithPath.size(); i++)
		cNameWithPath[i] = nameWithPath[i];
	cNameWithPath[nameWithPath.size()] = L'\0';
	cNameWithPath[nameWithPath.size() + 1] = L'\0';
	SHFILEOPSTRUCT fileOpStruct;
	ZeroMemory(&fileOpStruct, sizeof(fileOpStruct));
	fileOpStruct.fFlags = FOF_ALLOWUNDO;
	fileOpStruct.wFunc = FO_DELETE;
	fileOpStruct.pFrom = cNameWithPath;
	int fileOpResult = SHFileOperation(&fileOpStruct);
	std::wcout << L"用户删除：" << fileOpStruct.pFrom << std::endl;
	if (fileOpResult)
	{
		std::wcout << L"删除失败！删除文件：" << fileOpStruct.pFrom << L"时出现问题，错误代码：" << fileOpResult << std::endl;
		MessageBox(0, (L"删除失败！\n删除" + std::wstring() + fileOpStruct.pFrom + std::wstring() + L"时出现问题，\n错误代码：" + std::to_wstring(fileOpResult)).c_str(), L"error", 0);
	}
	//MessageBox(0, fileOpStruct.pFrom, (L"函数返回："+std::to_wstring(fileOpResult)).c_str(), 0);
	delete[] cNameWithPath;
	//_wremove((getPath() + L"\\" + text().toStdWString()).c_str());
}