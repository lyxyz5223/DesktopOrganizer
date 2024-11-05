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
	MyFileListItem(QWidget* parent = nullptr);
	void setViewMode(ViewMode View_Mode);
	void setImage(QImage image) {
		itemImage = image;
	}
	void setMyIconSize(double ICONSIZE) {
		MyIconSize = ICONSIZE;
	}
	void adjustSize();
	void setPath(std::wstring path)
	{
		MyPath = path;
	}
	std::wstring getPath()
	{
		return MyPath;
	}
	void setSelected(bool b) {
		if (b)
		{
			bgBrush = bgBrush_Selected;
			isSelected = true;
		}
		else
		{
			bgBrush = bgBrush_Default;
			isSelected = false;
		}
		update();
	}
protected:
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	//void mouseMoveEvent(QMouseEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

signals:
	void doubleClicked();
	void deleteItem();
	void selected();

public slots:
	void MenuClickedProc(QAction* action);
	void desktopItemProc(std::wstring name,std::wstring desktopPath);
private:
	ViewMode viewMode = ViewMode::List;
	double MyIconSize = 50;
	QImage itemImage;
	QIcon itemIcon = icon();
	QSize itemTextSize;
	std::wstring MyPath;
	//paint
	QBrush bgBrush_Default = QColor(0,0,0,1);//background,alpha=1防止鼠标穿透
	QBrush bgBrush_MouseMove = QColor(255, 255, 255, 75);//background
	QBrush bgBrush_Selected = QColor(255, 255, 255, 125);//background
	QBrush bgBrush = bgBrush_Default;//background
	bool isSelected = false;
};

