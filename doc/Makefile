FILE := $(shell ls *.md)

all: $(FILE:md=html)

%.html: %.md
	pandoc -f markdown_github -s -o $@ $^

clean:
	@ rm -f *.html
