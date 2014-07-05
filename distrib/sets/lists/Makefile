# Various list maintenance functions

#   sort: verify each md/mi file is sorted and contains uniq lines
# mdtomi: move common md file lines to mi, if present
#  dupes: detect (nothing more) duplicate entries in each arch's complete file
#         list
# sanity: sanity check sets for wrong filecounts across archs


all: sort


sort:
	@for f in */m*; do echo "===> sorting $$f"; \
		sed 's/[ ]*$$//' $$f | sort | uniq > $$f.tmp; \
		diff -u $$f $$f.tmp || true; \
		cat $$f.tmp > $$f && rm $$f.tmp; \
	done

basefiles!= cd base;echo md.*
archs  = ${basefiles:S/^md.//}

mdtomi:
	@echo "Checking for common md file lines for migration to mi files:"
	@for xdir in base comp etc game; do \
	dir=$$(mktemp -d /tmp/mdtomi.XXXXXXXXXX); \
	i1=first; \
	for a in ${archs}; do \
		sort $$xdir/md.$$a >> $$dir/$$a; \
		if [ "X$$i1" = "Xfirst" ]; then \
			i1=$$dir/$$a; \
		else \
			cat $$i1 $$dir/$$a | sort | uniq -d >> $$dir/mi.tmp; \
			echo "===> ($$(echo $$(cat $$dir/mi.tmp|wc -c)))\c"; \
			echo " $$i1 vs $$dir/$$a"; \
			mv $$dir/mi.tmp $$dir/mi; \
			i1=$$dir/mi; \
		fi; \
	done; \
	echo "If any common md files were found, migrating to mi files:"; \
	cat $$dir/mi | while read line; do \
		echo "==> $$line"; \
		for line2 in $$(grep "^$${line}$$" $$xdir/m*); do \
			f=$${line2%:*}; \
			grep -v "^$${line}$$" $$f > $$f.tmp && mv $$f.tmp $$f; \
			echo "===> -$$line2"; \
		done; \
		echo "===> +$$xdir/mi"; \
		echo $$line >> $$xdir/mi; \
	done; rm -rf $$dir; \
	done

dupes:
	@for d in base comp etc game; do \
	for arch in ${archs}; do \
		[ -f $$d/md.$$arch ] || continue; \
		echo "===> $$d/$$arch"; \
		cat $$d/md.$$arch $$d/mi | sort | uniq -d; \
	done; \
	done

sanity:
	@for d in base comp etc game; do \
		echo "==> $$d"; \
		(cd $$d; cat md* | sort | uniq -c | sort -n ); \
	done
