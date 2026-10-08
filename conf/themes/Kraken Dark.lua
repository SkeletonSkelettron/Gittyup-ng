--
-- Kraken Dark: a flat, dark theme with a teal accent and vivid graph lanes.
--
-- See Dark.lua for a description of the keys. The 'ui' table colors the QML
-- tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  -- inverted for a light on dark theme
  light            = '#1C1F24',
  midlight         = '#20242A',
  middark          = '#2A2E36',
  dark             = '#343A44',
  shadow           = '#15171B'
}

theme['widget']    = {
  text             = { default = '#DCE0E8', disabled = '#5B6270' },
  bright_text      = '#FFFFFF',
  background       = '#1C1F24',
  alternate        = '#20242A',
  highlight        = { active = '#2B5A8A', inactive = '#2B3644' },
  highlighted_text = { active = '#FFFFFF', inactive = '#DCE0E8' },
}

theme['window']    = {
  text             = '#DCE0E8',
  background       = '#23272E'
}

theme['button']    = {
  text             = { default = '#DCE0E8', inactive = '#8B93A1', disabled = '#5B6270' },
  background       = { default = '#2A2E36', checked = '#1BC5A4', pressed = '#3A404B' }
}

theme['commits']   = {
  text             = '#AEB6C3',
  bright_text      = '#E8EBF0',
  background       = '#1C1F24',
  alternate        = '#1F2228',
  highlight        = { active = '#2B5A8A', inactive = '#2B3644' },
  highlighted_text = { active = '#E8EBF0', inactive = '#DCE0E8' },
  highlighted_bright_text = { active = '#FFFFFF', inactive = '#E8EBF0' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#FFFFFF',
    selected       = '#1C1F24'
  },
  background       = {
    normal         = '#3B5673',
    selected       = '#E8EBF0',
    conflicted     = '#C43FD6',
    head           = '#12977F',
    notification   = '#E0634F',
    modified       = '#7D6420',
    added          = '#1E6B46',
    deleted        = '#8A2E36',
    untracked      = '#1C5E66',
    renamed        = '#2A5288'
  }
}

theme['blame'] = {
  cold             = '#222A38',
  hot              = '#4A2A30'
}

theme['graph']     = {
  edge1            = '#15A0BF',
  edge2            = '#0669F7',
  edge3            = '#8E00C2',
  edge4            = '#C517B6',
  edge5            = '#D90171',
  edge6            = '#CD0101',
  edge7            = '#F25D2E',
  edge8            = '#F2CA33',
  edge9            = '#7BD938',
  edge10           = '#2ECE9D',
  edge11           = '#39B5E0',
  edge12           = '#A56EFF',
  edge13           = '#FF7EB6',
  edge14           = '#FFB224',
  edge15           = '#6FDC8C'
}

theme['checkbox']  = {
  text             = '#DCE0E8',
  fill             = '#2A2E36',
  outline          = '#434A56'
}

theme['commiteditor'] = {
  spellerror       = '#F05A5A',
  spellignore      = '#DCE0E8',
  lengthwarning    = '#3D3520'
}

theme['diff']      = {
  addition         = '#1C3A2A',
  deletion         = '#45252A',
  plus             = '#3FB950',
  minus            = '#F85149',
  ours             = '#1D2E4A',
  theirs           = '#3B2248',
  word_addition    = '#2A6B45',
  word_deletion    = '#7A2F38',
  note             = '#DCE0E8',
  warning          = '#E8C080',
  error            = '#7E494B'
}

theme['notice']    = {
  background       = '#3D3316',
  foreground       = '#FFE7A3'
}

theme['link']      = {
  link             = '#3FB6E8',
  link_visited     = '#B48EF0'
}

theme['menubar']   = {
  text             = '#DCE0E8',
  background       = '#1F2228'
}

theme['tabbar']   = {
  text             = '#AEB6C3',
  base             = '#1F2228',
  selected         = '#23272E',
}

theme['comment']   = {
  background       = '#20242A',
  body             = '#AEB6C3',
  author           = '#3FB6E8',
  timestamp        = '#8B93A1'
}

theme['star']      = {
  fill             = '#F2C94C'
}

theme['titlebar']  = {
  background       = '#2A2E36'
}

theme['tooltip']   = {
  text             = '#DCE0E8',
  background       = '#2F343D'
}

theme['ui']        = {
  dark             = true,
  base             = '#1C1F24',
  alternate        = '#1F2228',
  panel            = '#1F2228',
  toolbar          = '#2A2E36',
  sidebar          = '#1F2228',
  field            = '#1C1F24',
  border           = '#343A44',
  text             = '#DCE0E8',
  text_muted       = '#8B93A1',
  text_disabled    = '#555C69',
  hover            = '#343A44',
  pressed          = '#3F4652',
  selected         = '#2B3F55',
  selected_text    = '#FFFFFF',
  accent           = '#1BC5A4',
  accent_text      = '#0E1A17',
  badge            = '#E0634F',
  badge_text       = '#FFFFFF',
  ahead            = '#12977F',
  behind           = '#D9822B',
  star             = '#F2C94C',
  tooltip          = '#2F343D',
  tooltip_text     = '#DCE0E8'
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
  background: #3F4652;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #566070;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #1C1F24;
  border: 1px solid #343A44;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #2B5A8A;
}
QLineEdit:focus {
  border: 1px solid #1BC5A4;
}

QPushButton {
  background: #2F343D;
  color: #DCE0E8;
  border: 1px solid #3F4652;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #3A404B;
}
QPushButton:pressed {
  background: #444B57;
}
QPushButton:default {
  background: #12977F;
  border-color: #12977F;
  color: #FFFFFF;
}
QPushButton:default:hover {
  background: #16AD91;
}
QPushButton:disabled {
  background: #262A31;
  border-color: #2F343D;
  color: #5B6270;
}

QMenu {
  background: #262A31;
  border: 1px solid #343A44;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #2B5A8A;
  color: #FFFFFF;
}
QMenu::item:disabled {
  color: #5B6270;
}
QMenu::separator {
  height: 1px;
  background: #343A44;
  margin: 4px 8px;
}

QToolTip {
  color: #DCE0E8;
  background: #2F343D;
  border: 1px solid #434A56;
  padding: 3px;
}

QHeaderView::section {
  background: #23272E;
  color: #8B93A1;
  border: none;
  border-bottom: 1px solid #343A44;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #1F2228;
}
TabBar::tab:selected {
  color: #FFFFFF;
  border-bottom: 2px solid #1BC5A4;
}
TabBar::tab:hover:!selected {
  background: #262A31;
}

QSplitter::handle {
  background: #343A44;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#E06C75'
theme.property['color.yellow']       = '#E5C07B'
theme.property['color.green']        = '#98C379'
theme.property['color.teal']         = '#56B6C2'
theme.property['color.purple']       = '#C678DD'
theme.property['color.orange']       = '#D19A66'
theme.property['color.blue']         = '#61AFEF'
theme.property['color.black']        = '#1C1F24'
theme.property['color.grey']         = '#7F8794'
theme.property['color.white']        = '#DCE0E8'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#AEB6C3,back:#2A2E36'
theme.property['style.class']        = 'fore:$(color.yellow)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.orange)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:#C8CDD6,back:#1C1F24'
theme.property['style.definition']   = 'fore:$(color.blue)'
theme.property['style.embedded']     = '$(style.tag),back:#2A2E36'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.blue)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#2A2E36,back:#2A2E36'
theme.property['style.keyword']      = 'fore:$(color.purple)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#5B6270,back:#1F2228'
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
