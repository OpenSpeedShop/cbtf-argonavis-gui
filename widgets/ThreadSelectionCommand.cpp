#include "ThreadSelectionCommand.h"

#include "widgets/TreeItem.h"
#include "widgets/TreeModel.h"

namespace ArgoNavis { namespace GUI {


/**
 * @brief ThreadSelectionCommand::ThreadSelectionCommand
 * @param model - pointer to tree model instance
 * @param item - pointer to tree item instance
 * @param selected - indicates whether select or unselect command
 * @param parent - the parent QUndoCommand (if any)
 *
 * Constructor for ThreadSelectionCommand object.
 */
ThreadSelectionCommand::ThreadSelectionCommand(TreeModel* model, TreeItem *item, bool selected, QUndoCommand *parent)
    : QUndoCommand( parent )
    , m_model( model )
    , m_index( Q_NULLPTR )
    , m_selected( selected )
    , m_ready( false )
{
    if ( model && item ) {
        m_index = new QPersistentModelIndex( model->createIndex( item->row(), 0, item->parentItem() ) );
    }
}

/**
 * @brief ThreadSelectionCommand::~ThreadSelectionCommand
 *
 * Destructor for ThreadSelectionCommand object.
 */
ThreadSelectionCommand::~ThreadSelectionCommand()
{
    delete m_index;
}

/**
 * @brief ThreadSelectionCommand::undo
 *
 * Overrides implementation of QUndoCommand::undo() method.  Implements the specific logic of the undo action.
 */
void ThreadSelectionCommand::undo()
{
    if ( m_model && m_index ) {
        QModelIndex index = m_model->createIndex( m_index->row(), m_index->column(), m_index->internalPointer() );
        m_model->setData( index, ! m_selected, Qt::CheckStateRole );
    }
}

/**
 * @brief ThreadSelectionCommand::redo
 *
 * Overrides implementation of QUndoCommand::redo() method.  Implements the specific logic of the redo action.
 */
void ThreadSelectionCommand::redo()
{
    if ( m_ready ) {
        if ( m_model && m_index ) {
            QModelIndex index = m_model->createIndex( m_index->row(), m_index->column(), m_index->internalPointer() );
            m_model->setData( index, m_selected, Qt::CheckStateRole );
        }
    }
    else {
        m_ready = true;
    }
}


} // GUI
} // ArgoNavis
