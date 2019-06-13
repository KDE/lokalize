find_package(Qt5Test CONFIG REQUIRED)
include(ECMAddTests)

ecm_add_test(projectmodeltest.cpp LINK_LIBRARIES liblokalize Qt5::Test)
