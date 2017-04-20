
#include(_INCLUDES)

set(langtool_pdf_SRCS main.cpp)

add_executable(langtool_pdf ${langtool_pdf_SRCS})

#poppler-qt4 ${QT_QTCORE_LIBRARY}
#target_link_libraries(langtool_pdf okularcore boost_regex ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})
target_link_libraries(langtool_pdf Okular::Core ${Qt5Widgets_LIBRARIES})

