#ifndef SELECTIONAREA_H
#define SELECTIONAREA_H
#include <qwidget.h>
class SelectionArea : public QWidget{
	Q_OBJECT
private:
	const QColor defaultBgColor = QColor(10, 123, 212, 100);
	const QColor defaultBorderColor = QColor(10, 123, 212, 255);
	const double defaultBorderWidth = 1.0;

	QPoint mouseDownPos;
	QRect _geometry;//frame center
	const QColor backgroundColor = defaultBgColor;
	const QColor borderColor = defaultBorderColor;
	const double borderWidth = defaultBorderWidth;
public:
	~SelectionArea() {}
	SelectionArea(QWidget* parent = nullptr);
	QRect getGeometry() const {
		return _geometry;
	}
	void setGeometry(QRect geometry) {
		this->_geometry = geometry;
		QWidget::setGeometry(geometry);
		repaint();
	}
	void setGeometry(int x, int y, int width, int height) {
		this->_geometry = QRect(x, y, width, height);
		QWidget::setGeometry(x, y, width, height);
		repaint();
	}
	QColor getBackgroundColor() const {
		return backgroundColor;
	}
	QColor getBorderColor() const {
		return borderColor;
	}
	double getBorderWidth() const {
		return borderWidth;
	}

	QPoint getMouseDownPos() const {
		return mouseDownPos;
	}
	void setMouseDownPos(QPoint pos) {
		this->mouseDownPos = pos;
	}

	void reset() {
		this->setGeometry(0, 0, 0, 0);
	}
	SelectionArea& operator=(const QRect& rect) {
		this->setGeometry(rect);
		return *this;
	}
	SelectionArea& operator=(const SelectionArea& area) {
		this->setGeometry(area._geometry);
		return *this;
	}
signals:
	void resized(QRect newRect);

protected:
	void paintEvent(QPaintEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
};


#endif