
BUILD_SERIF = sequences/build_serif.pl
PERL = perl

# Variables for daily-build target:
MAILTO = $(USER)@bbn.com
REPLYTO = $(USER)@bbn.com
REVISION = head
N = 4

QUICK_REGTEST = "English/x86_64/OldSymbol/Classic/Release/Fast"  # 1 config
QUICK_TPUT = "x86_64/OldSymbol/Classic/Release/TPutFast+TPutBest" # 6 configs

.PHONY: backup clean run daily-regtest daily-tput

run:
	perl $(BUILD_SERIF) -sge --build rev-${REVISION}

daily-regtest:
	$(PERL) "$(BUILD_SERIF)" --sge --name "SERIF Daily Build" \
	    --return-success-after-reporting-failure \
	    --build rev-${REVISION}/Classic \
	    --build rev-${REVISION}/Best/Modularized \
	    --build rev-${REVISION}/Best/CA \
	    --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)"

daily-tput:
	rm -f ckpts/*-bargraph-*.ckpt
	$(PERL) "$(BUILD_SERIF)" --sge --name "SERIF Daily Build" \
	    --return-success-after-reporting-failure \
	    --build rev-${REVISION} \
	    --tput --save-tput --one-machine \
	    --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)"

# Just build a single configuration:
quick-regtest:
	perl $(BUILD_SERIF) -sge \
	    --build rev-${REVISION}/${QUICK_REGTEST}

quick-tput:
	rm -f ckpts/*-bargraph-*.ckpt
	$(PERL) "$(BUILD_SERIF)" --sge --name "SERIF Daily Build" \
	    --return-success-after-reporting-failure \
	    --build rev-${REVISION}/${QUICK_TPUT} \
	    --tput --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)"

clean:
	rm -rf expts/*
	rm -rf etemplates/*
	rm -rf ckpts/*
	rm -rf logfiles/*

backup: backup/.exists
	@(DIR="backup/`date +'%F.%H:%M:%S'`" && \
		mkdir -p $$DIR/etemplates && \
		mkdir -p $$DIR/ckpts && \
		mkdir -p $$DIR/expts && \
		mkdir -p $$DIR/logfiles && \
		mv etemplates/* $$DIR/etemplates && \
		mv ckpts/* $$DIR/ckpts && \
		mv expts/* $$DIR/expts && \
		mv logfiles/* $$DIR/logfiles && \
	  echo "Backup -> $$DIR") || echo "Error creating backup!"

backup/.exists:
	mkdir -p '$(RAID)/backup'
	ln -s '$(RAID)/backup' backup
	touch backup/.exists

symbol-tput:
	rm -f ckpts/*-bargraph-*.ckpt
	$(PERL) "$(BUILD_SERIF)" --sge --name "SERIF Throughput Test: Symbols" \
	    --return-success-after-reporting-failure \
	    --build rev-${REVISION}/OldSymbol+NewSymbol+NoRefCount+NoThread/Classic/English/TPutBest+TPutFast \
	    --tput --tput-runs 32 --discard-outliers 16\
	    --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)"

build-test:
	$(PERL) "$(BUILD_SERIF)" --sge --name "SERIF Build Test" --build-only \
	    --build rev-${REVISION}/Classic \
	    --build rev-${REVISION}/Best/Modularized \
	    --build rev-${REVISION}/Best/CA \
	    --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)"

old-release:
	$(PERL) "$(BUILD_SERIF)" --sge --name "Assemble SERIF Release" \
	    --assemble-release \
	    --build rev-${REVISION}/Classic \
	    --build rev-${REVISION}/Best/CA \
	    --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)"

release:
	$(PERL) "$(BUILD_SERIF)" --sge --name "Assemble SERIF Release" \
		--build-only --assemble-release \
		--mailto "$(MAILTO)" --mailfrom "$(REPLYTO)" \
		--build-shared-libs --build-java \
		--build rev-${REVISION}/NoThread/Classic/Release/x86_64/Best \
		--build rev-${REVISION}/NoThread/Server/Release/x86_64/Best \
		</dev/null

release2:
	$(PERL) "$(BUILD_SERIF)" --sge --name "Assemble SERIF Release" \
	    --build rev-${REVISION}/Classic+Server/NoRefCount/x86_64+i686/English+Arabic/Best \
	    --build-only --mailto "$(MAILTO)" --mailfrom "$(REPLYTO)" </dev/null

