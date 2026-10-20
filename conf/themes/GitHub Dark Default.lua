--
-- GitHub Dark Default: the default dark theme of GitHub, with the colors of
-- its Primer design system.
--
-- See Dark.lua for a description of the keys. The 'ui' table colors the QML
-- tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  -- inverted for a light on dark theme
  light            = '#0D1117',
  midlight         = '#161B22',
  middark          = '#21262D',
  dark             = '#30363D',
  shadow           = '#010409'
}

theme['widget']    = {
  text             = { default = '#E6EDF3', disabled = '#6E7681' },
  bright_text      = '#FFFFFF',
  background       = '#0D1117',
  alternate        = '#161B22',
  highlight        = { active = '#1E4273', inactive = '#21262D' },
  highlighted_text = { active = '#FFFFFF', inactive = '#E6EDF3' },
}

theme['window']    = {
  text             = '#E6EDF3',
  background       = '#161B22'
}

theme['button']    = {
  text             = { default = '#E6EDF3', inactive = '#7D8590', disabled = '#6E7681' },
  background       = { default = '#21262D', checked = '#1F6FEB', pressed = '#30363D' }
}

theme['commits']   = {
  text             = '#C9D1D9',
  bright_text      = '#F0F6FC',
  background       = '#0D1117',
  alternate        = '#11161D',
  highlight        = { active = '#183051', inactive = '#161B22' },
  highlighted_text = { active = '#F0F6FC', inactive = '#E6EDF3' },
  highlighted_bright_text = { active = '#FFFFFF', inactive = '#F0F6FC' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#FFFFFF',
    selected       = '#0D1117'
  },
  background       = {
    normal         = '#30363D',
    selected       = '#E6EDF3',
    conflicted     = '#8957E5',
    head           = '#1F6FEB',
    notification   = '#DA3633',
    modified       = '#9E6A03',
    added          = '#238636',
    deleted        = '#DA3633',
    untracked      = '#1B7C83',
    renamed        = '#1F6FEB'
  }
}

theme['blame'] = {
  cold             = '#121D2F',
  hot              = '#3A1D22'
}

theme['graph']     = {
  edge1            = '#58A6FF',
  edge2            = '#3FB950',
  edge3            = '#D29922',
  edge4            = '#A371F7',
  edge5            = '#F85149',
  edge6            = '#DB6D28',
  edge7            = '#DB61A2',
  edge8            = '#56D4DD',
  edge9            = '#79C0FF',
  edge10           = '#7EE787',
  edge11           = '#E3B341',
  edge12           = '#D2A8FF',
  edge13           = '#FFA198',
  edge14           = '#FFA657',
  edge15           = '#FF9BCE'
}

theme['checkbox']  = {
  text             = '#E6EDF3',
  fill             = '#21262D',
  outline          = '#30363D'
}

theme['commiteditor'] = {
  spellerror       = '#F85149',
  spellignore      = '#E6EDF3',
  lengthwarning    = '#272215'
}

theme['diff']      = {
  addition         = '#12261E',
  deletion         = '#25171C',
  plus             = '#3FB950',
  minus            = '#F85149',
  ours             = '#13233A',
  theirs           = '#241F39',
  word_addition    = '#1A4A29',
  word_deletion    = '#6B2B2B',
  note             = '#E6EDF3',
  warning          = '#D29922',
  error            = '#5C2A2E'
}

theme['notice']    = {
  background       = '#272215',
  foreground       = '#E3B341'
}

theme['link']      = {
  link             = '#58A6FF',
  link_visited     = '#A371F7'
}

theme['menubar']   = {
  text             = '#E6EDF3',
  background       = '#010409'
}

theme['tabbar']   = {
  text             = '#7D8590',
  base             = '#010409',
  selected         = '#0D1117',
}

theme['comment']   = {
  background       = '#161B22',
  body             = '#C9D1D9',
  author           = '#58A6FF',
  timestamp        = '#7D8590'
}

theme['star']      = {
  fill             = '#E3B341'
}

theme['titlebar']  = {
  background       = '#010409'
}

theme['tooltip']   = {
  text             = '#E6EDF3',
  background       = '#30363D'
}

theme['ui']        = {
  dark             = true,
  base             = '#0D1117',
  alternate        = '#11161D',
  panel            = '#010409',
  toolbar          = '#010409',
  sidebar          = '#010409',
  field            = '#0D1117',
  border           = '#30363D',
  text             = '#E6EDF3',
  text_muted       = '#7D8590',
  text_disabled    = '#484F58',
  hover            = '#1C2128',
  pressed          = '#262C36',
  selected         = '#183051',
  selected_text    = '#F0F6FC',
  accent           = '#2F81F7',
  accent_text      = '#FFFFFF',
  badge            = '#DA3633',
  badge_text       = '#FFFFFF',
  ahead            = '#3FB950',
  behind           = '#DB6D28',
  star             = '#E3B341',
  tooltip          = '#30363D',
  tooltip_text     = '#E6EDF3',
  alert            = '#B62324',
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
  background: #30363D;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #484F58;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #0D1117;
  border: 1px solid #30363D;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #1E4273;
}
QLineEdit:focus {
  border: 1px solid #2F81F7;
}

QPushButton {
  background: #21262D;
  color: #E6EDF3;
  border: 1px solid #30363D;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #30363D;
}
QPushButton:pressed {
  background: #3A414A;
}
QPushButton:default {
  background: #238636;
  border-color: #238636;
  color: #FFFFFF;
}
QPushButton:default:hover {
  background: #2EA043;
}
QPushButton:disabled {
  background: #161B22;
  border-color: #21262D;
  color: #484F58;
}

QMenu {
  background: #161B22;
  border: 1px solid #30363D;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #1F6FEB;
  color: #FFFFFF;
}
QMenu::item:disabled {
  color: #484F58;
}
QMenu::separator {
  height: 1px;
  background: #30363D;
  margin: 4px 8px;
}

QToolTip {
  color: #E6EDF3;
  background: #30363D;
  border: 1px solid #484F58;
  padding: 3px;
}

QHeaderView::section {
  background: #161B22;
  color: #7D8590;
  border: none;
  border-bottom: 1px solid #30363D;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #010409;
}
TabBar::tab:selected {
  color: #FFFFFF;
  border-bottom: 2px solid #F78166;
}
TabBar::tab:hover:!selected {
  background: #161B22;
}

QSplitter::handle {
  background: #30363D;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#FF7B72'
theme.property['color.yellow']       = '#E3B341'
theme.property['color.green']        = '#7EE787'
theme.property['color.teal']         = '#56D4DD'
theme.property['color.purple']       = '#D2A8FF'
theme.property['color.orange']       = '#FFA657'
theme.property['color.blue']         = '#79C0FF'
theme.property['color.black']        = '#0D1117'
theme.property['color.grey']         = '#8B949E'
theme.property['color.white']        = '#E6EDF3'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#C9D1D9,back:#161B22'
theme.property['style.class']        = 'fore:$(color.orange)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.blue)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.white),back:$(color.black)'
theme.property['style.definition']   = 'fore:$(color.purple)'
theme.property['style.embedded']     = '$(style.tag),back:#161B22'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.purple)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#21262D,back:#21262D'
theme.property['style.keyword']      = 'fore:$(color.red)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#6E7681,back:#0D1117'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.blue)'
theme.property['style.operator']     = 'fore:$(color.red)'
theme.property['style.preprocessor'] = 'fore:$(color.red)'
theme.property['style.regex']        = 'fore:#A5D6FF'
theme.property['style.string']       = 'fore:#A5D6FF'
theme.property['style.tag']          = 'fore:$(color.green)'
theme.property['style.type']         = 'fore:$(color.orange)'
theme.property['style.variable']     = 'fore:$(color.orange)'
theme.property['style.whitespace']   = '$(style.nothing)'
