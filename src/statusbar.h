/*
  SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
  SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>
  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>
  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "lokalizetabpagebase.h"

#include <QLabel>
#include <QStatusBar>

/**
 * Class which handles the status bar: use a single instance of the status
 * bar, and dynamically switch the tab page that is connected to it
 * depending on the tab page currently visible. Signals and slots allow
 * the tab page to communicate with the status bar.
 * */
class LokalizeStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    LokalizeStatusBar(QWidget *parent);
    ~LokalizeStatusBar() override;

    // Set and clear sections in the status bar. These are slots.
    void setCurrentIndex(const int currentIndex);
    void clearCurrentIndex();
    void setTotalCount(const int totalCount);
    void clearTotalCount();
    void setFuzzyNotReadyCount(const int fuzzyNotReadyCount, const int totalCount);
    void clearFuzzyNotReadyCount();
    void setUntranslatedCount(const int untranslatedCount, const int totalCount);
    void clearUntranslatedCount();
    void setReadyCount(const QString text);
    void clearTranslationStatus();
    void clear();
    // Connect a tab page to the status bar and
    // save connections for disconnection later.
    void connectSignals(LokalizeTabPageBase *tab);
    void connectSignals(LokalizeTabPageBaseNoQMainWindow *tab);
    // Disconnect the currently visible tab: use the
    // saved connections to perform disconnections.
    void disconnectSignals();

    // The connections currently in use between tab page
    // signals and the status bar's slots.
    QMetaObject::Connection m_connectionCurrent;
    QMetaObject::Connection m_connectionTotal;
    QMetaObject::Connection m_connectionFuzzyNotReady;
    QMetaObject::Connection m_connectionUntranslated;
    QMetaObject::Connection m_connectionTranslationStatus;

private:
    // Variables used to create the QLabels in the status bar.
    int m_currentIndex;
    int m_totalCount;
    int m_fuzzyNotReadyCount;
    int m_untranslatedCount;
    QString m_translationStatusString;
    // QLabels in the status bar.
    QLabel *m_currentLabel;
    QLabel *m_totalLabel;
    QLabel *m_fuzzyNotReadyLabel;
    QLabel *m_untranslatedLabel;
    QLabel *m_translationStatusLabel;
};

#endif
