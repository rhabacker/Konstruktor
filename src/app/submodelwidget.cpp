// Konstruktor - An interactive LDraw modeler for KDE
// Copyright (c)2006-2011 Park "segfault" J. K. <mastermind@planetmono.org>

#include <QContextMenuEvent>
#include <QMenu>

#include <kaction.h>
#include <klocale.h>

#include "application.h"
#include "pixmapextension.h"
#include "submodelmodel.h"

#include "submodelwidget.h"

namespace Konstruktor
{

SubmodelWidget::SubmodelWidget(QWidget *parent)
	: QTreeView(parent)
{
	setRootIsDecorated(false);
	setHeaderHidden(true);
	setVerticalScrollMode(QTreeView::ScrollPerPixel);
	setDragEnabled(true);
}

SubmodelWidget::~SubmodelWidget()
{

}

void SubmodelWidget::modelChanged(ldraw::model *m)
{
	if (!m)
		return;
	
	SubmodelModel *sm = dynamic_cast<SubmodelModel *>(model());
	
	QModelIndex index = sm->index(m);

	if (!index.isValid())
		return;
	
	if (!previous_.isValid())
		previous_ = model()->index(0, 0);
	else {
		ldraw::model *m = sm->modelIndexOf(previous_).second;

		if (m) {
			std::list<ldraw::model *> affected = PixmapExtension::updateRelevant(m, Application::self()->pixmapRenderer());

			for (std::list<ldraw::model *>::iterator it = affected.begin(); it != affected.end(); ++it) {
				QModelIndex ii = sm->index(*it);

				if (ii.isValid())
					sm->setData(ii, QVariant(), Qt::DecorationRole);
			}
		}
	}
	
	model()->setData(previous_, QVariant(0));
	model()->setData(index, QVariant(1));
	
	previous_ = index;
}

void SubmodelWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if (selectedIndexes().count() != 1) {
		event->ignore();
		return;
	}
	
	QMenu contextMenu(this);
	contextMenu.addAction(new KAction(KIcon("edit-copy"), i18n("&Duplicate"), &contextMenu));

	QAction *action = contextMenu.exec(event->globalPos());
	if (action) {


	}

	event->accept();
}



}
