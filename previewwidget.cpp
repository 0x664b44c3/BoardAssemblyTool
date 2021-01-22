#include "previewwidget.h"
#include <QGraphicsView>
#include <overviewviewer.h>


OverviewViewer::OverviewViewer(QWidget *parent)
{

}

void OverviewViewer::setScene(QGraphicsScene *scene)
{
	QGraphicsView::setScene(scene);
	mHilightRect = scene->sceneRect();
}

void OverviewViewer::setHilightRect(const QRectF &r)
{
	mHilightRect = r;
}

void OverviewViewer::drawForeground(QPainter *painter, const QRectF &rect)
{
	Q_UNUSED(rect)
	painter->setBrush(QColor::fromRgb(0,0,0,128));
//	double penWidth = mapToScene(1,0).x();
	QPainterPath path;
	painter->setPen(Qt::NoPen);
	path.addRect(mapToScene(viewport()->geometry()).boundingRect());
	path.addRect(mHilightRect);
	painter->drawPath(path);
}


previewWidget::previewWidget(QWidget *parent) : QDockWidget(parent),mScene(0)
{
	mView = new OverviewViewer(this);
	setWindowTitle(tr("Overview"));
	setWidget(mView);
}

void previewWidget::onViewChanged(const QRectF & r)
{
	mView->setHilightRect(r);
	mView->updateScene(QList<QRectF>() << mScene->sceneRect());
}

void previewWidget::setScene(QGraphicsScene * scene)
{
	mScene = scene;
	mView->setScene(scene);
	QTransform xfm;
	xfm.scale(1,-1);
	mView->setTransform(xfm);
	mView->fitInView(mScene->sceneRect(), Qt::KeepAspectRatio);
}

void previewWidget::onSceneChanged()
{

}

void previewWidget::resizeEvent(QResizeEvent *event)
{
	Q_UNUSED(event)
	if (mScene)
		mView->fitInView(mScene->sceneRect(), Qt::KeepAspectRatio);
}
