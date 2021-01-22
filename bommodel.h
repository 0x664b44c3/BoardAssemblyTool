#ifndef BOMMODEL_H
#define BOMMODEL_H

#include <QAbstractTableModel>
#include <board.h>

class BomModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit BomModel(QObject *parent = nullptr);
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QStringList getDesignators(QModelIndexList indices);
	void setBom(QList<Board::bomEntry>);
public slots:

private slots:


signals:
private:
	QList<Board::bomEntry> mBom;

};

#endif // BOMMODEL_H
