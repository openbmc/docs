# To build:
#  $ apt install fonts-linuxlibertine texlive-xetex pandoc
#  $ make

# we need xelatex for utf-8 support
tex = xelatex
tflags = -interaction=nonstopmode -halt-on-error

all: userguide.pdf

.PHONY: all clean

userguide.pdf: userguide/userguide.tex
	$(tex) $(tflags) $^
	$(tex) $(tflags) $^

userguide/userguide.tex: rest-api.tex host-management.tex console.tex code-update.tex

%.tex: %.md
	pandoc -o $@ $^

clean:
	rm -f *.tex userguide.*
