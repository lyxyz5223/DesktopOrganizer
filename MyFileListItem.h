#pragma once
#include <qpushbutton.h>

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
	MyFileListItem(MyFileListItem& item, bool isShadow = false/*拖动的影子*/);
	void setViewMode(ViewMode View_Mode);
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

protected:
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override;
	bool eventFilter(QObject* watched, QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* e) override;

signals:
	void doubleClicked();
	void removeSelfSignal();
	void selected();
	void moveSignal(QPoint);

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
	MyFileListItem* shadowItem = nullptr;
};

