#pragma once
#include <qpushbutton.h>
#include <qdrag.h>
#include "ItemProperty.h"

#ifndef MYFILELISTITEM_H
#define MYFILELISTITEM_H
#include "DragArea.h"
#include "SelectionArea.h"
#include <QMimeData>
#include <qpen.h>
#include <qtextoption.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qplaintextedit.h>

class MyFileListShadowItem
{
private:
	QRect& imageRect;
	QRect& itemRect;
	QImage& itemImage;
	QString& text;
	QRect& textRect;
	QFont& textFont;
	QPen& textPen;
	QTextOption& textOption;

public:
	~MyFileListShadowItem() {}
	MyFileListShadowItem(
		QRect& itemRect,
		QImage& ItemImage,
		QRect& ImageRect,
		QString& Text,
		QRect& TextRect,
		QFont& TextFont,
		QPen& TextPen,
		QTextOption& TextOption)
		: itemRect(itemRect),
		imageRect(ImageRect), itemImage(ItemImage),
		textRect(TextRect), text(Text),
		textFont(TextFont),
		textPen(TextPen),
		textOption(TextOption) {}
	QRect getItemRect() const {
		return itemRect;
	}
	QRect getImageRect() const {
		return imageRect;
	}
	QImage getImage() const {
		return itemImage;
	}
	QString getText() const {
		return text;
	}
	QRect getTextRect() const {
		return textRect;
	}
	QFont getFont() const {
		return textFont;
	}
	QPen getPen() const {
		return textPen;
	}
	QTextOption getTextOption() const {
		return textOption;
	}
};

class MyFileListItem : public QPushButton
{
	Q_OBJECT
public:
	enum ViewMode {
		List,
		Icon,
	};
	~MyFileListItem() {
		editDisplayDelayTimer->stop();
		editDisplayDelayTimer->deleteLater();
		edit->deleteLater();
	}
	MyFileListItem(QWidget* parent, QSize defaultSize);
	void initialize(QWidget* parent, QSize defaultSize);
	void setViewMode(ViewMode View_Mode) {
		viewMode = View_Mode;
	}
	void setImage(QImage image) {
		itemImage = image;
	}
	QImage getImage() const {
		return itemImage;
	}
	void adjustSize();
	void setPath(std::wstring path)
	{
		if (path != L"" && path.back() != L'\\' && path.back() != L'/')
			path += L'\\';
		MyPath = path;
	}
	std::wstring getPath() {
		return MyPath;
	}
	//当框选区域变化时候，父窗口通知此窗口
	void judgeSelection(QRect selectionArea);


	void setChecked(bool judge) {
		isRenaming = false;
		if (isChecked() != judge)
		{
			QPushButton::setChecked(judge);
			emit checkChange(judge);
		}
	}
	//void deleteLater() {
	//	QPushButton::deleteLater();
	//}
	auto getIconZoom() const {
		return iconZoom;
	}
	void removeSelf();

	QRect getEffectiveGeometry() const {
		return QRect(geometry().topLeft(), backgroundRect.size());
	}

	bool shouldIgnore() const {
		return bIgnore;
	}
	void setShouldIgnore(bool ignore) {
		bIgnore = ignore;
	}

	MyFileListShadowItem* getShadowItem() const {
		return shadowItem;
	}

	void showLineEdit() {
		QString Qtext = text();
		if (Qtext.right(4) == ".lnk" || Qtext.right(4) == ".url") // 这两种后缀名的文件直接省略后缀
			Qtext = Qtext.left(Qtext.size() - 4);
		edit->setAlignment(Qt::AlignCenter);
		edit->setText(Qtext);
		//文本居中
		//edit->document()->setDefaultTextOption(QTextOption(Qt::AlignCenter));
		//edit->setPlainText(Qtext);
		hideText = true;
		update();
		edit->show();
		edit->setFocus();
		isRenaming = true;
	}
	void delayShowLineEdit() {
		if (editDisplayDelayTimer->isActive())
			editDisplayDelayTimer->stop();
		editDisplayDelayTimer->setInterval(timerInterval);
		editDisplayDelayTimer->setSingleShot(true);
		editDisplayDelayTimer->start();
	}
	void setShouldShowLineEdit(bool shouldShow) {
		_shouldShowLineEdit = shouldShow;
	}
	bool shouldShowLineEdit() const {
		return _shouldShowLineEdit;
	}

	QTimer* getLineEditDisplayDelayTimer() const {
		return editDisplayDelayTimer;
	}

	int getTimerInterval() const {
		return timerInterval;
	}

	void hideLineEdit() {
		edit->hide();
		hideText = false;
		update();
	}

	bool isRenamingWhenPressAndClearRenameState()  {
		bool res = isRenaming;
		isRenaming = false;
		return res;
	}
protected:
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override {
		//MessageBox(0, L"DoubleClicked!!!", L"", 0);
		//由父亲处理
		//emit doubleClicked();
	}
	bool eventFilter(QObject* watched, QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void dragMoveEvent(QDragMoveEvent* e) override;
	void dragEnterEvent(QDragEnterEvent* event) override;

signals:
	void doubleClicked();
	void checkChange(bool);
	void resizeSignal(QSize size);
	void moveSignal(QPoint pos);
	void adjustSizeSignal();
	void renamed(std::wstring oldName, std::wstring newName);

public slots:
	void MenuClickedProc(QAction* action);

private:
	MyFileListShadowItem* shadowItem
		= new MyFileListShadowItem(
			itemRect,
			itemImage, imageRect,
			elidedText, textRect, textFont, textPen, textOption);
	ViewMode viewMode = ViewMode::Icon;
	QImage itemImage;
	std::wstring MyPath;

	//paint
	QBrush bgBrush_Default = QColor(0,0,0,1);//background,alpha=1防止鼠标穿透
	QBrush bgBrush_Shadow = QColor(0,0,0,0);//background,alpha=0鼠标穿透
	QBrush bgBrush_MouseMove = QColor(255, 255, 255, 75);//background
	QBrush bgBrush_Selected = QColor(255, 255, 255, 125);//background

	QBrush bgBrush = bgBrush_Default;//background
	QRect itemRect;//itemRect
	QRect backgroundRect;//item实际绘制的部分
	QRect imageRect;//item图标
	QString elidedText;//item文字
	QRect textRect;//item文字Rect
	QFont textFont;//item文字字体
	QPen textPen;//item文字画笔
	QTextOption textOption;//item文字选项

	double iconZoom = 5.0 / 7;//图标的缩放比例
	bool bIgnore = false;//是否忽略鼠标事件
	bool hideText = false;//是否隐藏文字

	//QPlainTextEdit* edit = new QPlainTextEdit(this);
	QLineEdit* edit = new QLineEdit(this);
	QTimer* editDisplayDelayTimer = new QTimer();
	bool _shouldShowLineEdit = false;
	int timerInterval = 1000;
	bool isRenaming = false;
};



#endif //MYFILELISTITEM_H