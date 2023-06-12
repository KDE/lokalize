/*
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "webqueryview.h"

#include "lokalize_debug.h"

#include "project.h"
#include "catalog.h"
#include "flowlayout.h"

#include <KLocalizedString>
#include <QAction>

#include "ui_querycontrol.h"

#include "webquerycontroller.h"

#include <QDragEnterEvent>
#include <QTime>
#include <QSplitter>
#include <QTextBrowser>
#include <QTimer>

WebQueryView::WebQueryView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions)
    : QDockWidget(i18n("Web Queries"), parent)
    , m_catalog(catalog)
    , m_splitter(new QSplitter(this))
    , m_browser(new QTextBrowser(m_splitter))
    , ui_queryControl(new Ui_QueryControl)
    , m_actions(actions)

{
    setObjectName(QStringLiteral("WebQueryView"));
    setWidget(m_splitter);

    hide();

    m_browser->viewport()->setBackgroundRole(QPalette::Window);

    m_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QWidget* w = new QWidget(m_splitter);
    ui_queryControl->setupUi(w);

    QTimer::singleShot(0, this, &WebQueryView::initLater);
}

WebQueryView::~WebQueryView()
{
    delete ui_queryControl;
//     delete m_flowLayout;
}

void WebQueryView::initLater()
{
    m_browser->setToolTip(i18nc("@info:tooltip", "Double-click any word to insert it into translation"));


    int i = m_actions.size();
    while (--i >= 0) {
        connect(m_actions.at(i), &QAction::triggered, this, [this, i] { slotUseSuggestion(i); });
    }
    connect(m_browser, &QTextBrowser::selectionChanged, this, &WebQueryView::slotSelectionChanged);

}

void WebQueryView::slotSelectionChanged()
{
    //NOTE works fine only for dbl-click word selection
    //(actually, quick word insertion is exactly the purpose of this slot:)
    QString sel(m_browser->textCursor().selectedText());
    if (!sel.isEmpty()) {
        Q_EMIT textInsertRequested(sel);
    }
}

//TODO text may be dragged
// void WebQueryView::dragEnterEvent(QDragEnterEvent* event)
// {
//     /*    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
//         {
//             //qCWarning(LOKALIZE_LOG) << " " <<;
//             event->acceptProposedAction();
//         };*/
// }
//
// void WebQueryView::dropEvent(QDropEvent *event)
// {
//     /*    Q_EMIT mergeOpenRequested(event->mimeData()->urls().first());
//         event->acceptProposedAction();*/
// }

void WebQueryView::slotNewEntryDisplayed([[maybe_unused]] const DocPosition& pos)
{
    //m_flowLayout->clearWebQueryResult();
    m_browser->clear();
    m_suggestions.clear();
}

void WebQueryView::slotUseSuggestion(int i)
{
    if (i >= m_suggestions.size())
        return;
    Q_EMIT textInsertRequested(m_suggestions.at(i));
}


void WebQueryView::addWebQueryResult(const QString& name, const QString& str)
{
    QString html(str);
    html.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    html.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    html.append(QLatin1String("<br><br>"));
    html.prepend(QLatin1String("[%2] /%1/ ").arg(name).arg(
                     (m_suggestions.size() < m_actions.size()) ?
                     m_actions.at(m_suggestions.size())->shortcut().toString() :
                     QStringLiteral(" - ")));

    m_browser->insertHtml(html);
    //m_flowLayout->addWebQueryResult(str);

    m_suggestions.append(str);
}

