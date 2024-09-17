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
	void paintEvent(QPaintEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e);
	void setViewMode(ViewMode View_Mode);
	void setImage(QImage image) {
		itemImage = image;
	}
	void setMyIconSize(double ICONSIZE) {
		MyIconSize = ICONSIZE;
	}

signals:
	void doubleClicked();

public slots:
private:
	ViewMode viewMode = ViewMode::List;
	double MyIconSize = 50;
	QImage itemImage;
	QIcon itemIcon = icon();
	QSize itemTextSize;
};

