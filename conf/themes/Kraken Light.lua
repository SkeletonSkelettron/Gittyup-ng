--
-- Kraken Light: a flat, light theme with a teal accent and vivid graph lanes.
--
-- See Default.lua for a description of the keys. The 'ui' table colors the
-- QML tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  light            = '#FFFFFF',
  midlight         = '#F6F7F9',
  middark          = '#DADFE6',
  dark             = '#B9C0CA',
  shadow           = '#8E96A3'
}

theme['widget']    = {
  text             = { default = '#1E232B', disabled = '#A4ABB6' },
  bright_text      = '#000000',
  background       = '#FFFFFF',
  alternate        = '#F6F7F9',
  highlight        = { active = '#CFE2FA', inactive = '#E3EAF3' },
  highlighted_text = { active = '#10233F', inactive = '#1E232B' },
}

theme['window']    = {
  text             = '#1E232B',
  background       = '#F1F3F6'
}

theme['button']    = {
  text             = { default = '#1E232B', inactive = '#5E6673', disabled = '#A4ABB6' },
  background       = { default = '#F1F3F6', checked = '#0E9F84', pressed = '#DADFE6' }
}

theme['commits']   = {
  text             = '#3A414C',
  bright_text      = '#10141A',
  background       = '#FFFFFF',
  alternate        = '#F8F9FB',
  highlight        = { active = '#CFE2FA', inactive = '#E3EAF3' },
  highlighted_text = { active = '#10233F', inactive = '#1E232B' },
  highlighted_bright_text = { active = '#10233F', inactive = '#10141A' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#1E232B',
    selected       = '#1E232B'
  },
  background       = {
    normal         = '#DCE4EE',
    selected       = '#FFFFFF',
    conflicted     = '#F0B6F7',
    head           = '#8FE0CE',
    notification   = '#E5484D',
    modified       = '#FFEBC2',
    added          = '#CFF3DA',
    deleted        = '#FFD6D9',
    untracked      = '#CDEFF2',
    renamed        = '#D4E4FB'
  }
}

theme['blame'] = {
  cold             = '#DCE6FF',
  hot              = '#FFD9D9'
}

theme['graph']     = {
  edge1            = '#0B8FB0',
  edge2            = '#0A5FE0',
  edge3            = '#8A1FC4',
  edge4            = '#B8129F',
  edge5            = '#C8105F',
  edge6            = '#C62828',
  edge7            = '#E0561F',
  edge8            = '#C99A06',
  edge9            = '#4CA82A',
  edge10           = '#12A07E',
  edge11           = '#2596BE',
  edge12           = '#7C4DFF',
  edge13           = '#D63F8C',
  edge14           = '#D98E04',
  edge15           = '#2E9E5B'
}

theme['checkbox']  = {
  text             = '#1E232B',
  fill             = '#FFFFFF',
  outline          = '#B9C0CA'
}

theme['commiteditor'] = {
  spellerror       = '#D1242F',
  spellignore      = '#8E96A3',
  lengthwarning    = '#FFF4D6'
}

theme['diff']      = {
  addition         = '#E3F6E8',
  deletion         = '#FCE8EA',
  plus             = '#1A7F37',
  minus            = '#CF222E',
  ours             = '#E0F0FF',
  theirs           = '#F5E6FF',
  word_addition    = '#ABF2BC',
  word_deletion    = '#FFC1C0',
  note             = '#1E232B',
  warning          = '#FFF1C2',
  error            = '#FFCCCC'
}

theme['notice']    = {
  background       = '#FFF3CD',
  foreground       = '#664D03'
}

theme['link']      = {
  link             = '#0969DA',
  link_visited     = '#8250DF'
}

theme['menubar']   = {
  text             = '#1E232B',
  background       = '#F8F9FB'
}

theme['tabbar']   = {
  text             = '#3A414C',
  base             = '#E9ECF0',
  selected         = '#F1F3F6',
}

theme['comment']   = {
  background       = '#F6F7F9',
  body             = '#3A414C',
  author           = '#0969DA',
  timestamp        = '#5E6673'
}

theme['star']      = {
  fill             = '#E5A500'
}

theme['titlebar']  = {
  background       = '#F8F9FB'
}

theme['tooltip']   = {
  text             = '#FFFFFF',
  background       = '#2A2E36'
}

theme['ui']        = {
  dark             = false,
  base             = '#FFFFFF',
  alternate        = '#F8F9FB',
  panel            = '#F1F3F6',
  toolbar          = '#F8F9FB',
  sidebar          = '#F1F3F6',
  field            = '#FFFFFF',
  border           = '#DADFE6',
  text             = '#1E232B',
  text_muted       = '#5E6673',
  text_disabled    = '#AAB1BC',
  hover            = '#E6E9EE',
  pressed          = '#D9DEE5',
  selected         = '#D7E7FB',
  selected_text    = '#10233F',
  accent           = '#0E9F84',
  accent_text      = '#FFFFFF',
  badge            = '#E5484D',
  badge_text       = '#FFFFFF',
  ahead            = '#0E9F84',
  behind           = '#D9720B',
  star             = '#E5A500',
  tooltip          = '#2A2E36',
  tooltip_text     = '#FFFFFF'
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
  background: #C9CFD8;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #A9B1BD;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #FFFFFF;
  border: 1px solid #D0D6DE;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #CFE2FA;
  selection-color: #10233F;
}
QLineEdit:focus {
  border: 1px solid #0E9F84;
}

QPushButton {
  background: #FFFFFF;
  color: #1E232B;
  border: 1px solid #D0D6DE;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #F1F3F6;
}
QPushButton:pressed {
  background: #E3E7EC;
}
QPushButton:default {
  background: #0E9F84;
  border-color: #0E9F84;
  color: #FFFFFF;
}
QPushButton:default:hover {
  background: #0C8E76;
}
QPushButton:disabled {
  background: #F4F5F7;
  border-color: #E3E7EC;
  color: #A4ABB6;
}

QMenu {
  background: #FFFFFF;
  border: 1px solid #DADFE6;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #D7E7FB;
  color: #10233F;
}
QMenu::item:disabled {
  color: #A4ABB6;
}
QMenu::separator {
  height: 1px;
  background: #E3E7EC;
  margin: 4px 8px;
}

QToolTip {
  color: #FFFFFF;
  background: #2A2E36;
  border: 1px solid #2A2E36;
  padding: 3px;
}

QHeaderView::section {
  background: #F1F3F6;
  color: #5E6673;
  border: none;
  border-bottom: 1px solid #DADFE6;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #E9ECF0;
}
TabBar::tab:selected {
  color: #10141A;
  border-bottom: 2px solid #0E9F84;
}
TabBar::tab:hover:!selected {
  background: #E1E5EA;
}

QSplitter::handle {
  background: #DADFE6;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#CF222E'
theme.property['color.yellow']       = '#9A6700'
theme.property['color.green']        = '#116329'
theme.property['color.teal']         = '#0E7C86'
theme.property['color.purple']       = '#8250DF'
theme.property['color.orange']       = '#BC4C00'
theme.property['color.blue']         = '#0550AE'
theme.property['color.black']        = '#1E232B'
theme.property['color.grey']         = '#6E7781'
theme.property['color.white']        = '#FFFFFF'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#3A414C,back:#F1F3F6'
theme.property['style.class']        = 'fore:$(color.orange)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.blue)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.black),back:$(color.white)'
theme.property['style.definition']   = 'fore:$(color.purple)'
theme.property['style.embedded']     = '$(style.tag),back:#F6F7F9'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.purple)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#E3E7EC,back:#E3E7EC'
theme.property['style.keyword']      = 'fore:$(color.red)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#8E96A3,back:#F6F7F9'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.blue)'
theme.property['style.operator']     = 'fore:$(color.black)'
theme.property['style.preprocessor'] = 'fore:$(color.red)'
theme.property['style.regex']        = 'fore:$(color.teal)'
theme.property['style.string']       = 'fore:#0A3069'
theme.property['style.tag']          = 'fore:$(color.green)'
theme.property['style.type']         = 'fore:$(color.orange)'
theme.property['style.variable']     = 'fore:$(color.orange)'
theme.property['style.whitespace']   = '$(style.nothing)'
