add_library(loglib LogEntry.cpp LogModel.cpp)
target_link_libraries(loglib Qt6::Widgets)

set_target_properties(loglib PROPERTIES AUTOMOC ON)
