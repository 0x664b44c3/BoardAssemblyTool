#ifndef WNDMAIN_H
#define WNDMAIN_H

#include <QMainWindow>
#include "board.h"
class previewWidget;
class QTreeView;
class QTableView;
namespace Ui {
class wndMain;
}

class BomModel;
class wndMain : public QMainWindow
{
	Q_OBJECT

public:
	explicit wndMain(QWidget *parent = 0);
	~wndMain();

private slots:
	void on_actionOpen_Board_triggered();

	void on_actionZoom_to_fit_triggered();
	void onBomSelectionChanged();
private:
	Ui::wndMain *ui;
	QString m_LastDirectory;
	Board m_Board;
	previewWidget *mPreview;
	QTableView *mComponentView;
	QTableView *mBomView;
	BomModel * mBomModel;

};

#endif // WNDMAIN_H
