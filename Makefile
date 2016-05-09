
# we need xelatex for utf-8 support
tex = xelatex

all: userguide.pdf

.PHONY: all clean

userguide.pdf: userguide.tex
	$(tex) $^

userguide.tex: userguide/userguide.tex
	pandoc -f latex -t latex -s -o $@ $^

userguide/userguide.tex: rest-api.tex host-management.tex console.tex code-update.tex

%.tex: %.md
	pandoc -o $@ $^

clean:
	rm -f *.aux *.tex *.out *.pdf
