#include "dockidentifiers.h"
#include <QRegularExpression>

std::unordered_map<QWidget*, size_t> DockIdentifiers::m_lockedids;
std::unordered_multimap<QString, size_t> DockIdentifiers::m_freeids;
std::unordered_map<QString, size_t> DockIdentifiers::m_ids;

QString DockIdentifiers::getId(QWidget* w)
{
    auto title = w->windowTitle();
    if(title.isEmpty()) return QString();

    QObject::connect(w, &QWidget::destroyed, &DockIdentifiers::freeId);

    auto it = m_freeids.find(title);
    size_t ididx;

    if(it != m_freeids.end())
    {
        ididx = it->second;
        m_freeids.erase(it);
    }
    else
        ididx = m_ids[title]++;

    QString id = DockIdentifiers::makeId(title, ididx);
    m_lockedids[w] = ididx;
    return id;
}

void DockIdentifiers::freeId(QObject* obj)
{
    QWidget* w = dynamic_cast<QWidget*>(obj);
    if(!w) return;

    auto it = m_lockedids.find(w);
    if(it == m_lockedids.end()) return;

    m_freeids.insert({w->windowTitle(), it->second});
    m_lockedids.erase(it);
}

QString DockIdentifiers::makeId(const QString& id, size_t idx) { return idx ? QString("%1-%2").arg(id).arg(idx) : id; }
