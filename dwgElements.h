#ifndef DWGELEMENTS_H
#define DWGELEMENTS_H

#include <QVector>
#include <QPoint>
#include <QDebug>
namespace Drawing {
enum LineElementType {
	etInvalid=-1,
	etStart=0,
	etLine=1,
	etArc=2
};
struct lineSegment {
	QPointF p;
	LineElementType type;
	int curve;
	int width; /**< width -1 means no change */
	lineSegment() {
		type=etInvalid;
		width=0;
		curve=0;
	}
};

struct PolyLine {
	QVector<lineSegment> segments;
	int maxWidth;
	int layer; //TODO: move this to the container
	PolyLine() {
		maxWidth = 0;
	}
	PolyLine(QPointF p0) {
		lineSegment n;
		n.type = etStart;
		n.p = p0;
		segments.append(n);
		maxWidth = 0;
	}
	/** a line mus have at least two elements, a start and an end */
	bool isNull() const {
		if (segments.size()<2)
			return true;
		return false;
	}
	void addLineTo(QPointF p, int width=-1) {
		lineSegment n;
		n.type = etLine;
		n.p = p;
		if (width==-1) {
			n.width = segments.last().width;
		} else {
			n.width = width;
		}
		maxWidth = qMax(width, maxWidth);
		segments.append(n);
		//qDebug()<<"added node"<<elements.size();
	}
	void addArcTo(QPointF p, int curve, int width=-1) {
		lineSegment n;
		n.type = (curve!=0)?etArc:etLine;
		n.p = p;
		if (width==-1) {
			n.width = segments.last().width;
		} else {
			n.width = width;
		}
		maxWidth = qMax(width, maxWidth);
		n.curve = curve;
		segments.append(n);
		//qDebug()<<"added node"<<elements.size();
	}
	//polyLine(QPoint p1, QPoint p2, int width, int curve)
}; //struct

struct Wire {
	QPointF p1;
	QPointF p2;
	double width;
	double curve;
	int layer;
	// also has style and cap
	bool follows(const Wire & prev) {
		//qDebug()<<"Line joins?"<<prev.p2<<p1;
		if ((prev.p2 == this->p1)
		    &&(prev.layer == this->layer)
		     )
			return true;
		return false;
	}
	bool precedes(const Wire & next) {
		if ((next.p1 == this->p2)
		    &&(next.layer == this->layer)
		     )
			return true;
		return false;
	}
};

struct Rectangle{
	QPointF p1;
	QPointF p2;
	int layer;
	double rotation;
};

struct Polygon
{
	struct vertex
	{
	    public:
		QPointF p;
		double curve;
		vertex(double x, double y, double curve) {
			this->p.setX(x);
			this->p.setY(y);
			this->curve = curve;
		}
	};
	double width;
	int layer;
	int rank;
	QString pour;
	QList<vertex> vertices;
};


struct Circle
{
	QPointF center;
	double radius;
	double width;
	int layer;
};

struct Smd
{
	QString name;
	QPointF center;
	QSizeF size;
	int layer;
	double rotation;
	int roundness;
	bool cream;
	bool stop;
};

enum PadShape
{
	padRound=0,
	padOctagon=1,
	padSquare=2
};
struct Via
{
	QPointF p;
	QString extent;
	PadShape shape;
	double drill;
	double diameter;
};


struct Element
{
public:
	Element()
	{
		type = tNull;
	}
	enum Types
	{
		tNull=0,
		tWire,
		tRect,
		tCircle,
		tPolygon,
		tSmd,
		tVia
	} type;
	struct
	{
		Wire      wire;
		Rectangle rect;
		Circle    circle;
		Polygon   poly;
		Smd       smd;
		Via       via;
	} e;
};

} // namespace

#endif // DWGELEMENTS_H
