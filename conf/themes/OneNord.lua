--
-- OneNord: the Nord palette with One Dark syntax colors, like the OneNord
-- theme of Visual Studio Code.
--
-- See Dark.lua for a description of the keys. The 'ui' table colors the QML
-- tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  -- inverted for a light on dark theme
  light            = '#2E3440',
  midlight         = '#323846',
  middark          = '#3B4252',
  dark             = '#434C5E',
  shadow           = '#242933'
}

theme['widget']    = {
  text             = { default = '#C8D0E0', disabled = '#5B6577' },
  bright_text      = '#ECEFF4',
  background       = '#2E3440',
  alternate        = '#323846',
  highlight        = { active = '#4C566A', inactive = '#3B4252' },
  highlighted_text = { active = '#ECEFF4', inactive = '#C8D0E0' },
}

theme['window']    = {
  text             = '#C8D0E0',
  background       = '#2E3440'
}

theme['button']    = {
  text             = { default = '#C8D0E0', inactive = '#8A93A5', disabled = '#5B6577' },
  background       = { default = '#3B4252', checked = '#88C0D0', pressed = '#434C5E' }
}

theme['commits']   = {
  text             = '#AAB3C5',
  bright_text      = '#E5E9F0',
  background       = '#2E3440',
  alternate        = '#313744',
  highlight        = { active = '#434C5E', inactive = '#3B4252' },
  highlighted_text = { active = '#E5E9F0', inactive = '#C8D0E0' },
  highlighted_bright_text = { active = '#ECEFF4', inactive = '#E5E9F0' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#ECEFF4',
    selected       = '#2E3440'
  },
  background       = {
    normal         = '#4C566A',
    selected       = '#E5E9F0',
    conflicted     = '#B48EAD',
    head           = '#5E81AC',
    notification   = '#BF616A',
    modified       = '#7A6A3F',
    added          = '#4E6B44',
    deleted        = '#8C4A51',
    untracked      = '#3E6B73',
    renamed        = '#45618A'
  }
}

theme['blame'] = {
  cold             = '#323A4A',
  hot              = '#4A3540'
}

theme['graph']     = {
  edge1            = '#88C0D0',
  edge2            = '#81A1C1',
  edge3            = '#B988B0',
  edge4            = '#E85B7A',
  edge5            = '#D57780',
  edge6            = '#D08F70',
  edge7            = '#EBCB8B',
  edge8            = '#A3BE8C',
  edge9            = '#8FBCBB',
  edge10           = '#5E81AC',
  edge11           = '#B48EAD',
  edge12           = '#DE878F',
  edge13           = '#6FB3B8',
  edge14           = '#C8A26A',
  edge15           = '#9FC5E8'
}

theme['checkbox']  = {
  text             = '#C8D0E0',
  fill             = '#3B4252',
  outline          = '#4C566A'
}

theme['commiteditor'] = {
  spellerror       = '#D57780',
  spellignore      = '#C8D0E0',
  lengthwarning    = '#4A4436'
}

theme['diff']      = {
  addition         = '#3A4A42',
  deletion         = '#4A3A42',
  plus             = '#A3BE8C',
  minus            = '#BF616A',
  ours             = '#34405A',
  theirs           = '#463A52',
  word_addition    = '#4E6650',
  word_deletion    = '#6E4650',
  note             = '#C8D0E0',
  warning          = '#EBCB8B',
  error            = '#6B454C'
}

theme['notice']    = {
  background       = '#4A4436',
  foreground       = '#EBCB8B'
}

theme['link']      = {
  link             = '#88C0D0',
  link_visited     = '#B48EAD'
}

theme['menubar']   = {
  text             = '#C8D0E0',
  background       = '#2E3440'
}

theme['tabbar']   = {
  text             = '#AAB3C5',
  base             = '#2B303B',
  selected         = '#2E3440',
}

theme['comment']   = {
  background       = '#323846',
  body             = '#AAB3C5',
  author           = '#88C0D0',
  timestamp        = '#8A93A5'
}

theme['star']      = {
  fill             = '#EBCB8B'
}

theme['titlebar']  = {
  background       = '#2E3440'
}

theme['tooltip']   = {
  text             = '#C8D0E0',
  background       = '#3B4252'
}

theme['ui']        = {
  dark             = true,
  base             = '#2E3440',
  alternate        = '#313744',
  panel            = '#2E3440',
  toolbar          = '#2E3440',
  sidebar          = '#2E3440',
  field            = '#3B4252',
  border           = '#3B4252',
  text             = '#C8D0E0',
  text_muted       = '#8A93A5',
  text_disabled    = '#5B6577',
  hover            = '#353B45',
  pressed          = '#3F4758',
  selected         = '#434C5E',
  selected_text    = '#ECEFF4',
  accent           = '#88C0D0',
  accent_text      = '#2E3440',
  badge            = '#BF616A',
  badge_text       = '#ECEFF4',
  ahead            = '#A3BE8C',
  behind           = '#D08F70',
  star             = '#EBCB8B',
  tooltip          = '#3B4252',
  tooltip_text     = '#C8D0E0',
  alert            = '#9E3F49',
  alert_text       = '#ECEFF4'
}

theme['stylesheet'] = [[
QScrollBar:vertical {
  background: transparent;
  width: 10px;
  margin: 0px;
}
QScrollBar:horizontal {
  background: transparent;
  height: 10px;
  margin: 0px;
}
QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
  background: #434C5E;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #4C566A;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #3B4252;
  border: 1px solid #434C5E;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #4C566A;
}
QLineEdit:focus {
  border: 1px solid #88C0D0;
}

QPushButton {
  background: #3B4252;
  color: #C8D0E0;
  border: 1px solid #434C5E;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #434C5E;
}
QPushButton:pressed {
  background: #4C566A;
}
QPushButton:default {
  background: #5E81AC;
  border-color: #5E81AC;
  color: #ECEFF4;
}
QPushButton:default:hover {
  background: #6A8DB8;
}
QPushButton:disabled {
  background: #323846;
  border-color: #3B4252;
  color: #5B6577;
}

QMenu {
  background: #3B4252;
  border: 1px solid #434C5E;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #4C566A;
  color: #ECEFF4;
}
QMenu::item:disabled {
  color: #5B6577;
}
QMenu::separator {
  height: 1px;
  background: #434C5E;
  margin: 4px 8px;
}

QToolTip {
  color: #C8D0E0;
  background: #3B4252;
  border: 1px solid #4C566A;
  padding: 3px;
}

QHeaderView::section {
  background: #2E3440;
  color: #8A93A5;
  border: none;
  border-bottom: 1px solid #3B4252;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #2B303B;
}
TabBar::tab:selected {
  color: #ECEFF4;
  border-bottom: 2px solid #88C0D0;
}
TabBar::tab:hover:!selected {
  background: #353B45;
}

QSplitter::handle {
  background: #3B4252;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#D57780'
theme.property['color.yellow']       = '#EBCB8B'
theme.property['color.green']        = '#A3BE8C'
theme.property['color.teal']         = '#8FBCBB'
theme.property['color.purple']       = '#B988B0'
theme.property['color.orange']       = '#D08F70'
theme.property['color.blue']         = '#88C0D0'
theme.property['color.black']        = '#2E3440'
theme.property['color.grey']         = '#6C7A96'
theme.property['color.white']        = '#C8D0E0'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#AAB3C5,back:#3B4252'
theme.property['style.class']        = 'fore:$(color.yellow)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.orange)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:#C8D0E0,back:#2E3440'
theme.property['style.definition']   = 'fore:$(color.blue)'
theme.property['style.embedded']     = '$(style.tag),back:#3B4252'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.blue)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#3B4252,back:#3B4252'
theme.property['style.keyword']      = 'fore:$(color.purple)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#646A76,back:#2E3440'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.orange)'
theme.property['style.operator']     = 'fore:$(color.teal)'
theme.property['style.preprocessor'] = 'fore:$(color.purple)'
theme.property['style.regex']        = 'fore:$(color.green)'
theme.property['style.string']       = 'fore:$(color.green)'
theme.property['style.tag']          = 'fore:$(color.red)'
theme.property['style.type']         = 'fore:$(color.yellow)'
theme.property['style.variable']     = 'fore:$(color.red)'
theme.property['style.whitespace']   = '$(style.nothing)'
