#include "disassemblerpopup.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QLayout>

DisassemblerPopup::DisassemblerPopup(const RDContextPtr& ctx, QWidget *parent): QWidget(parent), m_context(ctx)
{
    m_popupview = new DisassemblerPopupView(ctx, this);

    QVBoxLayout* vboxlayout = new QVBoxLayout(this);
    vboxlayout->setContentsMargins(0, 0, 0, 0);
    vboxlayout->addWidget(m_popupview);
    this->setLayout(vboxlayout);

    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setWindowFlags(Qt::Popup);
    this->setMouseTracking(true);
    this->setMinimumHeight(0);
    this->setMinimumWidth(0);
}

void DisassemblerPopup::popup(const RDSymbol* symbol)
{
    if(!symbol || !m_popupview->renderPopup(symbol))
    {
        this->hide();
        return;
    }

    QPoint pt = QCursor::pos();
    pt.rx() += POPUP_MARGIN;
    pt.ry() += POPUP_MARGIN;

    this->move(pt);
    this->show();
}

void DisassemblerPopup::mouseMoveEvent(QMouseEvent* event)
{
    if(m_lastpos != event->globalPos())
    {
        this->hide();
        event->accept();
    }
    else
        QWidget::mouseMoveEvent(event);
}

void DisassemblerPopup::wheelEvent(QWheelEvent* event)
{
    m_lastpos = event->globalPosition();
    QPoint delta = event->angleDelta();

    if(delta.y() > 0) m_popupview->lessRows();
    else m_popupview->moreRows();
    event->accept();
}
