#include "bommodel.h"

BomModel::BomModel(QObject *parent) : QAbstractTableModel(parent)
{

}

int BomModel::rowCount(const QModelIndex &parent) const
{
	return mBom.size();
}

int BomModel::columnCount(const QModelIndex &parent) const
{
	return 4;
}

QVariant BomModel::data(const QModelIndex &index, int role) const
{
	if (role!=Qt::DisplayRole)
		return QVariant();
	int row = index.row();
	int col = index.column();
	if (row>=mBom.size())
		return QVariant();
	switch(col)
	{
		case 0:
			return mBom[row].designators.size();
		case 1:
			return mBom[row].package;
		case 2:
			return mBom[row].value;
		case 3:
			return mBom[row].designators.join(", ");
		default:
			return QVariant();
	}
	return QVariant();
}


QVariant BomModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role!=Qt::DisplayRole)
		return QVariant();
	if (orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
		case 0:
			return tr("Qty");
		case 1:
			return tr("Footprint");
		case 2:
			return tr("Value");
		case 3:
			return tr("Designators");
		default:
			return QVariant();
	}
}

QStringList BomModel::getDesignators(QModelIndexList indices)
{
	QList<int> rows;
	for(int i=0;i<indices.size();++i)
	{
		if (!rows.contains(indices[i].row()))
			rows.push_back(indices[i].row());
	}
	QStringList designators;
	foreach(int row, rows)
	{
		designators << mBom[row].designators;
	}
	return designators;
}

void BomModel::setBom(QList<Board::bomEntry> newBom)
{
	emit beginRemoveRows(QModelIndex(), 0, mBom.size());
	emit endRemoveRows();
	mBom = newBom;
	emit beginInsertRows(QModelIndex(), 0, mBom.size());
	emit endInsertRows();
}
