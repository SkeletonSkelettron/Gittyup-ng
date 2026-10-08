add_library(
  dialogs
  AboutDialog.cpp
  AccountDialog.cpp
  AddRemoteDialog.cpp
  AmendDialog.cpp
  CheckoutDialog.cpp
  CloneDialog.cpp
  CommitDialog.cpp
  ConfirmDialog.cpp
  ConfigDialog.cpp
  DeleteBranchDialog.cpp
  DeleteTagDialog.cpp
  ExternalToolsDialog.cpp
  ExternalToolsModel.cpp
  InputDialog.cpp
  MergeDialog.cpp
  NewBranchDialog.cpp
  PluginsDialog.cpp
  PullRequestDialog.cpp
  QmlDialog.cpp
  ReferenceItems.cpp
  RemoteDialog.cpp
  RenameBranchDialog.cpp
  SettingsDialog.cpp
  TagDialog.cpp
  ThemeDialog.cpp
  UpdateSubmodulesDialog.cpp)

target_compile_definitions(
  dialogs PRIVATE GITTYUP_BUILD_DATE="${GITTYUP_BUILD_DATE}"
                  GITTYUP_BUILD_REVISION="${GITTYUP_BUILD_REVISION}")

target_include_directories(dialogs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
  dialogs
  cli
  conf
  cred
  host
  index
  plugins
  Qt6::QuickWidgets
  Qt6::Widgets)

set_target_properties(dialogs PROPERTIES AUTOMOC ON)
