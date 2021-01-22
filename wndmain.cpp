#include "wndmain.h"
#include "ui_wndmain.h"
#include <QDebug>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QDockWidget>
#include <QTableView>
#include <QHeaderView>
#include "previewwidget.h"
#include <bommodel.h>

wndMain::wndMain(QWidget *parent) :
   QMainWindow(parent),
	ui(new Ui::wndMain) {
	ui->setupUi(this);
	QDockWidget *dock = new QDockWidget("BoM", this);
	QTableView * tw = new QTableView(this);
	dock->setWidget(tw);
	dock->show();
	dock->setFeatures(dock->features() & (~QDockWidget::DockWidgetClosable));
	tw->setModel(mBomModel = new BomModel(this));
	tw->setSelectionBehavior(QAbstractItemView::SelectRows);
	tw->verticalHeader()->hide();
	addDockWidget(Qt::LeftDockWidgetArea, dock);

	mBomView = tw;
	dock = new QDockWidget(tr("Components"), this);
	tw = new QTableView(this);
	dock->setWidget(tw);
	dock->show();
	dock->setFeatures(dock->features() & (~QDockWidget::DockWidgetClosable));
	mComponentView = tw;
	addDockWidget(Qt::LeftDockWidgetArea, dock);

	dock = mPreview = new previewWidget(this);
	dock->show();
	dock->setFeatures(dock->features() & (~QDockWidget::DockWidgetClosable));
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	connect(ui->graphicsView, SIGNAL(viewportChanged(QRectF)), mPreview, SLOT(onViewChanged(QRectF)));
	componentListModel * mod = new componentListModel(&m_Board, this);
	tw->setModel(mod);
	tw->setSelectionBehavior(QAbstractItemView::SelectRows);
	tw->verticalHeader()->hide();
	connect(tw->selectionModel(),
	        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
	        mod,
	        SLOT(updateSelection(QItemSelection,QItemSelection)));
	connect(mBomView->selectionModel(),
	        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
	        this,
	        SLOT(onBomSelectionChanged()));
	ui->actionLoad_project->setVisible(false);
	ui->actionOpen_BoM->setVisible(false);
	ui->actionSave_Project->setVisible(false);
	ui->action_Reset_new_project->setVisible(false);
}

wndMain::~wndMain()
{
	delete ui;
}
/*
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="6.5.0">
*/

void wndMain::on_actionOpen_Board_triggered()
{
	QString filters=tr("Board files (*.brd);;All Files (*.*)" );
	m_LastDirectory = trUtf8("/home/andreas/eagle/Intercom/Partyline/");
	QString fileName=
	        QFileDialog::getOpenFileName(
	            this,trUtf8("Import eagle board file"),
	            m_LastDirectory, filters);
	if(fileName.length()) {
		QFile f(fileName);
		if (f.open(QIODevice::ReadOnly)) {
			if (!m_Board.parseFile(f))
				qDebug()<<"Could not load file"<<m_Board.getLastError();
			else
			{
				m_Board.buildScene();
				QGraphicsScene * scn = m_Board.scene();
				ui->graphicsView->setScene(scn);
				QTransform xfm;
				xfm.scale(1,-1);
				ui->graphicsView->setTransform(xfm);
				qDebug()<<"Scene rect:"<<scn->sceneRect();
				ui->graphicsView->fitInView(scn->sceneRect(), Qt::KeepAspectRatio);
				mPreview->setScene(scn);
			}
		} else {
			qDebug()<<"Could not open file";
		}
	}
	auto bom = m_Board.generateBom();
	std::sort(bom.begin(), bom.end(), [](const Board::bomEntry & a, const Board::bomEntry & b)
	{return (a.partId > b.partId);});
	std::sort(bom.begin(), bom.end(), [](const Board::bomEntry & a, const Board::bomEntry & b)
	{return (a.designators.size() > b.designators.size());});
	mBomModel->setBom(bom);

}

void wndMain::on_actionZoom_to_fit_triggered()
{
	if (!ui->graphicsView->scene())
		return;
	ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(), Qt::KeepAspectRatio);

}

void wndMain::onBomSelectionChanged()
{
	QModelIndexList selectedRows = mBomView->selectionModel()->selectedRows();
	QStringList designators = mBomModel->getDesignators(selectedRows);
	mComponentView->selectionModel()->clearSelection();
	m_Board.deselectAll();
	foreach(QString i, designators)
		m_Board.setSelected(i, true);
}
