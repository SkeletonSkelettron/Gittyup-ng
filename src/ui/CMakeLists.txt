add_library(
  ui
  ChangedFilesModel.cpp
  CommitList.cpp
  CommandPalette.cpp
  CodeHighlighter.cpp
  CommitMessage.cpp
  DetailView.cpp
  DiffLines.cpp
  DiffModel.cpp
  EditorWindow.cpp
  FileContextMenu.cpp
  FileEditor.cpp
  FileViewModel.cpp
  MergeModel.cpp
  PullRequestList.cpp
  FindController.cpp
  History.cpp
  IgnoreDialog.cpp
  InteractiveRebase.cpp
  HotkeyManager.cpp
  Location.cpp
  LogPanel.cpp
  MainWindow.cpp
  MenuBar.cpp
  RefDrop.cpp
  RefsPanel.cpp
  RepoView.cpp
  RemoteCallbacks.cpp
  SearchField.cpp
  SideBar.cpp
  SpellCheck.cpp
  StateAction.cpp
  SyntaxHighlighter.cpp
  SpellChecker.cpp
  TabStrip.cpp
  TabWidget.cpp
  CommitTemplates.cpp
  TemplateDialog.cpp
  ToolBar.cpp
  TreeModel.cpp
  UndoHistory.cpp
  WelcomePage.cpp
  ConfigKeys.cpp
  qml/CommitGraphItem.cpp
  qml/QmlSupport.cpp
  qml/QmlTheme.cpp)

# QML files and icons for the tool bar and sidebar.
file(
  GLOB QML_ICONS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/icons/*.svg)
file(
  GLOB QML_FILES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml)
qt_add_resources(
  ui
  "qml"
  PREFIX
  "/"
  FILES
  ${QML_FILES}
  ${QML_ICONS})

target_compile_definitions(ui PRIVATE BUILD_DESCRIPTION="${BUILD_DESCRIPTION}")

target_include_directories(ui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
  ui
  PUBLIC
  conf
  cred
  dialogs
  editor
  git
  ${LIBSSH2_LIBRARIES}
  host
  hunspell
  index
  loglib
  plugins
  tools
  update
  watcher
  Qt6::Concurrent
  Qt6::Network
  Qt6::Quick
  Qt6::QuickWidgets
  Qt6::Widgets)

set_target_properties(ui PROPERTIES AUTOMOC ON)
