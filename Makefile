

LDFLAGS =  -L /usr/local/lib 

MODULE_big = gunzip
OBJS = gunzip.o

EXTENSION = gunzip
DATA = gunzip--0.1.sql

EXTRA-CLEAN =

#PG_CONFIG = pg_config
PG_CONFIG =/usr/lib/postgresql/9.6/bin/pg_config

CFLAGS += $(shell $(CURL_CONFIG) --cflags)

LIBS += $(LDFLAGS)
SHLIB_LINK := $(LIBS)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
