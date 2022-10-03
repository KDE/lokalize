# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Andreas Cord-Landwehr <cordlandwehr@kde.org>

include_directories(
    ${CMAKE_SOURCE_DIR}/src/
)

ecm_add_test(
    test_gettextheaderparser.cpp
    test_gettextheaderparser.h
    ${CMAKE_SOURCE_DIR}/src/catalog/gettextheaderparser.cpp
    LINK_LIBRARIES Qt::Core Qt::Test liblokalize
    TEST_NAME test_gettextheaderparser
)
