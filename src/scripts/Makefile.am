bin_SCRIPTS = \
	assembly_stats.py \
	extract_CG_contig.py \
	fastq_sample_mate_pairs.sh \
	kmer_counts.py \
	methratio2gff.py \
	methratios2table.py \
	table2fastq.py

EXTRA_DIST = \
	assembly_stats.py \
	fastq_sample_mate_pairs.sh.in \
	extract_CG_contig.py \
	kmer_counts.py \
	methratio2gff.py \
	methratios2table.py \
	plot_coverage.R \
	plot_qual.R \
	table2fastq.py

fastq_sample_mate_pairs.sh: fastq_sample_mate_pairs.sh.in
	sed \
		-e 's|@bindir[@]|$(bindir)|g' \
		< "$<" > "$@"

r_scriptsdir = $(pkgdatadir)/r_scripts

r_scripts_DATA = \
	plot_coverage.R \
	plot_qual.R

CLEANFILES = \
	fastq_sample_mate_pairs.sh

MAINTAINERCLEANFILES = \
	Makefile.in
