#include "disassemblerview.h"
#include "../hooks/disassemblerhooks.h"
#include "../models/symboltablemodel.h"
#include "../models/segmentsmodel.h"
#include "tabs/tabletab/tabletab.h"
#include "tabs/listingtab/listingtab.h"
#include "docks/listingmapdock/listingmapdock.h"
#include <QMessageBox>
#include <QBoxLayout>

DisassemblerView::DisassemblerView(const RDDisassemblerPtr& disassembler, QWidget *parent) : QWidget(parent), m_disassembler(disassembler)
{   
    m_disassemblertabs = new DisassemblerTabs(disassembler, this);

    QBoxLayout* boxlayout = new QBoxLayout(QBoxLayout::TopToBottom);
    boxlayout->setContentsMargins(0, 0, 0, 0);
    boxlayout->setSpacing(0);
    boxlayout->addWidget(m_disassemblertabs);
    this->setLayout(boxlayout);

    ICommandTab* commandtab = this->showListing();
    m_listingmapdock = new ListingMapDock(commandtab->command());

    this->showFunctions(Qt::LeftDockWidgetArea);
    this->showSegments();
    this->showExports();
    this->showImports();
    this->showStrings();
    this->dock(m_listingmapdock, Qt::RightDockWidgetArea);

    RDDisassembler_Subscribe(disassembler.get(), this, &DisassemblerView::listenEvents, this);

    m_worker = std::async([&, disassembler]() { // Capture 'disassembler' by value
        RD_Disassemble(disassembler.get());
        QMetaObject::invokeMethod(DisassemblerHooks::instance(), "enableViewCommands", Qt::QueuedConnection, Q_ARG(bool, true));
    });
}

DisassemblerView::~DisassemblerView()
{
    RDDisassembler_Unsubscribe(m_disassembler.get(), this);

    if(m_worker.valid()) m_worker.get();
    while(!m_docks.empty()) this->undock(*m_docks.begin());

    RD_Status("");
}

QWidget* DisassemblerView::currentWidget() const { return m_disassemblertabs->currentWidget(); }

ITableTab* DisassemblerView::showSegments(Qt::DockWidgetArea area)
{
    if(auto* t = this->findModelInTabs<SegmentsModel>())
    {
        m_disassemblertabs->setCurrentWidget(dynamic_cast<QWidget*>(t));
        return t;
    }

    TableTab* tabletab = this->createTable(new SegmentsModel(), "Segments");
    connect(tabletab, &TableTab::resizeColumns, tabletab, &TableTab::resizeAllColumns);

    if(area == Qt::NoDockWidgetArea) this->tab(tabletab);
    else this->dock(tabletab, area);
    return tabletab;
}

ITableTab* DisassemblerView::showFunctions(Qt::DockWidgetArea area)
{
    if(auto* t = this->findModelInTabs<ListingItemModel>())
    {
        if(t->model()->itemType() == DocumentItemType_Function)
        {
            m_disassemblertabs->setCurrentWidget(dynamic_cast<QWidget*>(t));
            return t;
        }
    }

    TableTab* tabletab = this->createTable(new ListingItemModel(DocumentItemType_Function), "Functions");
    tabletab->setColumnHidden(1);
    tabletab->setColumnHidden(2);
    connect(tabletab, &TableTab::resizeColumns, this, [tabletab]() { tabletab->resizeColumn(0); });

    if(area == Qt::NoDockWidgetArea) this->tab(tabletab);
    else this->dock(tabletab, area);
    return tabletab;
}

ITableTab* DisassemblerView::showExports(Qt::DockWidgetArea area)
{
    if(auto* t = this->findSymbolModelInTabs(SymbolType_None, SymbolFlags_Export))
    {
        m_disassemblertabs->setCurrentWidget(dynamic_cast<QWidget*>(t));
        return t;
    }

    auto* model = new SymbolTableModel(DocumentItemType_All);
    model->setSymbolFlags(SymbolFlags_Export);

    TableTab* tabletab = this->createTable(model, "Exports");
    connect(tabletab, &TableTab::resizeColumns, tabletab, &TableTab::resizeAllColumns);

    if(area == Qt::NoDockWidgetArea) this->tab(tabletab);
    else this->dock(tabletab, area);
    return tabletab;
}

ITableTab* DisassemblerView::showImports(Qt::DockWidgetArea area)
{
    if(auto* t = this->findSymbolModelInTabs(SymbolType_Import, SymbolFlags_None))
    {
        m_disassemblertabs->setCurrentWidget(dynamic_cast<QWidget*>(t));
        return t;
    }

    auto* model = new SymbolTableModel(DocumentItemType_Symbol);
    model->setSymbolType(SymbolType_Import);

    TableTab* tabletab = this->createTable(model, "Imports");
    connect(tabletab, &TableTab::resizeColumns, tabletab, &TableTab::resizeAllColumns);

    if(area == Qt::NoDockWidgetArea) this->tab(tabletab);
    else this->dock(tabletab, area);
    return tabletab;
}

ITableTab* DisassemblerView::showStrings(Qt::DockWidgetArea area)
{
    if(auto* t = this->findSymbolModelInTabs(SymbolType_String, SymbolFlags_None))
    {
        m_disassemblertabs->setCurrentWidget(dynamic_cast<QWidget*>(t));
        return t;
    }

    auto* model = new SymbolTableModel(DocumentItemType_Symbol);
    model->setSymbolType(SymbolType_String);

    TableTab* tabletab = this->createTable(model, "Strings");
    connect(tabletab, &TableTab::resizeColumns, tabletab, &TableTab::resizeAllColumns);

    if(area == Qt::NoDockWidgetArea) this->tab(tabletab);
    else this->dock(tabletab, area);
    return tabletab;
}

const RDDisassemblerPtr& DisassemblerView::disassembler() const { return m_disassembler; }

ITableTab* DisassemblerView::findSymbolModelInTabs(rd_type type, rd_flag flags) const
{
    for(int i = 0; i < m_disassemblertabs->count(); i++)
    {
        auto* tabletab = dynamic_cast<ITableTab*>(m_disassemblertabs->widget(i));
        if(!tabletab) continue;

        auto* symboltablemodel = dynamic_cast<SymbolTableModel*>(tabletab->model());
        if(!symboltablemodel || (symboltablemodel->symbolType() != type)) continue;
        if(symboltablemodel->symbolFlags() != flags) continue;
        return tabletab;
    }

    return nullptr;
}

TableTab* DisassemblerView::createTable(ListingItemModel* model, const QString& title)
{
    TableTab* tabletab = new TableTab(model);
    model->setParent(tabletab);
    tabletab->setWindowTitle(title);
    return tabletab;
}

void DisassemblerView::listenEvents(const RDEventArgs* e)
{
    auto* thethis = reinterpret_cast<DisassemblerView*>(e->userdata);

    switch(e->eventid)
    {
        case Event_BusyChanged:
            QMetaObject::invokeMethod(DisassemblerHooks::instance(), "updateViewWidgets", Qt::QueuedConnection, Q_ARG(bool, RDDisassembler_IsBusy(thethis->m_disassembler.get())));
            break;

        case Event_CursorPositionChanged: {
            auto* hooks = DisassemblerHooks::instance();
            const auto* ce  = reinterpret_cast<const RDCursorEventArgs*>(e);

            if(hooks->activeCommandTab()) {
                if(hooks->activeCommandTab()->command()->cursor() != ce->sender) return;
                hooks->statusAddress(hooks->activeCommandTab()->command());
            }
            else rd_status(std::string());
            break;
        }

        case Event_Error: {
            const auto* ee = reinterpret_cast<const RDErrorEventArgs*>(e);
            QMetaObject::invokeMethod(DisassemblerHooks::instance(), "showMessage", Qt::QueuedConnection,
                                      Q_ARG(QString, "Error"),
                                      Q_ARG(QString, ee->message),
                                      Q_ARG(size_t, QMessageBox::Critical));
            break;
        }
    }
}

ICommandTab* DisassemblerView::showListing()
{
    auto* listingtab = new ListingTab(m_disassembler);
    m_disassemblertabs->insertTab(0, listingtab, listingtab->windowTitle());
    return listingtab;
}

void DisassemblerView::tab(QWidget* w, int index)
{
    if(index != -1) m_disassemblertabs->insertTab(index, w, w->windowTitle());
    else m_disassemblertabs->addTab(w, w->windowTitle());
}

void DisassemblerView::tabify(QDockWidget* first, QDockWidget* second)
{
    DisassemblerHooks::instance()->mainWindow()->tabifyDockWidget(first, second);
}

void DisassemblerView::dock(QWidget* w, Qt::DockWidgetArea area)
{
    QDockWidget* dw = dynamic_cast<QDockWidget*>(w);

    if(!dw)
    {
        dw = new QDockWidget(this);
        w->setParent(dw); // Take ownership
        dw->setWindowTitle(w->windowTitle());
        dw->setWidget(w);
    }

    m_docks.insert(dw);
    DisassemblerHooks::instance()->mainWindow()->addDockWidget(area, dw); // Takes Ownership
}

void DisassemblerView::undock(QDockWidget* dw)
{
    m_docks.remove(dw);
    DisassemblerHooks::instance()->mainWindow()->removeDockWidget(dw);
    dw->deleteLater();
}
