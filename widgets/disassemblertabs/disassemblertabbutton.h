#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include "../hooks/idisassemblercommand.h"

struct RDEventArgs;

class DisassemblerTabButton : public QWidget
{
    Q_OBJECT

    public:
        explicit DisassemblerTabButton(const RDDisassemblerPtr& disassembler, QWidget* widget, QTabWidget* tabwidget, QWidget *parent = nullptr);
        virtual ~DisassemblerTabButton();

    private slots:
        void closeTab();

    private:
        void onCursorStackChanged(const RDEventArgs* e);
        QPushButton* createButton(const QIcon& icon);
        void customizeBehavior();

    private:
        RDDisassemblerPtr m_disassembler;
        QTabWidget* m_tabwidget;
        QWidget* m_widget;
};

