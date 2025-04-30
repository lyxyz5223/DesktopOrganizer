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
#include <qvalidator.h>

QString elidedMultiLinesText(QWidget* widget, QString text, int lines, Qt::TextElideMode ElideMode);
void MyFileListItem::initialize(QWidget* parent, QSize defaultSize)
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
	connect(this, &MyFileListItem::doubleClicked, this, [=]() {
#ifdef _DEBUG
			std::cout << UTF8ToANSI(wstr2str_2UTF8(getPath()) + text().toStdString()).c_str() << std::endl;
#endif // _DEBUG
			ShellExecute(0, L"open", (getPath() + text().toStdWString()).c_str(), L"", 0, SW_NORMAL);
		});
	connect(this, &MyFileListItem::moveSignal, this, [&](QPoint pos) { move(pos); });
	connect(this, &MyFileListItem::resizeSignal, this, [&](QSize size) { resize(size); });
	connect(this, &MyFileListItem::adjustSizeSignal, this, &MyFileListItem::adjustSize);

	//文本编辑框的延迟启动
	edit->hide();
	QRegularExpressionValidator* validator = new QRegularExpressionValidator(QRegularExpression("[^\\\\/:*?\"<>|]*"));
	edit->setValidator(validator);
	connect(editDisplayDelayTimer, &QTimer::timeout, this, [this]() {
		showLineEdit();
	});
	//connect(edit, &QLineEdit::editingFinished, this, [this]() {
	//	edit->hide();
	//	if (edit->text() != text())
	//	{
	//		QString textSet = edit->text();
	//		QString Qtext = text();
	//		if (Qtext.right(4) == ".lnk" || Qtext.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
	//			textSet += Qtext.right(4);
	//		setText(textSet);
	//		adjustSize();
	//		update();
	//	}
	//});
	edit->installEventFilter(this);
}
MyFileListItem::MyFileListItem(QWidget* parent, QSize defaultSize)
	: QPushButton(parent), itemRect(QPoint(0, 0), defaultSize)
{
	initialize(parent, defaultSize);//初始化
}

void MyFileListItem::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	QPen pen;
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);//抗锯齿
	qreal xr, yr;//圆角
	xr = yr = (size().width() > size().height() ? size().height() / 8 : size().width() / 8);
	QRect rect;
	rect = this->rect();
	QRect textRect;
	//qrect1.setWidth(qrect1.width() - 1);
	//qrect1.setHeight(qrect1.height() - 1);
	p.setPen(Qt::NoPen);
	p.fillRect(rect, QColor(0, 0, 0, 0));//全透明背景
	this->itemRect = rect;
	if (bgBrush != bgBrush_MouseMove)
	{
		if (isChecked())
			bgBrush = bgBrush_Selected;
		else if (!isChecked())
			bgBrush = bgBrush_Default;
	}
	
	p.setBrush(bgBrush);
	//p.setBrush(QBrush(QColor(100, 100, 119, 100)));
	//p.drawRoundedRect(rect, xr, yr, Qt::SizeMode::AbsoluteSize);
	QString Qtext = text();//文本
	if (Qtext.right(4) == ".lnk" || Qtext.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
		Qtext = Qtext.left(Qtext.size() - 4);
	QTextOption qto;//文本绘制选项
	switch (viewMode)
	{
	default:
	case ViewMode::Icon:
	{
		/*文字处理*/
		Qtext = elidedMultiLinesText(this, Qtext, 2, Qt::ElideRight);//两行分割
		QFontMetrics fontMetrics1(font());//字体
		int textHeight = fontMetrics1.size(Qt::TextExpandTabs, Qtext).height();//两行(如有，否则为一行)的总高度
		/*图标处理并绘制*/
		QRect imageRect;//图标具体位置
		double zoom = iconZoom;//图标的缩放比例
		QSize itemImageZoneSize = size();//图标绘制专属区域
		//QFontMetrics fm(font());
		//int ascent = fm.ascent();    // 上缘高度
		//int descent = fm.descent();  // 下缘深度
		//int height1 = fm.height();    // ascent + descent + 1（某些系统可能+1）
		//int leading = fm.leading();  // 行间额外空白
		//int lineSpacing = fm.lineSpacing(); // height + leading
		itemImageZoneSize.setHeight(itemImageZoneSize.height() - fontMetrics1.lineSpacing() * 2 - fontMetrics1.descent());
		if (itemImageZoneSize.width() <= itemImageZoneSize.height())
		//宽小，用宽度计算图标大小/缩放
			imageRect.setSize(QSize(itemImageZoneSize.width() * zoom, itemImageZoneSize.width() * zoom));
		else
		//高大，用高度计算
			imageRect.setSize(QSize(itemImageZoneSize.height() * zoom, itemImageZoneSize.height() * zoom));
		imageRect.moveTo(QPoint((itemImageZoneSize.width() - imageRect.width()) / 2, (itemImageZoneSize.height() - imageRect.height()) / 2));


		//文本处理
		qto.setAlignment(Qt::AlignHCenter | Qt::AlignTop);//文本居中，居上
		textRect.moveTo(0, itemImageZoneSize.height());
		textRect.setSize(QSize(width(), textHeight));
		int bgHeight = itemImageZoneSize.height() + textHeight;
		if (bgHeight > height())
			bgHeight = height();
		//绘制有效区域背景
		if (backgroundRect != QRect(0, 0, width(), bgHeight))
		{
			backgroundRect = QRect(0, 0, width(), bgHeight);
			setMask(backgroundRect);
		}
		p.drawRoundedRect(backgroundRect, xr, yr, Qt::SizeMode::AbsoluteSize);
		p.save();
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(Qt::DotLine));
		if (hasFocus())
			p.drawRoundedRect(backgroundRect, xr, yr, Qt::SizeMode::AbsoluteSize);//绘制焦点边框
		p.restore();
		//绘制图标
		pen.setColor(QColor(0, 0, 0, 100));//边框颜色
		p.setPen(pen);//边框画笔
		p.drawRect(imageRect);//正方形边框
		p.drawImage(imageRect, itemImage);//绘制图标
		this->imageRect = imageRect;//保存图标位置
	}
		break;
	case ViewMode::List:
	{
		qto.setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
		textRect.setX(itemImage.width());
		textRect.setWidth(size().width() - iconSize().width());
		p.drawRoundedRect(rect, xr, yr, Qt::SizeMode::AbsoluteSize);

	}
		break;
	}
	//绘制文本，设置颜色和画笔
	pen.setColor(QColor(0, 0, 0, 255));
	p.setPen(pen);
	if (!hideText)
		p.drawText(textRect, Qtext, qto);
	//设置QLineEdit的位置和大小
	edit->setGeometry(textRect);
	this->elidedText = Qtext;//保存文本
	this->textRect = textRect;//保存文本位置
	this->textFont = font();
	this->textPen = pen;
	this->textOption = qto;
	QWidget::paintEvent(e);
	//QPushButton::paintEvent(e);
}


bool MyFileListItem::eventFilter(QObject* watched, QEvent* event)
{
	if (!event)//没必要，此处只是为了消除编译器警告
		return false;
	if (watched == this)
	{
		bool mousePosInItem = backgroundRect.contains(mapFromGlobal(QCursor::pos()));
		//if (event->type() == QEvent::MouseMove && mousePosInItem)
		//	bgBrush = bgBrush_MouseMove;
		//else if (event->type() == QEvent::MouseMove && !mousePosInItem)
		//	bgBrush = bgBrush_Default;
		if (event->type() == QEvent::Enter)
			bgBrush = bgBrush_MouseMove;
		else if (event->type() == QEvent::Leave && !isChecked())
			bgBrush = bgBrush_Default;
		else if (event->type() == QEvent::Leave && isChecked())
			bgBrush = bgBrush_Selected;
		else if (event->type() == QEvent::MouseMove)
		{
			if (!mousePosInItem && bgBrush == bgBrush_MouseMove)
			{
				if (isChecked())
					bgBrush = bgBrush_Selected;
				else
					bgBrush = bgBrush_Default;
				update();
			}
			else if (mousePosInItem && bgBrush != bgBrush_MouseMove)
			{
				bgBrush = bgBrush_MouseMove;
				update();
			}
		}
		//if (event->type() == QEvent::MouseButtonPress)
		//{
		//	if (!mousePosInItem || shouldIgnore())
		//	{
		//		event->ignore();
		//		bIgnore = true;
		//	}
		//}
		//else if (event->type() == QEvent::MouseMove)
		//{
		//	if (shouldIgnore())
		//		event->ignore();
		//}
		//else if (event->type() == QEvent::MouseButtonRelease)
		//{
		//	if (bIgnore)
		//		event->ignore();
		//	bIgnore = false;
		//}
		//else if (event->type() == QEvent::MouseButtonDblClick)
		//{
		//	if (!mousePosInItem)
		//		event->ignore();
		//}
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
			if (keyEvent && keyEvent->key() == Qt::Key_F2)
			{
				showLineEdit();
			}
		}
	}
	else if (watched == edit)
	{
		QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
		if ((event->type() == QEvent::FocusOut
			|| (event->type() == QEvent::KeyPress
				&& keyEvent
				&& (keyEvent->key() == Qt::Key_Enter
					|| keyEvent->key() == Qt::Key_Return)
				)
			)
			&& edit->isVisible())
		{
			edit->hide();
			hideText = false;
			QString oldText = text();
			QString newText = edit->text();
			if (oldText.right(4) == ".lnk" || oldText.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
				newText += oldText.right(4);
			if (newText != oldText)
			{
				setText(newText);
				emit renamed(oldText.toStdWString(), newText.toStdWString());
			}
			update();
		}
		else if (event->type() == QEvent::KeyPress
			&& keyEvent
			&& (keyEvent->key() == Qt::Key_Escape)
			)
		{
			hideLineEdit();
		}
	}
	return QPushButton::eventFilter(watched, event);
}


//bool isInRoundedRect(const QPointF& point, const QRectF& rect, qreal radius) {
//	QPainterPath path;
//	path.addRoundedRect(rect, radius, radius); // 创建圆角矩形路径
//	return path.contains(point); // 直接判断点是否在路径内
//}


//矩形碰撞检测
void MyFileListItem::judgeSelection(QRect selectionArea)
{
	if (selectionArea == QRect(0, 0, 0, 0))
		setChecked(false);
	bool checked = true;
	QRect thisRect = geometry();
	thisRect.setSize(backgroundRect.size());
	if (thisRect.right() < selectionArea.left()
		|| thisRect.left() > selectionArea.right()
		|| thisRect.top() > selectionArea.bottom()
		|| thisRect.bottom() < selectionArea.top())
		checked = false;
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
	Qtext = elidedMultiLinesText(this, Qtext, 2, Qt::ElideRight);
	int textHeight = fontMetrics1.size(Qt::TextExpandTabs, Qtext).height();
	switch (viewMode)
	{
	default:
	case ViewMode::Icon:
		resize(itemImage.width() + 2 * fontMetrics1.averageCharWidth(), itemImage.height() + textHeight);
		break;
	case ViewMode::List:
		resize(parentWidget()->width(), 2 * textHeight);
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
	switch(e->button())
	{
	default:
		break;
	case Qt::LeftButton:
	{
		//if (isChecked())
		//{
		//	if (editDisplayDelayTimer->isActive())
		//		editDisplayDelayTimer->stop();
		//	editDisplayDelayTimer->setInterval(1000);
		//	editDisplayDelayTimer->setSingleShot(true);
		//	editDisplayDelayTimer->start();
		//}
		break;
	}
	}
}
void MyFileListItem::mouseMoveEvent(QMouseEvent* e)
{
}
void MyFileListItem::mouseReleaseEvent(QMouseEvent* e)
{
}
void MyFileListItem::dragMoveEvent(QDragMoveEvent* e)
{
}
void MyFileListItem::dragEnterEvent(QDragEnterEvent* e)
{
}

void MyFileListItem::MenuClickedProc(QAction* action)
{
	if (action->text() == "打开")       
		emit doubleClicked();
	else if (action->text() == "删除")
		removeSelf();
}

void MyFileListItem::removeSelf()
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