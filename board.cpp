#include "board.h"
#include <QDomDocument>
#include <QDomNode>
#include <QDebug>
#include "dwgElements.h"
#include <math.h>

#include <QGraphicsItemGroup>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QRegExp>
#include <algorithm>

double coordToFloat(int c) {
	return ((double)c)/1000000.0d;
}

//Pythagoras
double distance(QPointF a, QPointF b) {
	return sqrt(pow(a.x()-b.x(),2) + pow(a.y()-b.y(),2) );
}

QColor colorFromEagleNumber(int num) {
	QColor c;
	int f=(num&8)?255:170;
	c.setRed((num&4)?f:0);
	c.setGreen((num&2)?f:0);
	c.setBlue((num&1)?f:0);
	return c;
}


double deg2rad(double deg) {
	return (M_PI * deg / 180.0);
}
double rad2deg(double rad) {
	return (rad / M_PI) * 180.0;
}


bool layerActive(int l)
{
	//tplace
	if (l==21)
		return true;
	//tdoc
	if (l==51)
		return true;
	//vias
	if (l==18)
		return true;
	//dimension
	if (l==20)
		return true;

	return false;
}
QColor layerColor(int l)
{
	if (l==18)
		return QColor::fromRgb(0,0,0,64);
	return QColor::fromRgb(168,168,168,224);
}


/** a start b end */
QPointF arcCenter(QPointF a, QPointF b, float curve) {
	double r = distance(a,b) / 2.0 * 1.0 / sin(deg2rad(curve/2));
	//qDebug()<<"Radius"<<r;
	QPointF s = b-a;
	QPointF rv(-s.y(), s.x());
	double l = sqrt(pow(rv.x(), 2) + pow(rv.y(),2));
	rv/=l;
	QPointF center = a + 0.5 * s + r*rv;
	//qDebug()<<"Center would be at "<<center;
	return center;
}

bool evalBoolean(QString param, bool defaultValue) {
	QString p = param.toLower().trimmed();
	if (p == "yes")
		return true;
	if (p == "true")
		return true;
	if (p == "on")
		return true;
	if (p == "no")
		return false;
	if (p == "false")
		return false;
	if (p == "off")
		return false;
	return defaultValue;
}

QList<Board::bomEntry> Board::generateBom() const
{
	QMap<QString, bomEntry> bom;
	foreach (LibRef ref, mData.components) {
		bomEntry entry;
		entry.designators = QStringList() << ref.name;
		entry.library = ref.lib;
		entry.package = ref.package;
		entry.value   = ref.value;
		entry.partId  = ref.lib + "::" + ref.package + " " + ref.value;
		if (bom.contains(entry.partId))
		{
			bom[entry.partId].designators << ref.name;
		}
		else
		{
			bom.insert(entry.partId, entry);
		}
	}
	//qDebug()<<bom.values()-.;
	return bom.values();
}

Board::Board(QObject *parent) : QObject(parent),mHilightColor(Qt::blue), mScene(0) {
}

bool Board::parseFile(QIODevice & f) {
	clear();
	emit boardChanged();
	if (mScene)
	{
		mScene->clear();
	}
	if (!f.isOpen()) {
		postError(tr("File not open."));
		return false;
	}
	QDomDocument dom;
	QString errorStr;
	int errorLine, errorColumn;
	if (!dom.setContent(&f, false, &errorStr, &errorLine,
	                    &errorColumn)) {
		postError(tr("Parser error in line %2:%3 (%1)")
		          .arg(errorStr)
		          .arg(errorLine)
		          .arg(errorColumn));
		return false;
	}

	QDomElement root = dom.documentElement();
	if (root.tagName() != "eagle") {
		postError("Not an eagle file");
		return false;
	}
	if (root.hasAttribute("version"))
		qDebug()<<"Eagle version"<<root.attribute("version");

	//get the board root node
	QDomElement drawing = root.firstChildElement("drawing");
	if (drawing.isNull()) {
		postError("Invalid or damaged file");
		return false;
	}
	QDomElement board = drawing.firstChildElement("board");
	if (board.isNull()) {
		postError("The file is not an eagle board file");
		return false;
	}

	QDomElement layers = drawing.firstChildElement("layers");
	if(layers.isNull()) {
		postError("The drawing has no layers defined");
		return false;
	} else {
		parseLayers(&layers);
	}
	//get unit of measurement
	QDomElement grid = drawing.firstChildElement("grid");
	if (grid.isNull() || (!grid.hasAttribute("unit"))) {
		postError("No grid defined");
		return false;
	}
	if (grid.attribute("unit") == "inch")
		m_Grid = Eagle::uInch;
	if (grid.attribute("unit") == "mm")
		m_Grid = Eagle::uMillimeters;
	if (grid.attribute("unit") == "mic")
		m_Grid = Eagle::uMicron;
	if (grid.attribute("unit") == "mil")
		m_Grid = Eagle::uMils;

	if (m_Grid == Eagle::uUndefined) {
		postError("Grid has invalid unit: " + grid.attribute("unit") );
		return false;
	}
	//parse line segments now
	parseBoard(&board);
	//
	QList<Drawing::PolyLine> linesToMill;
	foreach(Drawing::PolyLine line, m_Lines) {
		if (line.layer == 46)
			linesToMill.append(line);
	}
	emit boardChanged();
	return true;
}

QString Board::getLastError() const {
	return m_LastError;
}

void Board::parseLayers(QDomElement * e) {
	QDomNode node = e->firstChild();
	while(!node.isNull()) {
		if(node.isElement()) {
			QDomElement layer = node.toElement();
			if (layer.isNull())
				return;
			if (layer.tagName()!="layer")
				return;
			Eagle::Layer l;
			//<layer number="19" name="Unrouted" color="6" fill="1" visible="yes" active="yes"/>
			l.number = layer.attribute("number", "0").toInt();
			l.name = layer.attribute("name");
			l.color = colorFromEagleNumber(layer.attribute("color", "7").toInt());
			l.fillStyle = layer.attribute("fill", "1").toInt();
			l.visible = evalBoolean(layer.attribute("visible"), false);
			l.active = evalBoolean(layer.attribute("active"), false);
			m_Layers.append(l);
		}
		node=node.nextSibling();
	}
}

Drawing::Wire parseLineElement(const QDomElement & e) {
	Drawing::Wire l;
	l.p1.setX(e.attribute("x1","0").toDouble());
	l.p1.setY(e.attribute("y1","0").toDouble());
	l.p2.setX(e.attribute("x2","0").toDouble());
	l.p2.setY(e.attribute("y2","0").toDouble());
	l.width = e.attribute("width","0").toDouble();
	l.curve = e.attribute("curve", "0").toFloat();
	l.layer = e.attribute("layer","0").toInt();
	return l;
}

//<rectangle x1="-0.675" y1="0" x2="-0.525" y2="0.3" layer="21"/>
Drawing::Rectangle parseRectElement(const QDomElement & e) {
	Drawing::Rectangle r;
	r.p1.setX(e.attribute("x1","0").toDouble());
	r.p1.setY(e.attribute("y1","0").toDouble());
	r.p2.setX(e.attribute("x2","0").toDouble());
	r.p2.setY(e.attribute("y2","0").toDouble());

	//FIXME wont work
	//R157
	r.rotation =  e.attribute("rot","0").toDouble();
	r.layer = e.attribute("layer","0").toInt();
	return r;
}

/*
<polygon width="0.254" layer="16" rank="4">
<polygon width="0.254" layer="1" pour="cutout">
<vertex x="97.79" y="10.4775"/>
<vertex x="92.71" y="10.4775"/>
<vertex x="92.71" y="7.3025"/>
<vertex x="97.79" y="7.3025"/>
</polygon>
*/
Drawing::Polygon parsePolygon(const QDomElement & e)
{
	Drawing::Polygon p;
	p.layer = e.attribute("layer","0").toInt();
	p.rank  = e.attribute("rank","0").toInt();
	p.pour  = e.attribute("pour","");
	p.width = e.attribute("width","0").toDouble();
	QDomNode node = e.firstChild();
	while(!node.isNull())
	{
		QDomElement e = node.toElement();
		if (e.tagName() == "vertex")
		{
			double x = e.attribute("x", "0").toDouble();
			double y = e.attribute("y", "0").toDouble();
			double c = e.attribute("curve", "0").toDouble();
			Drawing::Polygon::vertex vertex(x, y, c);
			p.vertices.append(vertex);
		}
		node = node.nextSibling();
	}
	return p;
}

bool parseRot(const QString & arg, bool & mirror, int &rot)
{
	QString tmp = arg.toLower();
	if (tmp.isEmpty())
	{
		rot=0;
		mirror = 0;
		return true;
	}
	if (tmp.startsWith("m"))
	{
		mirror = true;
		tmp = tmp.mid(1);
	}
	if (tmp.isEmpty())
	{
		rot=0;
		return true;
	}
	if (tmp.startsWith("r"))
	{
		bool ok=false;
		rot = tmp.mid(1).toInt(&ok);
		return ok;
	}
	return false;
}

int getRotation(QString arg)
{
	bool mirror;
	int rotation;
	if (parseRot(arg, mirror, rotation))
	{
		return rotation;
	}
	return 0;
}


bool getMirrored(QString arg)
{
	bool mirror;
	int rotation;
	if (parseRot(arg, mirror, rotation))
	{
		return mirror;
	}
	return false;
}

Drawing::Circle parseCircle(const QDomElement & e)
{
	Drawing::Circle c;
	c.layer = e.attribute("layer","0").toInt();
	c.center.setX(e.attribute("x","0").toDouble());
	c.center.setY(e.attribute("y","0").toDouble());
	c.radius = e.attribute("radius","0").toDouble();
	c.width =  e.attribute("width","0").toDouble();
	return c;
}

Drawing::Circle parseHole(const QDomElement & e)
{
	Drawing::Circle c;
	c.layer = 45; // holes
	c.center.setX(e.attribute("x","0").toDouble());
	c.center.setY(e.attribute("y","0").toDouble());
	c.radius = e.attribute("drill","0").toDouble() / 2.0;
	c.width =  0;
	return c;
}


//<smd name="2" x="0.65" y="0" dx="0.7" dy="0.9" layer="1" rot="R90"/>
Drawing::Smd parseSMD(const QDomElement & e)
{
	Drawing::Smd s;
	s.name = e.attribute("name","");
	s.center.setX(e.attribute("x","0").toDouble());
	s.center.setY(e.attribute("y","0").toDouble());
	s.size.setWidth(e.attribute("dx","0").toDouble());
	s.size.setHeight(e.attribute("dy","0").toDouble());
	s.layer = e.attribute("layer","0").toInt();
	s.rotation = getRotation(e.attribute("rot", "R0"));
	return s;
}

//<via x="4.1275" y="62.23" extent="1-16" drill="0.3" diameter="0.8128"/>
Drawing::Via parseVia(const QDomElement & e, double defaultRestring)
{
	Drawing::Via via;
	via.diameter = e.attribute("diameter").toDouble();
	via.drill = e.attribute("drill").toDouble();

	via.diameter = qMax(via.diameter, via.drill + 2.0 * defaultRestring);
	via.p.setX(e.attribute("x").toDouble());
	via.p.setY(e.attribute("y").toDouble());
	via.extent = e.attribute("extent");
	via.shape = Drawing::padRound;
	QString shape = e.attribute("shape");
	if (shape == "octagon")
		via.shape = Drawing::padOctagon;
	if (shape == "quare")
		via.shape = Drawing::padSquare;
	return via;
}
Drawing::Via parsePad(const QDomElement & e, double defaultRestring)
{
	Drawing::Via via;
	via.diameter = e.attribute("diameter").toDouble();
	via.drill = e.attribute("drill").toDouble();

	via.diameter = qMax(via.diameter, via.drill + 2.0 * defaultRestring);
	via.p.setX(e.attribute("x").toDouble());
	via.p.setY(e.attribute("y").toDouble());
	via.extent = "1-16";
	via.shape = Drawing::padRound;
	QString shape = e.attribute("shape");
	if (shape == "octagon")
		via.shape = Drawing::padOctagon;
	if (shape == "quare")
		via.shape = Drawing::padSquare;
	return via;
}

QList<Drawing::Element> parseBlock(QDomElement top)
{
	QDomNode node = top.firstChild();
	Drawing::Wire lastSegment;
	QList<Drawing::Element> elements;

	while(!node.isNull()) {
		if(node.isElement()) {
			QDomElement element = node.toElement();
//			qDebug()<<element.tagName();
			/* parse line elements
			 * connect line segments that directly follow each other
			 * this is the case if
			 * a) p2 of previous line is p1 of this line
			 * b) layer did not change
			 */
			if (element.tagName()=="wire") {
				Drawing::Wire w = parseLineElement(element);
				Drawing::Element e;
				e.e.wire = w;
				e.type = Drawing::Element::tWire;
				elements.append(e);
				lastSegment = w;
			}
			if (element.tagName() == "rectangle")
			{
				Drawing::Rectangle r = parseRectElement(element);
				Drawing::Element e;
				e.type = Drawing::Element::tRect;
				e.e.rect = r;
				elements.append(e);
			}
			if (element.tagName() == "polygon")
			{
				Drawing::Polygon p = parsePolygon(element);
				Drawing::Element e;
				e.type = Drawing::Element::tPolygon;
				e.e.poly = p;
				elements.append(e);
			}
			if (element.tagName() == "smd")
			{
				Drawing::Smd s = parseSMD(element);
				Drawing::Element e;
				e.type = Drawing::Element::tSmd;
				e.e.smd = s;
				elements.append(e);
			}
			if (element.tagName() == "via")
			{
				Drawing::Element e;
				e.e.via = parseVia(element, .15);
				e.type  = Drawing::Element::tVia;
				elements.append(e);
			}
			if (element.tagName() == "pad")
			{
				Drawing::Element e;
				e.e.via = parsePad(element, .15);
				e.type  = Drawing::Element::tVia;
				elements.append(e);
			}
			if (element.tagName() == "circle")
			{
				Drawing::Circle c = parseCircle(element);
				Drawing::Element e;
				e.type = Drawing::Element::tCircle;
				e.e.circle = c;
				elements.append(e);
			}
			if (element.tagName() == "hole")
			{
				Drawing::Circle c = parseHole(element);
				Drawing::Element e;
				e.type = Drawing::Element::tCircle;
				e.e.circle = c;
				elements.append(e);
			}
		}
		node = node.nextSibling();
	}
	return elements;
}

void Board::parseSignals(const QDomElement & top)
{
	QDomNode node = top.firstChild();
	while(!node.isNull()) {
		if(node.isElement()) {
			QDomElement element = node.toElement();
			if (element.tagName() == "signal")
			{
				netData netElements;
				netElements.net = element.attribute("name");
				netElements.elements = parseBlock(element);

				QDomElement cref = element.firstChildElement("contactref");
				while(!cref.isNull())
				{
					contactRef ref;
					ref.component = cref.attribute("element","");
					ref.pad       = cref.attribute("pad","");
					netElements.contectRef.push_back(ref);
//					qDebug()<<cref.tagName()<<netElements.net<<ref.component<<ref.pad;
					cref = cref.nextSiblingElement("contactref");
				}
				mData.net_data.push_back(netElements);
			}
		}
		node = node.nextSibling();
	}

}

QRegExp isPrefixWithNumber("([A-Z]+)(\\d+)", Qt::CaseInsensitive);

bool compareNames(Board::LibRef a, Board::LibRef b)
{
	QString prefix_a, prefix_b;
	int num_a, num_b;

	bool pwn_a = isPrefixWithNumber.exactMatch(a.name);
	if (pwn_a)
	{
		prefix_a = isPrefixWithNumber.cap(1);
		num_a = isPrefixWithNumber.cap(2).toInt();
	}
	bool pwn_b = isPrefixWithNumber.exactMatch(b.name);
	if (pwn_b)
	{
		prefix_b = isPrefixWithNumber.cap(1);
		num_b = isPrefixWithNumber.cap(2).toInt();
	}
	if ((pwn_a && pwn_b) && (prefix_a == prefix_b))
	{
		return num_a < num_b;
	}
	return (a.name < b.name);
}
//curve defines an arc with the second point being the center of the arc
void Board::parseBoard(QDomElement * brd) {
	//begin with stuff not in any net or symbol
	QDomElement plain = brd->firstChildElement("plain");
	auto elements = parseBlock(plain);
	if (!plain.isNull())
	{
		qDebug()<<"Plain section has"<<elements.size()<<"elements";
	} else {
		qDebug()<<"Board has no plain section";
	}
	netData plainData;
	plainData.net="";
	plainData.elements = elements;
	this->mData.net_data.push_back(plainData);
	QDomElement sigs = brd->firstChildElement("signals");
	parseSignals(sigs);
	QDomElement libs = brd->firstChildElement("libraries");
	QDomElement lib = libs.firstChildElement("library");
	while (!lib.isNull())
	{
		Library libData;
		libData.name = lib.attribute("name");
		libData.description = lib.firstChildElement("description").text();

		//parse for packages
		QDomElement pkgs = lib.firstChildElement("packages");
		QDomElement pkg = pkgs.firstChildElement("package");
		while(!pkg.isNull())
		{
			libraryEntry entry;
			entry.elements = parseBlock(pkg);
			libData.entries.insert(pkg.attribute("name"), entry);
			pkg = pkg.nextSiblingElement("package");
		}
		mData.libraryElements.insert(libData.name, libData);
		lib = lib.nextSiblingElement("library");
	}
	//finally get elements:

	QDomElement components = brd->firstChildElement("elements");
	QDomElement cmp = components.firstChildElement("element");
	while (!cmp.isNull())
	{
		QString name = cmp.attribute("name");
		QString library = cmp.attribute("library");
		QString package = cmp.attribute("package");
		QString value = cmp.attribute("value");
//		qDebug()<<name<<library<<package;
		LibRef ref;
		ref.name = name;
		ref.lib = library;
		ref.package = package;
		ref.value = value;
		QString rot = cmp.attribute("rot");
		ref.rotation = getRotation(rot);
		ref.isMirrored = getMirrored(rot);
		ref.pos.setX(cmp.attribute("x").toDouble());
		ref.pos.setY(cmp.attribute("y").toDouble());
		mData.components.append(ref);

		cmp = cmp.nextSiblingElement("element");
	}

	//sort elements by name
	std::sort(mData.components.begin(), mData.components.end(), compareNames);

	//<element name="IC1" library="linear" package="DIL08" value="LM358N" x="78.74" y="-39.37" rot="R90"/>

}


bool Board::hasLayer(int num) const {
	foreach(Eagle::Layer l, m_Layers) {
		if (l.number == num)
			return true;
	}
	return false;
}

const Eagle::Layer Board::getLayer(int num) const {
	foreach(Eagle::Layer l, m_Layers) {
		if (l.number == num)
			return l;
	}
	return Eagle::Layer();
}

QList<int> Board::getLayers() const {
	QList<int> ll;
	foreach(Eagle::Layer l, m_Layers) {
		ll.append(l.number);
	}
	return ll;
}

QGraphicsScene *Board::scene()
{
	if (!mScene)
		buildScene();
	return mScene;
}

void Board::buildScene()
{
	// if we have an old scene and are given a pointer, make sure the old scene gets deleted
	if (mScene)
	{
		disconnect(this, SLOT(onSceneDeleted()));
		mScene->deleteLater();
	}
	mScene = new QGraphicsScene(this);
	connect(mScene, SIGNAL(destroyed(QObject*)), SLOT(onSceneDeleted()));

	mScene->clear();
	//add net view

	foreach(netData nd, mData.net_data)
	{
		QGraphicsItem * itm = elementsToGfxItem(nd.elements, 0);
		mScene->addItem(itm);
	}

	foreach (LibRef ref, mData.components) {
		QString lib = ref.lib;
		QString pkg = ref.package;
		if (mData.libraryElements.contains(lib))
		{
			Library library = mData.libraryElements.value(lib);
			if (library.entries.contains(pkg))
			{
				QGraphicsItem * itm = elementsToGfxItem(library.entries.value(pkg).elements, 0, false);
				itm->setRotation(ref.rotation);
				itm->setPos(ref.pos);
				mScene->addItem(itm);
				mComponents.insert(ref.name, itm);
				itm = elementsToGfxItem(library.entries.value(pkg).elements, 0, true);
				itm->setRotation(ref.rotation);
				itm->setPos(ref.pos);
				itm->setVisible(false);
				mScene->addItem(itm);
				mSelectedComponents.insert(ref.name, itm);
			}
			else
			{
				qDebug()<<"lib item"<<pkg<<"not found in"<<lib;
				qDebug()<<"lib contents:"<<library.entries.keys();
			}
		}
	}
}

void Board::deselectAll()
{
	foreach(QGraphicsItem * itm, mComponents.values())
		itm->setVisible(true);
	foreach(QGraphicsItem * itm, mSelectedComponents.values())
		itm->setVisible(false);
}

bool Board::isSelected(QString designator)
{
	QGraphicsItem *itm = mSelectedComponents.value(designator, NULL);
	if (!itm)
		return false;
	return itm->isVisible();
}

QStringList Board::getSelected()
{
	QStringList ret;
	for(auto it = mSelectedComponents.begin(); it!=mSelectedComponents.end(); ++it)
	{
		if (it.value()->isVisible())
			ret.push_back(it.key());
	}
	return ret;
}

void Board::setSelected(QString part, bool onOff)
{
	QGraphicsItem * itm = mComponents.value(part, NULL);
	if (itm)
		itm->setVisible(!onOff);
	itm = mSelectedComponents.value(part, NULL);
	if (itm)
		itm->setVisible(onOff);
}

QStringList Board::designators() const
{
	return mComponents.keys();
}

void Board::onSceneDeleted()
{
	mScene = 0;
	mSelectedComponents.clear();
}

void Board::clear() {
	m_Layers.clear();
	m_Grid = Eagle::uUndefined;
	m_Lines.clear();
	mData.components.clear();
	mData.libraryElements.clear();
	mData.net_data.clear();
}

void Board::postError(QString s) {
	m_LastError = s;
	qDebug()<<s;
}

QPointF flipVertical(QPointF a)
{
	return QPointF(a.x(), -a.y());
}
QRectF arcRect(QPointF center, double radius)
{
	QRectF r(0, 0, 2.0 * radius, 2.0 * radius);
	r.moveCenter(center);
	return r;
}
QGraphicsItemGroup *Board::elementsToGfxItem(const QList<Drawing::Element> &data, QGraphicsItem *parent, bool hiLight)
{
	QGraphicsItemGroup *part = new QGraphicsItemGroup(parent);
	foreach(Drawing::Element e, data)
	{
		switch (e.type)
		{
			case Drawing::Element::tWire:
			{
				Drawing::Wire w = e.e.wire;
				if (!layerActive(w.layer))
					break;
				QPainterPath path;
				path.moveTo(w.p1);
				path.lineTo(w.p2);
//				if (w.curve)
//				{
//					QPointF center = arcCenter(w.p1, w.p2, w.curve);
//					double r = distance(center, w.p1);
//					path.moveTo(center);
//					QPointF d = w.p1 - center;
//					double a1 = atan2(d.y(), d.x());
//					d = w.p2 - center;
//					double a2 = atan2(d.y(), d.x());
//					qDebug()<<w.curve<<rad2deg(a1-a2);
//					path.arcTo(arcRect(center, r), -rad2deg(a1),-rad2deg(a2));
//				}
				QGraphicsPathItem *itm = new QGraphicsPathItem(path, part);
				QPen linePen;
				linePen.setCapStyle(Qt::RoundCap);
				linePen.setWidthF(qMin(0.2, w.width));
				linePen.setColor(hiLight?mHilightColor:layerColor(w.layer));
				itm->setPen(linePen);
			}
				break;
			case Drawing::Element::tRect:
			{
				Drawing::Rectangle r = e.e.rect;
				if (!layerActive(r.layer))
					break;
				QBrush fillBrush;
				fillBrush.setColor(hiLight?mHilightColor:layerColor(r.layer));
				fillBrush.setStyle(Qt::SolidPattern);
				QGraphicsRectItem *itm = new QGraphicsRectItem(QRectF(r.p1, r.p2), part);
				itm->setBrush(fillBrush);
				itm->setPen(Qt::NoPen);
			}
				break;
			case Drawing::Element::tCircle:
			{
				Drawing::Circle c = e.e.circle;
				if (!layerActive(c.layer))
					break;
				QGraphicsEllipseItem *itm= new QGraphicsEllipseItem(part);
				itm->setRect(-c.radius,-c.radius,2.0*c.radius,2.0*c.radius);
				itm->setPos(c.center);
				QPen linePen = Qt::NoPen;
				QBrush fillBrush = Qt::NoBrush;
				if (c.width == 0)
				{
					fillBrush.setColor(hiLight?mHilightColor:layerColor(c.layer));
					fillBrush.setStyle(Qt::SolidPattern);
				}
				else
				{
					linePen.setColor(hiLight?mHilightColor:layerColor(c.layer));
					linePen.setWidthF((c.width>0)?c.width:0.05);
					linePen.setStyle(Qt::SolidLine);
				}
				itm->setBrush(fillBrush);
				itm->setPen(linePen);
			}
				break;
			case Drawing::Element::tPolygon:
			{
				Drawing::Polygon p = e.e.poly;
				if (!layerActive(p.layer))
					break;
				QPolygonF poly;
				foreach(auto vertex, p.vertices)
					poly.append(vertex.p);
				QGraphicsPolygonItem *itm = new QGraphicsPolygonItem(poly, part);
				QColor col = hiLight?mHilightColor:layerColor(p.layer);
				col.setAlpha(64);
				itm->setBrush(col);
				itm->setPen(Qt::NoPen);
			}
				break;
			case Drawing::Element::tSmd:
			{
				Drawing::Smd smd = e.e.smd;
				if (!layerActive(smd.layer))
					break;
				QRectF rct(QPointF(0,0), smd.size);
				rct.moveCenter(smd.center);
				QGraphicsRectItem *itm = new QGraphicsRectItem(rct, part);
				itm->setRotation(smd.rotation);
				itm->setPen(Qt::NoPen);
				itm->setBrush(hiLight?mHilightColor:layerColor(smd.layer));
			}
			case Drawing::Element::tVia:
			{
				Drawing::Via via = e.e.via;
				if (!layerActive(18))
					break;
				QPainterPath path;
				switch(via.shape)
				{
					case Drawing::padRound:
						path.addEllipse(via.p, via.diameter * 0.5, via.diameter * 0.5);
					break;
					case Drawing::padSquare:
						path.addRect(via.p.x() - via.diameter*.5,
						             via.p.y() - via.diameter*.5,
						             via.diameter,
						             via.diameter);
						break;
					case Drawing::padOctagon:
						double a = via.diameter / 2.414;
						double r = via.diameter / 2.0;
						path.moveTo(via.p.x() - a * .5, via.p.y() - r);
						path.lineTo(via.p.x() + a * .5, via.p.y() - r);
						path.lineTo(via.p.x() + r, via.p.y() - a * .5);
						path.lineTo(via.p.x() + r, via.p.y() + a * .5);
						path.lineTo(via.p.x() + a * .5, via.p.y() + r);
						path.lineTo(via.p.x() - a * .5, via.p.y() + r);
						path.lineTo(via.p.x() - r, via.p.y() + a*.5);
						path.lineTo(via.p.x() - r, via.p.y() - a*.5);
						path.moveTo(via.p.x() - a * .5, via.p.y() - r);
						break;
				}
				path.addEllipse(via.p, via.drill * .5, via.drill * .5);
				QGraphicsPathItem *itm = new QGraphicsPathItem(path, part);

				itm->setPen(Qt::NoPen);
				itm->setBrush(hiLight?mHilightColor:layerColor(18));


			}
				break;
			default:
				qWarning() << "unhandled item type" << e.type;
		}
	}
	return part;
}


componentListModel::componentListModel(Board *board, QObject *parent) :
    QAbstractTableModel(parent), mRows(0), mBoard(board)
{
	connect(mBoard, SIGNAL(boardChanged()), SLOT(onBoardChanged()));
	onBoardChanged();
}

int componentListModel::rowCount(const QModelIndex &parent) const
{
	return mRows;
}


int componentListModel::columnCount(const QModelIndex &parent) const
{
	return 4;
}

QVariant componentListModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	int col = index.column();
	if (role!=Qt::DisplayRole)
		return QVariant();
	if (row>=mBoard->mData.components.size())
		return QVariant();

	switch(col)
	{
		case 0:
			return mBoard->mData.components[row].name;
		case 1:
			return mBoard->mData.components[row].lib;
		case 2:
			return mBoard->mData.components[row].package;
		case 3:
			return mBoard->mData.components[row].value;
		default:
			break;
	}
	return QVariant();
}

QVariant componentListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role!=Qt::DisplayRole)
		return QVariant();
	if (orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
		case 0:
			return tr("Designator");
		case 1:
			return tr("Library");
		case 2:
			return tr("Footprint");
		case 3:
			return tr("Value");
		default:
			return QVariant();
	}
}

void componentListModel::updateSelection(const QItemSelection &selected, const QItemSelection &deselected)
{
	QModelIndex index;
	QModelIndexList items = deselected.indexes();
	foreach (index, items) {
		if (index.column()==0)
		{
			mBoard->setSelected(data(index, Qt::DisplayRole).toString(), false);
		}
	}
	items = selected.indexes();
	foreach (index, items) {
		if (index.column()==0)
		{
			mBoard->setSelected(data(index, Qt::DisplayRole).toString(), true);
		}
	}

}

void componentListModel::onBoardChanged()
{
	emit beginRemoveRows(QModelIndex(),0, mRows);
	emit endRemoveRows();
	mRows = mBoard->mData.components.size();
	emit beginInsertRows(QModelIndex(),0, mRows);
	emit endInsertRows();

}
