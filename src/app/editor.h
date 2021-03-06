// Konstruktor - An interactive LDraw modeler for KDE
// Copyright (c)2006-2011 Park "segfault" J. K. <mastermind@planetmono.org>

#ifndef _EDITOR_H_
#define _EDITOR_H_

#include <QSet>
#include <QUndoGroup>

#include <kaction.h>

#include <libldr/color.h>
#include <libldr/math.h>

#include "commandbase.h"

namespace ldraw
{
    class model;
}

class KActionCollection;
class KMenu;

namespace Konstruktor
{

class UndoAction : public KAction 
{
	Q_OBJECT;

  public:
	UndoAction(const QString &prefix, QObject *parent = 0L);

  public slots:
	void setPrefixedText(const QString &text);

  private:
	QString prefix_;
};


class Editor : public QUndoGroup
{
	Q_OBJECT;
	
  public:
	enum GridMode { Grid20, Grid10, Grid5, Grid1 };
	enum RotationPivot { PivotEach, PivotCenter, PivotManual };
	enum Axis { AxisX, AxisY, AxisZ };	
	
	Editor(QObject *parent = 0L);
	~Editor();

	KAction* createRedoAction(KActionCollection *actionCollection);
	KAction* createUndoAction(KActionCollection *actionCollection);

	GridMode gridMode() const { return gridMode_; }

	float snap(float v) const;
	float snapYAxis(float v) const;
	float gridDensity() const;
	float gridDensityYAxis() const;
	float gridDensityAngle() const;

  signals:
	void selectionIndexModified(const QSet<int> &selection);
	void selectionRemoved(const QSet<int> &selection);
	void objectInserted(int offset, int items);
	void rowsChanged(const QPair<CommandBase::AffectedRow, QSet<int> > &rowList);
	void modified();
	void needRepaint();

  public slots:
	void selectionChanged(const QSet<int> &selection);
	void modelChanged(ldraw::model *model);
	void activeChanged(QUndoStack *stack);
	void stackAdded(QUndoStack *stack);

	void setGridMode(GridMode mode);
	
	// Editing
	void cut();
	void copy();
	void paste();
	void deleteSelected();
	void editColor();
	void rotationPivot();
	void move(const ldraw::vector &vector);
	void moveByXPositive();
	void moveByXNegative();
	void moveByYPositive();
	void moveByYNegative();
	void moveByZPositive();
	void moveByZNegative();
	void rotateByXClockwise();
	void rotateByXCounterClockwise();
	void rotateByYClockwise();
	void rotateByYCounterClockwise();
	void rotateByZClockwise();
	void rotateByZCounterClockwise();
	void insert(const QString &filename, const ldraw::matrix &matrix, const ldraw::color &color);

  private slots:
	void indexChanged(int index);

  private:
	GridMode gridMode_;
	RotationPivot pivot_;

	// Stacks
	QUndoStack *activeStack_;
	bool changedFlag_;
	int lastIndex_;
	
	const QSet<int> *selection_;
	ldraw::model *model_;
};

}

#endif
