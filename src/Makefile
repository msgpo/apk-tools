PKG_CONFIG		?= pkg-config
LUA			?= $(firstword $(wildcard /usr/bin/lua5.3 /usr/bin/lua5.2))

ifeq ($(LUA),no)
LUAAPK			?= no
else ifneq ($(LUA),)
LUAAPK			?= yes
else
$(error Lua interpreter not found. Please specify LUA interpreter, or use LUA=no to build without help.)
endif

OPENSSL_CFLAGS		:= $(shell $(PKG_CONFIG) --cflags openssl)
OPENSSL_LIBS		:= $(shell $(PKG_CONFIG) --libs openssl)

ZLIB_CFLAGS		:= $(shell $(PKG_CONFIG) --cflags zlib)
ZLIB_LIBS		:= $(shell $(PKG_CONFIG) --libs zlib)

# Dynamic library
libapk.so.$(VERSION)-objs := \
	common.o database.o package.o commit.o solver.o \
	version.o atom.o blob.o hash.o print.o \
	io.o io_url.o io_gunzip.o io_archive.o

libapk.so.$(VERSION)-libs := libfetch/libfetch.a

libapk_soname		:= $(VERSION)
shlibs-y		+= libapk.so.$(VERSION)
LDFLAGS_libapk.so.$(VERSION) += -Wl,-soname,libapk.so.$(libapk_soname)

libapk_so		:= $(obj)/libapk.so.$(VERSION)
install-libapk_so	:= $(INSTALLDIR) $(DESTDIR)$(LIBDIR) && \
			   $(INSTALL) $(libapk_so) $(DESTDIR)$(LIBDIR)/libapk.so.$(VERSION) && \
			   $(INSTALL) $(obj)/libapk.so.$(libapk_soname) $(DESTDIR)$(LIBDIR) && \
			   $(INSTALL) $(obj)/libapk.so $(DESTDIR)$(LIBDIR)

# Static library
libapk.a-objs		:= $(libapk.so.$(VERSION)-objs)
libapk.a-libs		:= $(libapk.so.$(VERSION)-libs)

libs-y			+= libapk.a
libapk_a		:= $(obj)/libapk.a

install-libapk_a	:= $(INSTALLDIR) $(DESTDIR)$(LIBDIR) && \
			   $(INSTALL) $(libapk_a) $(DESTDIR)$(LIBDIR)
apk-static-libs		:= $(libapk.a-libs) $(obj)/libapk.a

# Lua module
ifeq ($(LUAAPK),yes)
LUA_VERSION		?= 5.3
LUA_PC			?= lua$(LUA_VERSION)
LUA_LIBDIR		?= /usr/lib/lua/$(LUA_VERSION)
shlibs-y		+= apk.so
apk.so-objs		:= lua-apk.o
CFLAGS_lua-apk.o	:= -DAPK_VERSION=\"$(FULL_VERSION)\" \
			   $(shell $(PKG_CONFIG) $(LUA_PC) --cflags)
apklua_so		:= $(obj)/apk.so
install-apklua_so	:= $(INSTALLDIR) $(DESTDIR)$(LUA_LIBDIR) && \
			   $(INSTALL) $(apklua_so) $(DESTDIR)$(LUA_LIBDIR)
endif

# Apk utility
progs-y			+= apk
apk-objs		:= apk.o help.o \
			   app_add.o app_del.o app_fix.o app_update.o app_upgrade.o \
			   app_info.o app_list.o app_search.o app_manifest.o \
			   app_policy.o app_stats.o \
			   app_cache.o app_version.o \
			   app_index.o app_fetch.o app_verify.o app_dot.o \
			   app_audit.o

LIBS_apk		:= -lapk
LIBS_apk-test		:= -lapk
LIBS_apk.so		:= -L$(obj) -lapk

CFLAGS_ALL		+= -D_ATFILE_SOURCE -Ilibfetch
CFLAGS_apk.o		:= -DAPK_VERSION=\"$(FULL_VERSION)\"
CFLAGS_apk-static.o	:= -DAPK_VERSION=\"$(FULL_VERSION)\" -DOPENSSL_NO_ENGINE
CFLAGS_apk-test.o	:= -DAPK_VERSION=\"$(FULL_VERSION)\" -DOPENSSL_NO_ENGINE -DTEST_MODE

progs-$(STATIC)		+= apk.static
apk.static-objs		:= $(filter-out apk.o,$(apk-objs)) apk-static.o
apk.static-libs		:= $(apk-static-libs)
LDFLAGS_apk.static	:= -static
LIBS_apk.static		:= -Wl,--as-needed -ldl -Wl,--no-as-needed
LDFLAGS_apk		+= -L$(obj)
LDFLAGS_apk-test	+= -L$(obj)

CFLAGS_ALL		+= $(OPENSSL_CFLAGS) $(ZLIB_CFLAGS)
LIBS			:= -Wl,--as-needed \
				$(OPENSSL_LIBS) $(ZLIB_LIBS) \
			   -Wl,--no-as-needed

# Help generation
quiet_cmd_genhelp = GENHELP $@
ifneq ($(LUA),no)
      cmd_genhelp = $(LUA) $(src)/genhelp.lua $(filter %.scd, $^) > $@
else
      cmd_genhelp = echo \\\#define NO_HELP > $@
endif

$(obj)/help.h: $(src)/genhelp.lua $(wildcard doc/apk*.8.scd) FORCE
	$(call if_changed,genhelp)

CFLAGS_help.o		:= -I$(obj)

generate-y		+= help.h

# Test build
ifeq ($(TEST),y)
progs-y			+= apk-test
apk-test-objs		:= apk-test.o $(filter-out apk.o, $(apk-objs))
endif

$(obj)/apk: $(libapk_so)

$(obj)/apk-test: $(libapk_so)

$(obj)/apk.so: $(libapk_so)

ifneq ($(libapk_soname), $(VERSION))
generate-y += libapk.so.$(libapk_soname)

$(obj)/libapk.so.$(libapk_soname):
	$(LN) libapk.so.$(VERSION) $(obj)/libapk.so.$(libapk_soname)
endif

generate-y	+= libapk.so

$(obj)/libapk.so:
	$(LN) libapk.so.$(libapk_soname) $(obj)/libapk.so

generate-y	+= apk.pc

install: $(obj)/apk $(libapk_so) $(libapk_a) $(apklua_so)
	$(INSTALLDIR) $(DESTDIR)$(SBINDIR)
	$(INSTALL) $(obj)/apk $(DESTDIR)$(SBINDIR)
	$(install-libapk_so)
	$(install-libapk_a)
	$(install-apklua_so)
	$(INSTALLDIR) $(DESTDIR)$(INCLUDEDIR)/apk
	$(INSTALL) -m644 src/*.h $(DESTDIR)$(INCLUDEDIR)/apk
	$(INSTALLDIR) $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m644 $(obj)/apk.pc $(DESTDIR)$(PKGCONFIGDIR)

