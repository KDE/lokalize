/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef STATE_H
#define STATE_H

///@see https://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html#state
enum TargetState {
    New,
    NeedsTranslation,
    NeedsL10n,
    NeedsAdaptation,
    Translated,
    NeedsReviewTranslation,
    NeedsReviewL10n,
    NeedsReviewAdaptation,
    Final,
    SignedOff,
    StateCount
};

#endif
