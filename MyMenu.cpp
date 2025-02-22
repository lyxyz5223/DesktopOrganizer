#include "MyMenu.h"

#include <qevent.h>
#include <qpainter.h>
#include <qproxystyle.h>
#include <qstyleoption.h>
#include "MyMenuAction.h"
#include <QApplication>
#include <QAnimationGroup>

//svg
#include <QFile>
#include <QtSvg/qsvgrenderer.h>
#include <QGraphicsDropShadowEffect>
#include <QParallelAnimationGroup>

class MyMenuStyle : public QProxyStyle
{
	

public:
	MyMenuStyle(QStyle* style = 0) {}
	void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const;
	QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const;
	void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;

private:
	int menuItemHeight = 30;
	int iconWidth = 15;
	int borderWidth = 7;
	double tipTextHeightZoom = 0.7;// 灰色提示文字相对于正常menuItemHeight的高度倍数
};

void MyMenuStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	if (element == PE_PanelMenu)
	{
		// 绘制菜单面板时添加阴影
		//painter->save();
		//painter->setRenderHint(QPainter::Antialiasing, true);
		//painter->setPen(Qt::transparent);
		//painter->setBrush(QColor(0, 0, 0, 150));  // 阴影颜色和透明度
		//painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 5, 5);  // 绘制阴影
		//painter->restore();
	}
	QProxyStyle::drawPrimitive(element, option, painter, widget);  // 调用基类绘制
}

void MyMenuStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	switch (element)
	{
	case QStyle::CE_MenuItem:
	{
		const MyMenu* menu = dynamic_cast<const MyMenu*>(widget);
		//内容绘制 区分类型
		if (const QStyleOptionMenuItem* mopt = qstyleoption_cast<const QStyleOptionMenuItem*>(option))
		{
			painter->save();
			painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
			// 分隔线
			if (mopt->menuItemType == QStyleOptionMenuItem::Separator)
			{
				const QRect separatorAreaRect = mopt->rect;
				double separatorHeight = 1;
				int paddingLeft = 10;
				int paddingRight = 10;
				int separatorWidth = separatorAreaRect.width() - paddingLeft - paddingRight;

				QRectF separatorRect(// AlignCenter: horzontal and vertical
					separatorAreaRect.x() + (separatorAreaRect.width() - separatorWidth) / 2,// left
					separatorAreaRect.y() + (separatorAreaRect.height() - separatorHeight) / 2,// top
					separatorWidth,// width
					separatorHeight// height
				);
				QBrush brush = QBrush(QColor(150, 150, 150, 100));
				painter->setPen(Qt::NoPen);
				painter->fillRect(separatorRect, brush);
			}
			else
			{
				QRect menuItemRect = mopt->rect;

				//hover
				if (mopt->state.testFlag(QStyle::State_Enabled) && (mopt->state.testFlag(QStyle::State_MouseOver) || mopt->state.testFlag(QStyle::State_Selected)))
				{
					painter->setPen(Qt::NoPen);
					painter->setBrush(QColor(232, 232, 232));
					painter->drawRoundedRect(menuItemRect, 5, 5);
				}

				//Icon绘制
				//check绘制
				if (mopt->menuHasCheckableItems)
				{
					QRect iconRect = QRect(
						menuItemRect.x() + (menuItemHeight - iconWidth) / 2,
						menuItemRect.y() + (menuItemHeight - iconWidth) / 2,
						iconWidth,
						iconWidth
					);
					if (!mopt->checked)
					{
						QIcon menuIcon = mopt->icon;
						painter->drawPixmap(
							iconRect, menuIcon.pixmap(iconRect.size())
						);
					}
					else
					{
						QSvgRenderer svgCheckedRender;
						QFile svgCheckedFile;
						svgCheckedFile.setFileName(":/DesktopOrganizer/img/iconoir--check.svg");
						svgCheckedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
						svgCheckedRender.load(svgCheckedFile.readAll());
						svgCheckedRender.render(painter, iconRect);
					}
				}
				else
				{
					QIcon menuIcon = mopt->icon;
					QRect iconRect = QRect(
						menuItemRect.x() + (menuItemHeight - iconWidth) / 2,
						menuItemRect.y() + (menuItemHeight - iconWidth) / 2,
						iconWidth,
						iconWidth
					);
					if (!menuIcon.isNull())
						painter->drawPixmap(iconRect, menuIcon.pixmap(iconRect.size()));
				}
				//文字和快捷键绘制
				if (!mopt->text.isEmpty())
				{
					// 文本处理
					QStringList textAndShortCutList = mopt->text.split("\t");
					QString text = textAndShortCutList[0];
					QStringList textList = text.split("\n");
					QString mainText = textList[0];
					QString tipText	= "";
					QString shortCutText = "";
					if (textList.count() > 1)
					{
						textList.removeFirst();
						tipText = textList.join("\n");
					}
					if (textAndShortCutList.count() > 1)
					{
						textAndShortCutList.removeFirst();
						shortCutText = textAndShortCutList.join("\t");
					}
					// 几何图形位置大小处理
					QRect mainTextRect = menuItemRect;
					mainTextRect.setLeft(mainTextRect.x() + menuItemHeight);
					mainTextRect.setHeight(menuItemHeight);
					QRect tipTextRect = mainTextRect;
					tipTextRect.setBottom(menuItemRect.bottom());
					tipTextRect.setTop(mainTextRect.y() + mainTextRect.height());
					tipTextRect.setRight(tipTextRect.right() - iconWidth);
					//画笔设置并绘制
					painter->setPen(mopt->state.testFlag(QStyle::State_Enabled) ? Qt::black : Qt::gray);
					painter->drawText(mainTextRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, mainText);
					painter->setPen(Qt::gray);
					//绘制快捷键
					mainTextRect.setRight(mainTextRect.right() - iconWidth);
					mainTextRect.setLeft(mainTextRect.right() - mopt->reservedShortcutWidth);
					//painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, text);
					painter->drawText(mainTextRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, shortCutText);
					
					//绘制灰色提示文本
					painter->drawText(tipTextRect, Qt::AlignLeft | Qt::AlignTop/*Qt::AlignVCenter*/, tipText);
					//painter->drawRect(tipTextRect);
				}
				//>图标
				if (mopt->menuItemType == QStyleOptionMenuItem::SubMenu)
				{
					//MyMenuAction* action = menu->actionAt(menuItemRect.center());
					//if (!action) return;
					//QFontMetrics fm(action->font());
					QRect unfoldIconRect = QRect(
						menuItemRect.right() - iconWidth,
						menuItemRect.y() + (menuItemRect.height() - iconWidth) / 2,
						iconWidth,
						iconWidth
					);
					painter->setPen(mopt->state.testFlag(QStyle::State_Enabled) ? Qt::black : Qt::gray);
					//painter->drawText(QRect(menuItemRect.right() - fm.size(0, ">").width() * 2, menuItemRect.y(), fm.size(0, ">").width() * 2, menuItemRect.height()), Qt::AlignCenter, ">");
					QSvgRenderer svgCheckedRender;
					QFile svgCheckedFile;
					svgCheckedFile.setFileName(":/DesktopOrganizer/img/iconoir--nav-arrow-right.svg");
					svgCheckedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
					svgCheckedRender.load(svgCheckedFile.readAll());
					svgCheckedRender.render(painter, unfoldIconRect);
					
				}
			}
			painter->restore();
		}

		return;
	}
	case QStyle::CE_MenuEmptyArea:
		break;
	default:
		break;
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

int MyMenuStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	switch (metric)
	{
	case QStyle::PM_SmallIconSize:
	{
		//图标宽度
		return iconWidth;
	}
	case QStyle::PM_MenuPanelWidth:
	{
		//外围容器宽度
		return borderWidth;
	}
	default:
		break;
	}
	return QProxyStyle::pixelMetric(metric, option, widget);
}

QSize MyMenuStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const
{
	switch (type)
	{
	case QStyle::CT_MenuItem:
	{
		if (const QStyleOptionMenuItem* mopt = qstyleoption_cast<const QStyleOptionMenuItem*>(option))
		{
			if (mopt->menuItemType == QStyleOptionMenuItem::Separator)
				break;
			const MyMenu* menu = dynamic_cast<const MyMenu*>(widget);
			//QSize menuItemSize = QProxyStyle::sizeFromContents(type, option, size, widget);
			QString itemText = mopt->text;
			QStringList itemTextList = itemText.split("\t")[0].split("\n");
			int maxWidth = 0;
			int itemWidth = 0;
			for (qsizetype i = 0; i < itemTextList.size(); i++)
			{
				QStyleOptionMenuItem soptmi(*mopt);
				soptmi.text = itemTextList[i];
				int tmp = soptmi.fontMetrics.size(0, soptmi.text).width();
				if (tmp > maxWidth)
				{
					maxWidth = tmp;
					itemWidth = QProxyStyle::sizeFromContents(type, &soptmi, QSize(maxWidth, size.height()), widget).width();
				}
			}
			qsizetype lineNum = itemText.count("\n");
			int actionHeight = menuItemHeight * (1 + lineNum * tipTextHeightZoom);
			if (menu->hasChildMenu())//有子菜单
			{
				if (!menu->hasIcon())//没图标
					return QSize(itemWidth + actionHeight, actionHeight);
				else//有图标
					return QSize(itemWidth, actionHeight);

			}
			else
				return QSize(itemWidth, actionHeight);
		}
		break;
	}
	default:
		break;
	}
	return QProxyStyle::sizeFromContents(type, option, size, widget);
}

MyMenu::MyMenu(QWidget* parent) : QMenu(parent)
{
	MyMenuStyle* s = new MyMenuStyle();
	setStyle(s);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlag(Qt::Popup);
	setWindowFlag(Qt::FramelessWindowHint);//无边框
	setWindowFlag(Qt::NoDropShadowWindowHint);//取消windows自带阴影
	//QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
	//shadow->setOffset(0, 0);
	//shadow->setColor(QColor(/*"#444444"*/0,0,0,0));
	//shadow->setBlurRadius(10);
	//this->setGraphicsEffect(shadow);
	installEventFilter(this);
}

bool MyMenu::eventFilter(QObject* watched, QEvent* e)
{
	if (e->type() == QEvent::Close)
	{
		if (!hasPlayCloseAni)
		{
			e->ignore();
			startCloseAnimation();
			return true;
		}
		else
			hasPlayCloseAni = false;
	}
	return QMenu::eventFilter(watched, e);
}

void MyMenu::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	const QList<QAction*>& actions = this->actions();
	for (int i = 0; i < actions.size(); ++i)
	{
		QAction* pAction = actions.at(i);
		QRect rect = actionGeometry(pAction);
		//p.fillRect(rect, QColor(115, 110, 110));
	}
	QMenu::paintEvent(e);
}

void MyMenu::mouseMoveEvent(QMouseEvent* e)
{
	QMenu::mouseMoveEvent(e);

	//const QList<QAction*>& actions = this->actions();
	//for (int i = 0; i < actions.size(); ++i)
	//{
	//	QAction* pAction = actions.at(i);
	//	QRect rect = actionGeometry(pAction);

	//	if (rect.contains(e->pos()))
	//	{
	//		break;
	//	}
	//}
}

void MyMenu::mouseReleaseEvent(QMouseEvent* e)
{
	auto action = actionAt(e->pos());
	if (action && action->isSeparator())
		return;
	QMenu::mouseReleaseEvent(e);
}

bool MyMenu::event(QEvent* e)
{
	if (e->type() == QEvent::WinIdChange)
	{

	}
	else if (e->type() == QEvent::Close)
	{
		
	}
	return QMenu::event(e);
}

void MyMenu::showEvent(QShowEvent* e)
{
	startShowAnimation();
	QMenu::showEvent(e);
}

void MyMenu::closeEvent(QCloseEvent* e)
{
	QMenu::closeEvent(e);
}

void MyMenu::startCloseAnimation()
{
	QParallelAnimationGroup* aniGroup = new QParallelAnimationGroup(this);
	QPropertyAnimation* ani;
	ani = new QPropertyAnimation(this, "geometry", parent());
	ani->setEasingCurve(QEasingCurve::InQuart);
	QRect endGeometry = geometry();
	endGeometry.setHeight(0);
	ani->setStartValue(geometry());
	ani->setEndValue(endGeometry);
	ani->setDuration(200);
	ani->setDirection(QAbstractAnimation::Forward);
	//connect(ani, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
	//	update();
	//	});
	aniGroup->addAnimation(ani);
	//ani->start(QAbstractAnimation::DeleteWhenStopped);
	QPropertyAnimation* aniOpacity = new QPropertyAnimation(this, "windowOpacity", parent());
	aniOpacity->setStartValue(1);
	aniOpacity->setEndValue(0);
	aniOpacity->setDuration(200);
	aniOpacity->setDirection(QAbstractAnimation::Forward);
	aniOpacity->setEasingCurve(QEasingCurve::InQuart);
	aniGroup->addAnimation(aniOpacity);
	connect(aniGroup, &QParallelAnimationGroup::finished, this, [=]() {
		hasPlayCloseAni = true;
		close();
		});
	aniGroup->start(QAbstractAnimation::DeleteWhenStopped);

}


void MyMenu::startShowAnimation()
{
	QParallelAnimationGroup* aniGroup = new QParallelAnimationGroup(this);
	QPropertyAnimation* aniGeometry;
	aniGeometry = new QPropertyAnimation(this, "geometry", parent());
	aniGeometry->setEasingCurve(QEasingCurve::OutQuart);
	QRect startGeometry = geometry();
	startGeometry.setHeight(0);
	aniGeometry->setStartValue(startGeometry);
	aniGeometry->setEndValue(geometry());
	aniGeometry->setDuration(200);
	aniGeometry->setDirection(QAbstractAnimation::Forward);
	//ani->start(QAbstractAnimation::DeleteWhenStopped);
	aniGroup->addAnimation(aniGeometry);
	QPropertyAnimation* aniOpacity = new QPropertyAnimation(this, "windowOpacity", parent());
	aniOpacity->setStartValue(0);
	aniOpacity->setEndValue(1);
	aniOpacity->setDuration(200);
	aniOpacity->setDirection(QAbstractAnimation::Forward);
	aniOpacity->setEasingCurve(QEasingCurve::OutQuart);
	aniGroup->addAnimation(aniOpacity);
	//connect(aniGroup, &QParallelAnimationGroup::finished, this, [=]() {
	//	show();
	//	});
	aniGroup->start(QAbstractAnimation::DeleteWhenStopped);
}
