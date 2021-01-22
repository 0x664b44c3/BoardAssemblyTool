#ifndef OVERVIEWVIEWER_H
#define OVERVIEWVIEWER_H
#include <QGraphicsView>
class OverviewViewer : public QGraphicsView
{
	Q_OBJECT
public:
	explicit OverviewViewer(QWidget *parent = nullptr);
	void setScene(QGraphicsScene *scene);
	void setHilightRect(const QRectF & r);
private:
	QRectF mHilightRect;
protected slots:
	void drawForeground(QPainter *painter, const QRectF &rect);

};

#endif // OVERVIEWVIEWER_H
