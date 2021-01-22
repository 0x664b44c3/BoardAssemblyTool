#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include <QGraphicsView>
class Viewer : public QGraphicsView
{
	Q_OBJECT
public:
	explicit Viewer(QWidget *parent = nullptr);
private:
	QPoint mLastMousePos;
	void notifyViewport();
signals:
	void viewportChanged(QRectF sceneRect);
public slots:
private slots:
	void resizeEvent(QResizeEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);


};

#endif // VIEWER_H
