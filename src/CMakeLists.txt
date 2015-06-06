
#include(_INCLUDES)

set(langtool_pdf_SRCS main.cpp)

add_executable(langtool_pdf ${langtool_pdf_SRCS})

target_link_libraries(langtool_pdf poppler-glib ${GLIB2_LIBRARIES})

