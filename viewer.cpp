#include "viewer.h"
#include <QDebug>
#include <QWheelEvent>
Viewer::Viewer(QWidget *parent) : QGraphicsView(parent)
{

}

void Viewer::notifyViewport()
{
	if (!scene())
		return;
	QRectF r = mapToScene(viewport()->geometry()).boundingRect();
	emit viewportChanged(r);
}

void Viewer::resizeEvent(QResizeEvent *event)
{
	Q_UNUSED(event)
	QGraphicsView::resizeEvent(event);
	emit notifyViewport();
}

void Viewer::wheelEvent(QWheelEvent *event)
{
	bool wasHandled=false;
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	if (event->modifiers() & Qt::ControlModifier)
	{
		if (event->delta()>0)
		{
			scale(1.2, 1.2);
		}
		else
		{
			scale(.8, .8);
		}
		wasHandled = true;
	}
	if (event->modifiers() & Qt::ShiftModifier)
	{
		int deltaX = event->delta() ;//Delta().y();//viewport()->geometry().width() / 10;
		QPoint newCenter(viewport()->geometry().center().x() - 1 - deltaX, viewport()->geometry().center().y()-1);

		centerOn(mapToScene(newCenter));
		wasHandled = true;
	}
	if (event->modifiers()==Qt::NoModifier)
	{
		int deltaY = event->delta() ;//Delta().y();//viewport()->geometry().width() / 10;
		QPoint newCenter(viewport()->geometry().center().x() - 1,
		                 viewport()->geometry().center().y() - 1 - deltaY);

		centerOn(mapToScene(newCenter));
		wasHandled = true;

	}

	if(wasHandled)
		event->accept();
	else
		QGraphicsView::wheelEvent(event);
	notifyViewport();
}

void Viewer::mousePressEvent(QMouseEvent * evt)
{
	mLastMousePos = evt->pos();
	if (evt->buttons() == Qt::MiddleButton)
		setCursor(Qt::ClosedHandCursor);
	QGraphicsView::mousePressEvent(evt);
}

void Viewer::mouseReleaseEvent(QMouseEvent *)
{
	setCursor(Qt::ArrowCursor);
}

void Viewer::mouseMoveEvent(QMouseEvent * evt)
{
	QPoint delta = mLastMousePos - evt->pos();
	mLastMousePos = evt->pos();
	if (evt->buttons() == Qt::MiddleButton)
	{
		QPoint newCenter(viewport()->geometry().center().x() - 1 + delta.x(),
		                 viewport()->geometry().center().y() - 1 + delta.y());
		centerOn(mapToScene(newCenter));
		evt->accept();
		notifyViewport();
	}
	QGraphicsView::mouseMoveEvent(evt);
}
