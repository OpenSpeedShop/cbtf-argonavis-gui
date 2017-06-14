#include "ThreadSelectionCommand.h"

#include "widgets/TreeModel.h"

namespace ArgoNavis { namespace GUI {


/**
 * @brief ThreadSelectionCommand::ThreadSelectionCommand
 * @param index
 * @param selected
 * @param parent
 */
ThreadSelectionCommand::ThreadSelectionCommand(QPersistentModelIndex *index, bool selected, QUndoCommand *parent)
    : QUndoCommand( parent )
    , m_model( Q_NULLPTR )
    , m_index( index )
    , m_selected( selected )
    , m_ready( false )
{
    if ( index ) {
        m_model = qobject_cast< TreeModel* >( const_cast< QAbstractItemModel* >( index->model() ) );
    }
}

/**
 * @brief ThreadSelectionCommand::undo
 */
void ThreadSelectionCommand::undo()
{
    QModelIndex index = m_model->createIndex( m_index->row(), m_index->column(), m_index->internalPointer() );
    m_model->setData( index, ! m_selected, Qt::CheckStateRole );
}

/**
 * @brief ThreadSelectionCommand::redo
 */
void ThreadSelectionCommand::redo()
{
    if ( m_ready ) {
        QModelIndex index = m_model->createIndex( m_index->row(), m_index->column(), m_index->internalPointer() );
        m_model->setData( index, m_selected, Qt::CheckStateRole );
    }
    else {
        m_ready = true;
    }
}


} // GUI
} // ArgoNavis
