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

protected:
	void mousePressEvent(QMouseEvent* e);
	void paintEvent(QPaintEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e);

signals:
	void doubleClicked();
	void deleteItem();

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
};

