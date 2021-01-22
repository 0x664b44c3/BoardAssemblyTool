#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QDockWidget>
#include <QRectF>

class OverviewViewer;
class QGraphicsScene;
class previewWidget : public QDockWidget
{
	Q_OBJECT
public:
	explicit previewWidget(QWidget *parent = nullptr);

signals:

public slots:
	void onViewChanged(const QRectF &);
	void setScene(QGraphicsScene *);
private slots:
	void onSceneChanged();
protected slots:
	void resizeEvent(QResizeEvent *event);
private:
	OverviewViewer * mView;
	QGraphicsScene * mScene;
};

#endif // PREVIEWWIDGET_H
