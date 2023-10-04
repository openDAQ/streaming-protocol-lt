TARGETS := openDAQ-streaming.pdf openDAQ-signal-definition-examples.pdf
TARGETS += openDAQ-streaming.html openDAQ-signal-definition-examples.html openDAQ-streaming-workflow.html openDAQ-streaming-optional-features.html

TARGETS += client-requirements.pdf

all: .git/hooks/post-commit .git/hooks/post-checkout

.git/hooks:
	mkdir -p $@

.git/hooks/post-commit: .git/hooks
	cp post-xxx-sample.txt .git/hooks/post-commit
	chmod a+x ./.git/hooks/post-commit
	git checkout

.git/hooks/post-checkout: .git/hooks
	cp post-xxx-sample.txt .git/hooks/post-checkout
	chmod a+x ./.git/hooks/post-checkout
	git checkout

all: $(TARGETS)

%.pdf: %.md
	pandoc -s -f markdown -t latex --template openDAQ.template  -o $@ $<

%.html: %.md
	pandoc -s --css=openDAQ.css --toc --toc-depth=3 \
	--mathjax \
	-N -f markdown $< -o $@

clean:
	rm -f *.epub *.html *.docx $(TARGETS)
