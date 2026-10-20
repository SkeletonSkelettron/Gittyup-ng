--
-- GitHub Dark Dimmed: the dimmed dark theme of GitHub, with the colors of
-- its Primer design system.
--
-- See Dark.lua for a description of the keys. The 'ui' table colors the QML
-- tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  -- inverted for a light on dark theme
  light            = '#22272E',
  midlight         = '#2D333B',
  middark          = '#373E47',
  dark             = '#444C56',
  shadow           = '#1C2128'
}

theme['widget']    = {
  text             = { default = '#ADBAC7', disabled = '#545D68' },
  bright_text      = '#CDD9E5',
  background       = '#22272E',
  alternate        = '#2D333B',
  highlight        = { active = '#2E4C77', inactive = '#2D333B' },
  highlighted_text = { active = '#CDD9E5', inactive = '#ADBAC7' },
}

theme['window']    = {
  text             = '#ADBAC7',
  background       = '#2D333B'
}

theme['button']    = {
  text             = { default = '#ADBAC7', inactive = '#768390', disabled = '#545D68' },
  background       = { default = '#373E47', checked = '#316DCA', pressed = '#444C56' }
}

theme['commits']   = {
  text             = '#ADBAC7',
  bright_text      = '#CDD9E5',
  background       = '#22272E',
  alternate        = '#252B33',
  highlight        = { active = '#2A3E5B', inactive = '#2D333B' },
  highlighted_text = { active = '#CDD9E5', inactive = '#ADBAC7' },
  highlighted_bright_text = { active = '#FFFFFF', inactive = '#CDD9E5' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#FFFFFF',
    selected       = '#22272E'
  },
  background       = {
    normal         = '#444C56',
    selected       = '#ADBAC7',
    conflicted     = '#8256D0',
    head           = '#316DCA',
    notification   = '#C93C37',
    modified       = '#966600',
    added          = '#347D39',
    deleted        = '#C93C37',
    untracked      = '#1F6F72',
    renamed        = '#316DCA'
  }
}

theme['blame'] = {
  cold             = '#273444',
  hot              = '#43303A'
}

theme['graph']     = {
  edge1            = '#539BF5',
  edge2            = '#57AB5A',
  edge3            = '#C69026',
  edge4            = '#986EE2',
  edge5            = '#E5534B',
  edge6            = '#CC6B2C',
  edge7            = '#C96198',
  edge8            = '#4BB8C3',
  edge9            = '#6CB6FF',
  edge10           = '#8DDB8C',
  edge11           = '#DAAA3F',
  edge12           = '#DCBDFB',
  edge13           = '#FF938A',
  edge14           = '#F69D50',
  edge15           = '#FC8DC7'
}

theme['checkbox']  = {
  text             = '#ADBAC7',
  fill             = '#2D333B',
  outline          = '#444C56'
}

theme['commiteditor'] = {
  spellerror       = '#E5534B',
  spellignore      = '#ADBAC7',
  lengthwarning    = '#37342A'
}

theme['diff']      = {
  addition         = '#273832',
  deletion         = '#362B31',
  plus             = '#57AB5A',
  minus            = '#E5534B',
  ours             = '#273549',
  theirs           = '#343249',
  word_addition    = '#305339',
  word_deletion    = '#70393A',
  note             = '#ADBAC7',
  warning          = '#C69026',
  error            = '#5A3036'
}

theme['notice']    = {
  background       = '#37342A',
  foreground       = '#DAAA3F'
}

theme['link']      = {
  link             = '#539BF5',
  link_visited     = '#986EE2'
}

theme['menubar']   = {
  text             = '#ADBAC7',
  background       = '#1C2128'
}

theme['tabbar']   = {
  text             = '#768390',
  base             = '#1C2128',
  selected         = '#22272E',
}

theme['comment']   = {
  background       = '#2D333B',
  body             = '#ADBAC7',
  author           = '#539BF5',
  timestamp        = '#768390'
}

theme['star']      = {
  fill             = '#DAAA3F'
}

theme['titlebar']  = {
  background       = '#1C2128'
}

theme['tooltip']   = {
  text             = '#ADBAC7',
  background       = '#373E47'
}

theme['ui']        = {
  dark             = true,
  base             = '#22272E',
  alternate        = '#252B33',
  panel            = '#1C2128',
  toolbar          = '#1C2128',
  sidebar          = '#1C2128',
  field            = '#22272E',
  border           = '#444C56',
  text             = '#ADBAC7',
  text_muted       = '#768390',
  text_disabled    = '#545D68',
  hover            = '#2D333B',
  pressed          = '#373E47',
  selected         = '#2A3E5B',
  selected_text    = '#CDD9E5',
  accent           = '#539BF5',
  accent_text      = '#1C2128',
  badge            = '#C93C37',
  badge_text       = '#FFFFFF',
  ahead            = '#57AB5A',
  behind           = '#CC6B2C',
  star             = '#DAAA3F',
  tooltip          = '#373E47',
  tooltip_text     = '#ADBAC7',
  alert            = '#A82E2C',
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
  background: #444C56;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #545D68;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: none;
}

QLineEdit {
  background: #22272E;
  border: 1px solid #444C56;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #2E4C77;
}
QLineEdit:focus {
  border: 1px solid #539BF5;
}

QPushButton {
  background: #373E47;
  color: #ADBAC7;
  border: 1px solid #444C56;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #444C56;
}
QPushButton:pressed {
  background: #545D68;
}
QPushButton:default {
  background: #347D39;
  border-color: #347D39;
  color: #FFFFFF;
}
QPushButton:default:hover {
  background: #46954A;
}
QPushButton:disabled {
  background: #2D333B;
  border-color: #373E47;
  color: #545D68;
}

QMenu {
  background: #2D333B;
  border: 1px solid #444C56;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #316DCA;
  color: #FFFFFF;
}
QMenu::item:disabled {
  color: #545D68;
}
QMenu::separator {
  height: 1px;
  background: #444C56;
  margin: 4px 8px;
}

QToolTip {
  color: #ADBAC7;
  background: #373E47;
  border: 1px solid #545D68;
  padding: 3px;
}

QHeaderView::section {
  background: #2D333B;
  color: #768390;
  border: none;
  border-bottom: 1px solid #444C56;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #1C2128;
}
TabBar::tab:selected {
  color: #CDD9E5;
  border-bottom: 2px solid #EC775C;
}
TabBar::tab:hover:!selected {
  background: #2D333B;
}

QSplitter::handle {
  background: #444C56;
}
]]

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#F47067'
theme.property['color.yellow']       = '#DAAA3F'
theme.property['color.green']        = '#8DDB8C'
theme.property['color.teal']         = '#4BB8C3'
theme.property['color.purple']       = '#DCBDFB'
theme.property['color.orange']       = '#F69D50'
theme.property['color.blue']         = '#6CB6FF'
theme.property['color.black']        = '#22272E'
theme.property['color.grey']         = '#768390'
theme.property['color.white']        = '#ADBAC7'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#ADBAC7,back:#2D333B'
theme.property['style.class']        = 'fore:$(color.orange)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.blue)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.white),back:$(color.black)'
theme.property['style.definition']   = 'fore:$(color.purple)'
theme.property['style.embedded']     = '$(style.tag),back:#2D333B'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.purple)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#373E47,back:#373E47'
theme.property['style.keyword']      = 'fore:$(color.red)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#636E7B,back:#22272E'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.blue)'
theme.property['style.operator']     = 'fore:$(color.red)'
theme.property['style.preprocessor'] = 'fore:$(color.red)'
theme.property['style.regex']        = 'fore:#96D0FF'
theme.property['style.string']       = 'fore:#96D0FF'
theme.property['style.tag']          = 'fore:$(color.green)'
theme.property['style.type']         = 'fore:$(color.orange)'
theme.property['style.variable']     = 'fore:$(color.orange)'
theme.property['style.whitespace']   = '$(style.nothing)'
