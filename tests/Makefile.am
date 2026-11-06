check_PROGRAMS = test_data_structs test_finder test_parser
EXTRA_PROGRAMS = fuzz_parser
TESTS = $(check_PROGRAMS)
AM_CPPFLAGS = -I$(top_srcdir)/src/include
LDADD = $(top_builddir)/src/libizumi.la
EXTRA_DIST = test.h fuzz_parser.py
