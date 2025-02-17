#pragma once
#include <qpushbutton.h>
#include <qdrag.h>
#include "ItemProperty.h"

#ifndef MYFILELISTITEM_H
#define MYFILELISTITEM_H
#include "GrabArea.h"
#include "SelectionArea.h"
#include <QMimeData>



class MyFileListItem : public QPushButton
{
	Q_OBJECT
public:
	enum ViewMode {
		List,
		Icon,
	};
	~MyFileListItem() {}
	MyFileListItem(QWidget* parent, QSize defaultSize);
	MyFileListItem(const MyFileListItem& item, bool isShadow = false/*拖动的影子*/);
	void initialize(QWidget* parent, QSize defaultSize, bool isShadow);
	void setViewMode(ViewMode View_Mode) {
		viewMode = View_Mode;
	}
	void setImage(QImage image) {
		itemImage = image;
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
	void onSelectionAreaResize();
	void setSelectionArea(SelectionArea* selectionArea) {
		if (this->selectionArea)
			disconnect(this->selectionArea, &SelectionArea::resized, this, &MyFileListItem::onSelectionAreaResize);
		this->selectionArea = selectionArea;
		connect(selectionArea, &SelectionArea::resized, this, &MyFileListItem::onSelectionAreaResize);
	}
	void setGrabArea(GrabArea* grabArea) {
		this->grabArea = grabArea;
	}
	void setChecked(bool judge) {
		if (isChecked() != judge)
		{
			QPushButton::setChecked(judge);
			emit checkChange(judge);
		}
	}

protected:
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override {
		//MessageBox(0, L"DoubleClicked!!!", L"", 0);
		emit doubleClicked();
	}
	bool eventFilter(QObject* watched, QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void dragMoveEvent(QDragMoveEvent* e) override;
	void dragEnterEvent(QDragEnterEvent* event) override;

signals:
	void doubleClicked();
	void removeSelfSignal();
	void moveSignal(QPoint);
	void checkChange(bool);

public slots:
	void MenuClickedProc(QAction* action);
	void removeSelfSlot();

private:
	ViewMode viewMode = ViewMode::Icon;
	QImage itemImage;
	QSize itemTextSize;// 文本字体大小，只包含height
	std::wstring MyPath;

	//paint
	QBrush bgBrush_Default = QColor(0,0,0,1);//background,alpha=1防止鼠标穿透
	QBrush bgBrush_Shadow = QColor(0,0,0,0);//background,alpha=0鼠标穿透
	QBrush bgBrush_MouseMove = QColor(255, 255, 255, 75);//background
	QBrush bgBrush_Selected = QColor(255, 255, 255, 125);//background
	QBrush bgBrush = bgBrush_Default;//background

	//
	QPoint startPosOffset;// 鼠标与item左上角坐标的偏移，为正数
	bool isShadowItem = false;
	SelectionArea* selectionArea = nullptr;// 选择区域
	GrabArea* grabArea = nullptr;
};


#endif //MYFILELISTITEM_H