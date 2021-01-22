#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QIODevice>
#include <QList>
#include <QHash>
#include "eagle.h"
#include <QPoint>
#include <QSharedPointer>
#include <QDebug>
#include <QAbstractTableModel>
#include <QItemSelection>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsItemGroup;
class Board;
class componentListModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	componentListModel(Board* board, QObject *parent = 0);
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
public slots:
	void updateSelection(const QItemSelection &selected,
	    const QItemSelection &deselected);
private slots:
	void onBoardChanged();
private:
	Board *mBoard;
	int mRows;
};


//	    QModelIndex index;
//	    QModelIndexList items = selected.indexes();

//	    foreach (index, items) {
//	        QString text = QString("(%1,%2)").arg(index.row()).arg(index.column());
//	        model->setData(index, text);
//	    }

//	    items = deselected.indexes();

//	    foreach (index, items)
//	        model->setData(index, "");



//typedef QSharedPointer<QList<Line> > lineListPtr;
struct libraryEntry
{
	QList<Drawing::Element> elements;
};

class QDomElement;
class Board : public QObject {
	Q_OBJECT
	friend class componentListModel;
public:
	struct Library
	{
		QString name;
		QString description;
		QMap<QString, libraryEntry> entries;
	};

	struct contactRef
	{
		QString component;
		QString pad;
	};
	struct netData
	{
		QString net;
		QList<Drawing::Element> elements;
		QList<contactRef> contectRef;
	};
	struct LibRef
	{
		QString name;
		QString lib;
		QString package;

		QPointF pos;
		QString value;
		int rotation;
		bool isMirrored;
		int Layer;
	};

	struct BoardData
	{
		QList<netData> net_data;
		QMap<QString, Library> libraryElements;
		QList<LibRef> components;
	};
	struct bomEntry
	{
		QString partId;
		QString library;
		QString package;
		QString value;
		QStringList designators;
	};

	QList<bomEntry> generateBom() const;



	explicit Board(QObject *parent = 0);
	bool parseFile(QIODevice &);
	QString getLastError() const;

	//layer functions
	bool hasLayer(int num) const;
	const Eagle::Layer getLayer(int num) const;
	QList<int> getLayers() const;

	QGraphicsScene * scene();
	void buildScene();

	void deselectAll();
	bool isSelected(QString designator);
	QStringList getSelected();
	void setSelected(QString part, bool onOff);
	QStringList designators() const;
signals:
	void boardChanged();
public slots:
private slots:
	void onSceneDeleted();
private:

	QGraphicsScene * mScene;
	QMap<QString, QGraphicsItem*> mComponents;
	QMap<QString, QGraphicsItem*> mSelectedComponents;
	BoardData mData;
	void parseLayers(QDomElement*);
	void parseBoard(QDomElement*);

	void clear();
	QString m_LastError;
	void postError(QString);
	QList<Eagle::Layer> m_Layers;
	QList<Drawing::PolyLine> m_Lines;

	Eagle::GridUnit m_Grid;

	QGraphicsItemGroup *elementsToGfxItem(const QList<Drawing::Element> & data, QGraphicsItem * parent, bool hilight = false);
	void parseSignals(const QDomElement &top);
	QColor mHilightColor;
};

#endif // BOARD_H
