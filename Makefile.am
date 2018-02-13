bin_PROGRAMS = bsapply bschecksum bsdata bsinfo

common_sources = src/bsheader.c src/bsheader.h src/common.c src/common.h error.h
bsapply_SOURCES = src/bs_apply.c $(common_sources)
bschecksum_SOURCES = src/bs_checksum.c $(common_sources)
bsdata_SOURCES = src/bs_data.c $(common_sources)
bsinfo_SOURCES = src/bs_info.c $(common_sources)

dist_bin_SCRIPTS = src/binary-sync
