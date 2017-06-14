#ifndef THREADSELECTIONCOMMAND_H
#define THREADSELECTIONCOMMAND_H

#include <QUndoCommand>
#include <QPersistentModelIndex>

#include "common/openss-gui-config.h"


namespace ArgoNavis { namespace GUI {


class TreeModel;


class ThreadSelectionCommand : public QUndoCommand
{
public:

   explicit ThreadSelectionCommand(QPersistentModelIndex* index, bool selected = true,  QUndoCommand *parent = 0);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:

    TreeModel* m_model;
    const QPersistentModelIndex* m_index;
    bool m_selected;
    bool m_ready;

};


} // GUI
} // ArgoNavis

#endif // THREADSELECTIONCOMMAND_H
