#include "ThreadSelectionCommand.h"

#include "widgets/TreeItem.h"
#include "widgets/TreeModel.h"

namespace ArgoNavis { namespace GUI {


/**
 * @brief ThreadSelectionCommand::ThreadSelectionCommand
 * @param index
 * @param selected
 * @param parent
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
 * @brief ThreadSelectionCommand::undo
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
