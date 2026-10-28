--
-- Neon: neon pink, cyan and green on a deep night blue, like the Neon
-- City theme of Visual Studio Code.
--
-- See Dark.lua for a description of the keys. The 'ui' table colors the QML
-- tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  -- inverted for a light on dark theme
  light            = '#0C0A20',
  midlight         = '#100D2B',
  middark          = '#1E1A3A',
  dark             = '#2D2844',
  shadow           = '#050410'
}

theme['widget']    = {
  text             = { default = '#F2F3F7', disabled = '#484F7D' },
  bright_text      = '#FFFFFF',
  background       = '#0C0A20',
  alternate        = '#0F0C26',
  highlight        = { active = '#484F7D', inactive = '#131033' },
  highlighted_text = { active = '#FFFFFF', inactive = '#F2F3F7' },
}

theme['window']    = {
  text             = '#F2F3F7',
  background       = '#090819'
}

theme['button']    = {
  text             = { default = '#F2F3F7', inactive = '#9199D9', disabled = '#484F7D' },
  background       = { default = '#100D2B', checked = '#1EA8FC', pressed = '#2D2844' }
}

theme['commits']   = {
  text             = '#C9CCE6',
  bright_text      = '#FFFFFF',
  background       = '#0C0A20',
  alternate        = '#0F0C26',
  highlight        = { active = '#7B3A9A', inactive = '#131033' },
  highlighted_text = { active = '#FFFFFF', inactive = '#F2F3F7' },
  highlighted_bright_text = { active = '#FFFFFF', inactive = '#FFFFFF' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#FFFFFF',
    selected       = '#090819'
  },
  background       = {
    normal         = '#2D2844',
    selected       = '#F2F3F7',
    conflicted     = '#8A45A3',
    head           = '#1A6FA8',
    notification   = '#B0105E',
    modified       = '#8A6E00',
    added          = '#3D7A1F',
    deleted        = '#B0105E',
    untracked      = '#1A6FA8',
    renamed        = '#6E2F8A'
  }
}

theme['blame'] = {
  cold             = '#141233',
  hot              = '#3A0F2E'
}

theme['graph']     = {
  edge1            = '#42C6FF',
  edge2            = '#FF2AFC',
  edge3            = '#83FF52',
  edge4            = '#FFD400',
  edge5            = '#FF2E97',
  edge6            = '#1EA8FC',
  edge7            = '#D86BFF',
  edge8            = '#7AFFE2',
  edge9            = '#FF9B50',
  edge10           = '#3DD8FF',
  edge11           = '#E3A3F7',
  edge12           = '#DCFF7A',
  edge13           = '#FB4293',
  edge14           = '#A8FFDB',
  edge15           = '#B267E6'
}

theme['checkbox']  = {
  text             = '#F2F3F7',
  fill             = '#100D2B',
  outline          = '#763B8C'
}

theme['commiteditor'] = {
  spellerror       = '#FF2E97',
  spellignore      = '#F2F3F7',
  lengthwarning    = '#3A3010'
}

theme['diff']      = {
  addition         = '#293122',
  deletion         = '#3D081A',
  plus             = '#83FF52',
  minus            = '#FF2E97',
  ours             = '#17304D',
  theirs           = '#3D104C',
  word_addition    = '#465825',
  word_deletion    = '#6D0613',
  note             = '#F2F3F7',
  warning          = '#FF9B50',
  error            = '#5A0A2E'
}

theme['notice']    = {
  background       = '#3A3010',
  foreground       = '#FFD400'
}

theme['link']      = {
  link             = '#42C6FF',
  link_visited     = '#D86BFF'
}

theme['menubar']   = {
  text             = '#F2F3F7',
  background       = '#090819'
}

theme['tabbar']   = {
  text             = '#9199D9',
  base             = '#090819',
  selected         = '#0C0A20',
}

theme['comment']   = {
  background       = '#100D2B',
  body             = '#E4D2E6',
  author           = '#42C6FF',
  timestamp        = '#9199D9'
}

theme['star']      = {
  fill             = '#FFD400'
}

theme['titlebar']  = {
  background       = '#090819'
}

theme['tooltip']   = {
  text             = '#F2F3F7',
  background       = '#100D2B'
}

theme['ui']        = {
  dark             = true,
  base             = '#0C0A20',
  alternate        = '#0F0C26',
  panel            = '#090819',
  toolbar          = '#090819',
  sidebar          = '#090819',
  field            = '#0C0A20',
  border           = '#2D2844',
  text             = '#F2F3F7',
  text_muted       = '#9199D9',
  text_disabled    = '#484F7D',
  hover            = '#231B44',
  pressed          = '#3A2766',
  selected         = '#7B3A9A',
  selected_text    = '#FFFFFF',
  accent           = '#1EA8FC',
  accent_text      = '#090819',
  badge            = '#FF2E97',
  badge_text       = '#090819',
  ahead            = '#83FF52',
  behind           = '#FF9B50',
  star             = '#FFD400',
  tooltip          = '#100D2B',
  tooltip_text     = '#F2F3F7',
  alert            = '#B0105E',
  alert_text       = '#FFFFFF'
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
  background: #353759;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #696EA2;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #0C0A20;
  border: 1px solid #763B8C;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #484F7D;
}
QLineEdit:focus {
  border: 1px solid #1EA8FC;
}

QPushButton {
  background: #100D2B;
  color: #F2F3F7;
  border: 1px solid #2D2844;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #1E1A3A;
}
QPushButton:pressed {
  background: #2D2844;
}
QPushButton:default {
  background: #1EA8FC;
  border-color: #1EA8FC;
  color: #090819;
}
QPushButton:default:hover {
  background: #4BBBFD;
}
QPushButton:disabled {
  background: #0C0A20;
  border-color: #1E1A3A;
  color: #484F7D;
}

QMenu {
  background: #100D2B;
  border: 1px solid #2D2844;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #D86BFF;
  color: #090819;
}
QMenu::item:disabled {
  color: #484F7D;
}
QMenu::separator {
  height: 1px;
  background: #2D2844;
  margin: 4px 8px;
}

QToolTip {
  color: #F2F3F7;
  background: #100D2B;
  border: 1px solid #763B8C;
  padding: 3px;
}

QHeaderView::section {
  background: #090819;
  color: #9199D9;
  border: none;
  border-bottom: 1px solid #2D2844;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #090819;
}
TabBar::tab:selected {
  color: #FFFFFF;
  border-bottom: 2px solid #D86BFF;
}
TabBar::tab:hover:!selected {
  background: #100D2B;
}

QSplitter::handle {
  background: #2D2844;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#FB4293'
theme.property['color.yellow']       = '#FFD400'
theme.property['color.green']        = '#83FF52'
theme.property['color.teal']         = '#7AFFE2'
theme.property['color.purple']       = '#E3A3F7'
theme.property['color.orange']       = '#FF9B50'
theme.property['color.blue']         = '#3DD8FF'
theme.property['color.black']        = '#0C0A20'
theme.property['color.grey']         = '#616791'
theme.property['color.white']        = '#F2F3F7'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#E4D2E6,back:#100D2B'
theme.property['style.class']        = 'fore:$(color.teal)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:#E5FCA6'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.white),back:$(color.black)'
theme.property['style.definition']   = 'fore:$(color.blue)'
theme.property['style.embedded']     = '$(style.tag),back:#100D2B'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.blue)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#2D2844,back:#2D2844'
theme.property['style.keyword']      = 'fore:$(color.red)'
theme.property['style.label']        = 'fore:$(color.purple)'
theme.property['style.linenumber']   = 'fore:#484F7D,back:#0C0A20'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:#E5FCA6'
theme.property['style.operator']     = 'fore:#FE8DA5'
theme.property['style.preprocessor'] = 'fore:#F37AB0'
theme.property['style.regex']        = 'fore:$(color.green)'
theme.property['style.string']       = 'fore:$(color.green)'
theme.property['style.tag']          = 'fore:#6ACBD8'
theme.property['style.type']         = 'fore:$(color.teal)'
theme.property['style.variable']     = 'fore:#E4D2E6'
theme.property['style.whitespace']   = '$(style.nothing)'
