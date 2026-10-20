--
-- Palenight: the purple and blue Material Palenight palette, like the
-- Palenight Theme of Visual Studio Code.
--
-- See Dark.lua for a description of the keys. The 'ui' table colors the QML
-- tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  -- inverted for a light on dark theme
  light            = '#292D3E',
  midlight         = '#2C3043',
  middark          = '#32374D',
  dark             = '#3A3F58',
  shadow           = '#202331'
}

theme['widget']    = {
  text             = { default = '#BFC7D5', disabled = '#4E5579' },
  bright_text      = '#FFFFFF',
  background       = '#292D3E',
  alternate        = '#2C3043',
  highlight        = { active = '#444267', inactive = '#32374D' },
  highlighted_text = { active = '#FFFFFF', inactive = '#BFC7D5' },
}

theme['window']    = {
  text             = '#BFC7D5',
  background       = '#292D3E'
}

theme['button']    = {
  text             = { default = '#BFC7D5', inactive = '#8A91B4', disabled = '#4E5579' },
  background       = { default = '#32374D', checked = '#7E57C2', pressed = '#3A3F58' }
}

theme['commits']   = {
  text             = '#A6ACCD',
  bright_text      = '#EEFFFF',
  background       = '#292D3E',
  alternate        = '#2C3043',
  highlight        = { active = '#444267', inactive = '#32374D' },
  highlighted_text = { active = '#EEFFFF', inactive = '#BFC7D5' },
  highlighted_bright_text = { active = '#FFFFFF', inactive = '#EEFFFF' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#FFFFFF',
    selected       = '#292D3E'
  },
  background       = {
    normal         = '#3A3F58',
    selected       = '#BFC7D5',
    conflicted     = '#7E57C2',
    head           = '#5C6BC0',
    notification   = '#C3405A',
    modified       = '#8A6D2B',
    added          = '#4E7A34',
    deleted        = '#A33C4B',
    untracked      = '#2F6F78',
    renamed        = '#4C5FA6'
  }
}

theme['blame'] = {
  cold             = '#2D3550',
  hot              = '#4A2F40'
}

theme['graph']     = {
  edge1            = '#82AAFF',
  edge2            = '#C792EA',
  edge3            = '#C3E88D',
  edge4            = '#FFCB6B',
  edge5            = '#F78C6C',
  edge6            = '#F07178',
  edge7            = '#89DDFF',
  edge8            = '#FF5370',
  edge9            = '#80CBC4',
  edge10           = '#7E57C2',
  edge11           = '#B2CCD6',
  edge12           = '#AB47BC',
  edge13           = '#DDFFA7',
  edge14           = '#FFAB70',
  edge15           = '#9CC4FF'
}

theme['checkbox']  = {
  text             = '#BFC7D5',
  fill             = '#32374D',
  outline          = '#4E5579'
}

theme['commiteditor'] = {
  spellerror       = '#FF5370',
  spellignore      = '#BFC7D5',
  lengthwarning    = '#4A4332'
}

theme['diff']      = {
  addition         = '#33453E',
  deletion         = '#4A3040',
  plus             = '#C3E88D',
  minus            = '#FF5370',
  ours             = '#33406A',
  theirs           = '#4A3A66',
  word_addition    = '#47603F',
  word_deletion    = '#6E3A4D',
  note             = '#BFC7D5',
  warning          = '#FFCB6B',
  error            = '#6B3446'
}

theme['notice']    = {
  background       = '#4A4332',
  foreground       = '#FFCB6B'
}

theme['link']      = {
  link             = '#82AAFF',
  link_visited     = '#C792EA'
}

theme['menubar']   = {
  text             = '#BFC7D5',
  background       = '#292D3E'
}

theme['tabbar']   = {
  text             = '#8A91B4',
  base             = '#242837',
  selected         = '#292D3E',
}

theme['comment']   = {
  background       = '#2C3043',
  body             = '#A6ACCD',
  author           = '#82AAFF',
  timestamp        = '#8A91B4'
}

theme['star']      = {
  fill             = '#FFCB6B'
}

theme['titlebar']  = {
  background       = '#292D3E'
}

theme['tooltip']   = {
  text             = '#BFC7D5',
  background       = '#34324A'
}

theme['ui']        = {
  dark             = true,
  base             = '#292D3E',
  alternate        = '#2C3043',
  panel            = '#292D3E',
  toolbar          = '#292D3E',
  sidebar          = '#292D3E',
  field            = '#313850',
  border           = '#32374D',
  text             = '#BFC7D5',
  text_muted       = '#8A91B4',
  text_disabled    = '#4E5579',
  hover            = '#31364A',
  pressed          = '#3A3F58',
  selected         = '#444267',
  selected_text    = '#FFFFFF',
  accent           = '#7E57C2',
  accent_text      = '#FFFFFF',
  badge            = '#FF5370',
  badge_text       = '#292D3E',
  ahead            = '#C3E88D',
  behind           = '#F78C6C',
  star             = '#FFCB6B',
  tooltip          = '#34324A',
  tooltip_text     = '#BFC7D5',
  alert            = '#B3364B',
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
  background: #3A3F58;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #4E5579;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #313850;
  border: 1px solid #3A3F58;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #444267;
}
QLineEdit:focus {
  border: 1px solid #7E57C2;
}

QPushButton {
  background: #32374D;
  color: #BFC7D5;
  border: 1px solid #3A3F58;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #3A3F58;
}
QPushButton:pressed {
  background: #444267;
}
QPushButton:default {
  background: #7E57C2;
  border-color: #7E57C2;
  color: #FFFFFF;
}
QPushButton:default:hover {
  background: #8E67D2;
}
QPushButton:disabled {
  background: #2C3043;
  border-color: #32374D;
  color: #4E5579;
}

QMenu {
  background: #2C3043;
  border: 1px solid #3A3F58;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #7E57C2;
  color: #FFFFFF;
}
QMenu::item:disabled {
  color: #4E5579;
}
QMenu::separator {
  height: 1px;
  background: #3A3F58;
  margin: 4px 8px;
}

QToolTip {
  color: #BFC7D5;
  background: #34324A;
  border: 1px solid #4E5579;
  padding: 3px;
}

QHeaderView::section {
  background: #292D3E;
  color: #8A91B4;
  border: none;
  border-bottom: 1px solid #32374D;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #242837;
}
TabBar::tab:selected {
  color: #FFFFFF;
  border-bottom: 2px solid #80CBC4;
}
TabBar::tab:hover:!selected {
  background: #31364A;
}

QSplitter::handle {
  background: #32374D;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#F07178'
theme.property['color.yellow']       = '#FFCB6B'
theme.property['color.green']        = '#C3E88D'
theme.property['color.teal']         = '#89DDFF'
theme.property['color.purple']       = '#C792EA'
theme.property['color.orange']       = '#F78C6C'
theme.property['color.blue']         = '#82AAFF'
theme.property['color.black']        = '#292D3E'
theme.property['color.grey']         = '#697098'
theme.property['color.white']        = '#BFC7D5'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#A6ACCD,back:#32374D'
theme.property['style.class']        = 'fore:$(color.yellow)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.orange)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.white),back:$(color.black)'
theme.property['style.definition']   = 'fore:$(color.blue)'
theme.property['style.embedded']     = '$(style.tag),back:#32374D'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.blue)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#32374D,back:#32374D'
theme.property['style.keyword']      = 'fore:$(color.purple)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#4C5374,back:#292D3E'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.orange)'
theme.property['style.operator']     = 'fore:$(color.teal)'
theme.property['style.preprocessor'] = 'fore:$(color.purple)'
theme.property['style.regex']        = 'fore:$(color.teal)'
theme.property['style.string']       = 'fore:$(color.green)'
theme.property['style.tag']          = 'fore:$(color.red)'
theme.property['style.type']         = 'fore:$(color.yellow)'
theme.property['style.variable']     = 'fore:$(color.white)'
theme.property['style.whitespace']   = '$(style.nothing)'
