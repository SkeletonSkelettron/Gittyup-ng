--
-- GitHub Light Default: the default light theme of GitHub, with the colors
-- of its Primer design system.
--
-- See Default.lua for a description of the keys. The 'ui' table colors the
-- QML tool bar and sidebar, and 'stylesheet' is appended to the application
-- style sheet.
--

theme['palette']   = {
  light            = '#FFFFFF',
  midlight         = '#F6F8FA',
  middark          = '#D0D7DE',
  dark             = '#AFB8C1',
  shadow           = '#8C959F'
}

theme['widget']    = {
  text             = { default = '#1F2328', disabled = '#8C959F' },
  bright_text      = '#000000',
  background       = '#FFFFFF',
  alternate        = '#F6F8FA',
  highlight        = { active = '#BBDFFF', inactive = '#EAEEF2' },
  highlighted_text = { active = '#1F2328', inactive = '#1F2328' },
}

theme['window']    = {
  text             = '#1F2328',
  background       = '#F6F8FA'
}

theme['button']    = {
  text             = { default = '#1F2328', inactive = '#656D76', disabled = '#8C959F' },
  background       = { default = '#F6F8FA', checked = '#0969DA', pressed = '#EAEEF2' }
}

theme['commits']   = {
  text             = '#424A53',
  bright_text      = '#1F2328',
  background       = '#FFFFFF',
  alternate        = '#F6F8FA',
  highlight        = { active = '#DDF4FF', inactive = '#EAEEF2' },
  highlighted_text = { active = '#1F2328', inactive = '#1F2328' },
  highlighted_bright_text = { active = '#0A3069', inactive = '#1F2328' }
}

theme['badge']     = {
  foreground       = {
    normal         = '#1F2328',
    selected       = '#1F2328'
  },
  background       = {
    normal         = '#EAEEF2',
    selected       = '#FFFFFF',
    conflicted     = '#D8B9FF',
    head           = '#B6E3FF',
    notification   = '#FFCECB',
    modified       = '#FAE17D',
    added          = '#ACEEBB',
    deleted        = '#FFCECB',
    untracked      = '#C3F0F3',
    renamed        = '#DDF4FF'
  }
}

theme['blame'] = {
  cold             = '#DDF4FF',
  hot              = '#FFEBE9'
}

theme['graph']     = {
  edge1            = '#0969DA',
  edge2            = '#1A7F37',
  edge3            = '#9A6700',
  edge4            = '#8250DF',
  edge5            = '#CF222E',
  edge6            = '#BC4C00',
  edge7            = '#BF3989',
  edge8            = '#1B7C83',
  edge9            = '#218BFF',
  edge10           = '#2DA44E',
  edge11           = '#BF8700',
  edge12           = '#A475F9',
  edge13           = '#FA4549',
  edge14           = '#E16F24',
  edge15           = '#E85AAD'
}

theme['checkbox']  = {
  text             = '#1F2328',
  fill             = '#FFFFFF',
  outline          = '#D0D7DE'
}

theme['commiteditor'] = {
  spellerror       = '#CF222E',
  spellignore      = '#8C959F',
  lengthwarning    = '#FFF8C5'
}

theme['diff']      = {
  addition         = '#E6FFEC',
  deletion         = '#FFEBE9',
  plus             = '#1A7F37',
  minus            = '#CF222E',
  ours             = '#DDF4FF',
  theirs           = '#FBEFFF',
  word_addition    = '#ABF2BC',
  word_deletion    = '#FFCECB',
  note             = '#1F2328',
  warning          = '#FFF8C5',
  error            = '#FFEBE9'
}

theme['notice']    = {
  background       = '#FFF8C5',
  foreground       = '#7D4E00'
}

theme['link']      = {
  link             = '#0969DA',
  link_visited     = '#8250DF'
}

theme['menubar']   = {
  text             = '#1F2328',
  background       = '#F6F8FA'
}

theme['tabbar']   = {
  text             = '#656D76',
  base             = '#F6F8FA',
  selected         = '#FFFFFF',
}

theme['comment']   = {
  background       = '#F6F8FA',
  body             = '#1F2328',
  author           = '#0969DA',
  timestamp        = '#656D76'
}

theme['star']      = {
  fill             = '#BF8700'
}

theme['titlebar']  = {
  background       = '#F6F8FA'
}

theme['tooltip']   = {
  text             = '#FFFFFF',
  background       = '#24292F'
}

theme['ui']        = {
  dark             = false,
  base             = '#FFFFFF',
  alternate        = '#F6F8FA',
  panel            = '#F6F8FA',
  toolbar          = '#F6F8FA',
  sidebar          = '#F6F8FA',
  field            = '#FFFFFF',
  border           = '#D0D7DE',
  text             = '#1F2328',
  text_muted       = '#656D76',
  text_disabled    = '#8C959F',
  hover            = '#EAEEF2',
  pressed          = '#D8DEE4',
  selected         = '#DDF4FF',
  selected_text    = '#1F2328',
  accent           = '#0969DA',
  accent_text      = '#FFFFFF',
  badge            = '#CF222E',
  badge_text       = '#FFFFFF',
  ahead            = '#1A7F37',
  behind           = '#BC4C00',
  star             = '#BF8700',
  tooltip          = '#24292F',
  tooltip_text     = '#FFFFFF',
  alert            = '#CF222E',
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
  background: #D0D7DE;
  border-radius: 3px;
  margin: 2px;
}
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
  background: #AFB8C1;
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
  border: 1px solid #D0D7DE;
  border-radius: 5px;
  padding: 2px 4px;
  selection-background-color: #BBDFFF;
  selection-color: #1F2328;
}
QLineEdit:focus {
  border: 1px solid #0969DA;
}

QPushButton {
  background: #F6F8FA;
  color: #1F2328;
  border: 1px solid #D0D7DE;
  border-radius: 5px;
  padding: 4px 14px;
}
QPushButton:hover {
  background: #F3F4F6;
}
QPushButton:pressed {
  background: #EBECF0;
}
QPushButton:default {
  background: #1F883D;
  border-color: #1F883D;
  color: #FFFFFF;
}
QPushButton:default:hover {
  background: #1A7F37;
}
QPushButton:disabled {
  background: #F6F8FA;
  border-color: #D8DEE4;
  color: #8C959F;
}

QMenu {
  background: #FFFFFF;
  border: 1px solid #D0D7DE;
  padding: 4px;
}
QMenu::item {
  padding: 5px 24px 5px 20px;
  border-radius: 4px;
}
QMenu::item:selected {
  background: #DDF4FF;
  color: #1F2328;
}
QMenu::item:disabled {
  color: #8C959F;
}
QMenu::separator {
  height: 1px;
  background: #D8DEE4;
  margin: 4px 8px;
}

QToolTip {
  color: #FFFFFF;
  background: #24292F;
  border: 1px solid #24292F;
  padding: 3px;
}

QHeaderView::section {
  background: #F6F8FA;
  color: #656D76;
  border: none;
  border-bottom: 1px solid #D0D7DE;
  padding: 4px 6px;
}

TabBar::tab {
  padding: 0px 12px;
  border-bottom: 2px solid #F6F8FA;
}
TabBar::tab:selected {
  color: #1F2328;
  border-bottom: 2px solid #FD8C73;
}
TabBar::tab:hover:!selected {
  background: #EAEEF2;
}

QSplitter::handle {
  background: #D0D7DE;
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
theme.property['color.teal']         = '#1B7C83'
theme.property['color.purple']       = '#8250DF'
theme.property['color.orange']       = '#953800'
theme.property['color.blue']         = '#0550AE'
theme.property['color.black']        = '#1F2328'
theme.property['color.grey']         = '#6E7781'
theme.property['color.white']        = '#FFFFFF'

-- styles
theme.property['style.bracebad']     = 'fore:$(color.red)'
theme.property['style.bracelight']   = 'fore:$(color.blue),bold'
theme.property['style.calltip']      = 'fore:#424A53,back:#F6F8FA'
theme.property['style.class']        = 'fore:$(color.orange)'
theme.property['style.comment']      = 'fore:$(color.grey),italics'
theme.property['style.constant']     = 'fore:$(color.blue)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.black),back:$(color.white)'
theme.property['style.definition']   = 'fore:$(color.purple)'
theme.property['style.embedded']     = '$(style.tag),back:#F6F8FA'
theme.property['style.error']        = 'fore:$(color.red)'
theme.property['style.function']     = 'fore:$(color.purple)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#D8DEE4,back:#D8DEE4'
theme.property['style.keyword']      = 'fore:$(color.red)'
theme.property['style.label']        = 'fore:$(color.orange)'
theme.property['style.linenumber']   = 'fore:#8C959F,back:#FFFFFF'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.blue)'
theme.property['style.operator']     = 'fore:$(color.red)'
theme.property['style.preprocessor'] = 'fore:$(color.red)'
theme.property['style.regex']        = 'fore:#0A3069'
theme.property['style.string']       = 'fore:#0A3069'
theme.property['style.tag']          = 'fore:$(color.green)'
theme.property['style.type']         = 'fore:$(color.orange)'
theme.property['style.variable']     = 'fore:$(color.orange)'
theme.property['style.whitespace']   = '$(style.nothing)'
