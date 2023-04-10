find_package(Qt${QT_MAJOR_VERSION}Test CONFIG REQUIRED)
include(ECMAddTests)

ecm_add_test(projectmodeltest.cpp LINK_LIBRARIES liblokalize Qt::Test)
ecm_add_test(gettextheadertest.cpp LINK_LIBRARIES liblokalize Qt::Test)
ecm_add_test(filemetadatatest.cpp LINK_LIBRARIES liblokalize Qt::Test)
ecm_add_test(glossarytest.cpp LINK_LIBRARIES liblokalize Qt::Test)
ecm_add_test(tmjobstest.cpp LINK_LIBRARIES liblokalize Qt::Test)
