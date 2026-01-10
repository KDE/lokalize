/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
  SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>
  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "statusbar.h"
#include "projectbase.h"

#include <KLocalizedString>

#include <QLabel>
#include <qnamespace.h>

LokalizeStatusBar::LokalizeStatusBar(QWidget *parent)
    : QStatusBar(parent)
    , m_currentIndex(-1)
    , m_totalCount(-1)
    , m_fuzzyNotReadyCount(-1)
    , m_untranslatedCount(-1)
    , m_translationStatusString(QString())
    , m_currentLabel(new QLabel(this))
    , m_totalLabel(new QLabel(this))
    , m_fuzzyNotReadyLabel(new QLabel(this))
    , m_untranslatedLabel(new QLabel(this))
    , m_translationStatusLabel(new QLabel(this))
{
    m_currentLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
    m_totalLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
    m_fuzzyNotReadyLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
    m_untranslatedLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
    m_translationStatusLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

    addWidget(m_currentLabel, 1);
    addWidget(m_totalLabel, 1);
    addWidget(m_fuzzyNotReadyLabel, 1);
    addWidget(m_untranslatedLabel, 1);
    addWidget(m_translationStatusLabel, 1);
}

LokalizeStatusBar::~LokalizeStatusBar()
{
}

void LokalizeStatusBar::setCurrentIndex(const int currentIndex)
{
    if (currentIndex != m_currentIndex) {
        m_currentIndex = currentIndex;
        m_currentLabel->setText(i18nc("@info:status", "Current: %1", currentIndex));
    }
}

void LokalizeStatusBar::clearCurrentIndex()
{
    m_currentIndex = 0;
    m_currentLabel->clear();
}

void LokalizeStatusBar::setTotalCount(const int totalCount)
{
    if (totalCount != m_totalCount) {
        m_totalCount = totalCount;
        m_totalLabel->setText(i18nc("@info:status message entries", "Total: %1", totalCount));
    }
}

void LokalizeStatusBar::clearTotalCount()
{
    m_totalCount = -1;
    m_totalLabel->clear();
}

void LokalizeStatusBar::setFuzzyNotReadyCount(const int fuzzyNotReadyCount, const int totalCount)
{
    QString text = i18nc("@info:status message entries\n'fuzzy' in gettext terminology", "Not ready: %1", fuzzyNotReadyCount);
    if (fuzzyNotReadyCount && totalCount)
        text += i18nc("percentages in statusbar", " (%1%)", int(100.0 * fuzzyNotReadyCount / totalCount));
    m_fuzzyNotReadyCount = fuzzyNotReadyCount;
    m_fuzzyNotReadyLabel->setText(text);
}

void LokalizeStatusBar::clearFuzzyNotReadyCount()
{
    m_fuzzyNotReadyCount = -1;
    m_fuzzyNotReadyLabel->clear();
}

void LokalizeStatusBar::setUntranslatedCount(const int untranslatedCount, const int totalCount)
{
    QString text = i18nc("@info:status message entries", "Untranslated: %1", untranslatedCount);
    if (untranslatedCount && totalCount)
        text += i18nc("percentages in statusbar", " (%1%)", int(100.0 * untranslatedCount / totalCount));
    m_untranslatedCount = untranslatedCount;
    m_untranslatedLabel->setText(text);
}

void LokalizeStatusBar::clearUntranslatedCount()
{
    m_untranslatedCount = -1;
    m_untranslatedLabel->clear();
}

void LokalizeStatusBar::setReadyCount(const QString text)
{
    if (text != m_translationStatusString) {
        m_translationStatusString = text;
        m_translationStatusLabel->setText(text);
    }
}

void LokalizeStatusBar::clearTranslationStatus()
{
    m_translationStatusLabel->clear();
    m_translationStatusString.clear();
}

void LokalizeStatusBar::clear()
{
    clearCurrentIndex();
    clearTotalCount();
    clearFuzzyNotReadyCount();
    clearUntranslatedCount();
    clearTranslationStatus();
}

void LokalizeStatusBar::connectSignals(LokalizeTabPageBaseNoQMainWindow *tab)
{
    m_connectionCurrent = connect(tab, &LokalizeTabPageBaseNoQMainWindow::signalStatusBarCurrent, this, &LokalizeStatusBar::setCurrentIndex);
    m_connectionTotal = connect(tab, &LokalizeTabPageBaseNoQMainWindow::signalStatusBarTotal, this, &LokalizeStatusBar::setTotalCount);
    m_connectionFuzzyNotReady = connect(tab, &LokalizeTabPageBaseNoQMainWindow::signalStatusBarFuzzyNotReady, this, &LokalizeStatusBar::setFuzzyNotReadyCount);
    m_connectionUntranslated = connect(tab, &LokalizeTabPageBaseNoQMainWindow::signalStatusBarUntranslated, this, &LokalizeStatusBar::setUntranslatedCount);
    m_connectionTranslationStatus = connect(tab, &LokalizeTabPageBaseNoQMainWindow::signalStatusBarTranslationStatus, this, &LokalizeStatusBar::setReadyCount);
}

void LokalizeStatusBar::connectSignals(LokalizeTabPageBase *tab)
{
    m_connectionCurrent = connect(tab, &LokalizeTabPageBase::signalStatusBarCurrent, this, &LokalizeStatusBar::setCurrentIndex);
    m_connectionTotal = connect(tab, &LokalizeTabPageBase::signalStatusBarTotal, this, &LokalizeStatusBar::setTotalCount);
    m_connectionFuzzyNotReady = connect(tab, &LokalizeTabPageBase::signalStatusBarFuzzyNotReady, this, &LokalizeStatusBar::setFuzzyNotReadyCount);
    m_connectionUntranslated = connect(tab, &LokalizeTabPageBase::signalStatusBarUntranslated, this, &LokalizeStatusBar::setUntranslatedCount);
    m_connectionTranslationStatus = connect(tab, &LokalizeTabPageBase::signalStatusBarTranslationStatus, this, &LokalizeStatusBar::setReadyCount);
}

void LokalizeStatusBar::disconnectSignals()
{
    disconnect(m_connectionCurrent);
    disconnect(m_connectionTotal);
    disconnect(m_connectionFuzzyNotReady);
    disconnect(m_connectionUntranslated);
    disconnect(m_connectionTranslationStatus);
}

#include "moc_statusbar.cpp"
