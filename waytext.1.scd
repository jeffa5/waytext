waytext(1)

# NAME

waytext - show text on all outputs to the user

# DESCRIPTION

*waytext* [options...]

# SYNOPSIS

waytext is a command-line utility to display text on all outputs using the
Wayland layer-shell protocol.

# OPTIONS

*-h*
	Show help message and quit.

*-b* _color_
	Set background color. See *COLORS* for more detail. Default=#ffffff40

*-f* _color_
	Set foreground color. See *COLORS* for more detail. Default=#000000ff

*-w* _weight_
	Set the font weight. Default=100

*-t* _text_
	Set the text to display.

# COLORS

Colors may be specified in #RRGGBB or #RRGGBBAA format. The # is optional.

# AUTHORS

Maintained by Jeffas <dev@jeffas.io>.
