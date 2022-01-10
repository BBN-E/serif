
usage:
	@echo "Usage:"
	@echo "  make run-serif"
	@echo "  make train-rel"
	@echo "  make clean"

run-serif:
	perl sequences/main.pl -sge -die_on_error

train-rel:
	perl sequences/train-rel.pl -sge -die_on_error

clean:
	rm -rf expts/*
	rm -rf ckpts/*
	rm -rf etemplates/*
