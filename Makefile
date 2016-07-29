#
# Copyright (c) 2014-2015, Hewlett-Packard Development Company, LP.
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details. You should have received a copy of the GNU General Public
# License along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# HP designates this particular file as subject to the "Classpath" exception
# as provided by HP in the LICENSE.txt file that accompanied this code.
#
# The "Classpath" exception is restated below:
#
# Certain source files distributed by Hewlett-Packard Company and/or its 
# affiliates(“HP”) are subject to the following clarification and special 
# exception to the GPL, but only where HP has expressly included in the 
# particular source file's header the words "HP designates this particular file 
# as subject to the "Classpath" exception as provided by HP in the LICENSE file 
# that accompanied this code."
# 
# Linking this library statically or dynamically with other modules is making 
# a combined work based on this library.  Thus, the terms and conditions of the 
# GNU General Public License cover the whole combination.
#
# As a special exception, the copyright holders of this library give you 
# permission to link this library with independent modules to produce an 
# executable, regardless of the license terms of these independent modules, 
# and to copy and distribute the resulting executable under terms of your 
# choice, provided that you also meet, for each linked independent module, 
# the terms and conditions of the license of that module. An independent module 
# is a module which is not derived from or based on this library. If you modify 
# this library, you may extend this exception to your version of the library, 
# but you are not obligated to do so. If you do not wish to do so, delete this 
# exception statement from your version.
#




# Make to end all make

# TODO
# bring in line with http://aegis.sourceforge.net/auug97.pdf
# header .d files for tracking header dependencies (-MM -MG)
# better bin and archive solution than copying
# find executable source?

# ===============================================
# User configured variables
# ===============================================

WARNING_FLAGS:=-ftrapv -Wreturn-type -W -Wall \
-Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter

ANNOYING_FLAGS:= -Wall -W -Wextra -Wundef -Wshadow \
-Wunreachable-code -Wredundant-decls -Wunused-macros \
-Wcast-qual -Wcast-align -Wwrite-strings -Wmissing-field-initializers \
-Wendif-labels -Winit-self -Wlogical-op -Wmissing-declarations \
-Wpacked -Wstack-protector -Wformat=2 -Wswitch-default -Wswitch-enum \
-Wunused -Wstrict-overflow=5 -Wpointer-arith -Wnormalized=nfc \
-Wlong-long -Wconversion -Wmissing-format-attribute -Wpadded \
-Winline -Wvariadic-macros -Wvla -Wdisabled-optimization \
-Woverlength-strings -Waggregate-return -Wmissing-prototypes \
-Wstrict-prototypes -Wold-style-definition -Wbad-function-cast \
-Wc++-compat -Wjump-misses-init -Wnested-externs \
-Wdeclaration-after-statement -ftrapv 

IDIRS:=. 
ODIR:=./obj
LDIRS:=.
CFLAGS:=-pthread -g -gdwarf-2 -fpic $(WARNING_FLAGS)
CXXFLAGS:= -pthread -std=c++11 -g -fpic $(WARNING_FLAGS)
CFLAGS_DEBUG:= -O0
CXXFLAGS_DEBUG:= -O0
CFLAGS_RELEASE:= -O3
CXXFLAGS_RELEASE:= -O3
LIBS:=-lpthread -l:libjemalloc.so.1
LDFLAGS:= 
SRCDIRS:= .
BINDIR:= ./bin
# we input LIBS and output ARCHIVES
STATICARCHIVE:= 
SHAREDARCHIVE:= 


# -generated executables have the same name 
# as their source file but with the .bin file 
# extension
# -since all executables go into the same folder,
# they must each have unique file names (even if the
# source files are in different folders)
# -since we do pattern matching between this list and the
# source files, the file path specified must be the same
# type (absolute or relative)

EXECUTABLES:= tests/simple.cpp tests/timing.cpp tests/threads.cpp

IGNORES:=
#IGNORES:= hashtrie/hashtrie.cpp hashtrie/hashtrienodes.cpp

HOSTNAME:=$(shell hostname)
ifeq ($(HOSTNAME),vax)
	CC:=clang
	CXX:=clang++
else
	CC:=gcc
	CXX:=g++
endif
LD:= $(CXX)


ACC:=atlas
ACXX:=atlas++
ALD:=$(ACXX)


all: release

.PHONY: annoying
annoying : _CXX_COMP_ARGS += $(ANNOYING_FLAGS)
annoying : _C_COMP_ARGS += $(ANNOYING_FLAGS)
annoying : all

.PHONY: release
release : _CXX_COMP_ARGS += $(CXXFLAGS_RELEASE)
release : _C_COMP_ARGS += $(CFLAGS_RELEASE)
release : to_build

.PHONY: debug
debug : _CXX_COMP_ARGS += $(CXXFLAGS_DEBUG)
debug : _C_COMP_ARGS += $(CFLAGS_DEBUG)
debug : to_build

# what gets built by default
to_build : bin 
# bin : compiles all executables
# lib : compiles archive files (.a , .so)



# =============================================
# Automatic stuff. Hopefully you don't need to
# access below here.
# =============================================

# -------------------------
# Functions
# -------------------------

_SEP:=-_

# mangle takes a file path and converts it into 
# a basename for a single file
mangle=$(subst /,$(_SEP),$(basename $1))

# unmangle takes a file name and converts it
# into a full file path 
unmangle=$(subst $(_SEP),/,$(notdir $(basename $1)))

# mangletarget takes an executable and converts it into 
# a basename for a single file
#mangletarget=$(subst /,$(_SEP),$1)
mangletarget=$(subst $(_SEP),/,$(strip $1))

# because make doesn't have a newline character
define \n

endef

# because make lacks equals
eq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))

# -------------------------
# Generate variables
# -------------------------

_HOSTNAME:=$(shell hostname)
MKFILE_PATH:= $(abspath $(lastword $(MAKEFILE_LIST)))
DIR_PATH:= $(patsubst %/,%,$(dir $(MKFILE_PATH)))
CURRENT_DIR:= $(notdir $(patsubst %/,%,$(dir $(MKFILE_PATH))))

_DEPDIRS:=$(IDIRS)
_IDIRS:=$(foreach d, $(IDIRS), -I $d)
_LDIRS:=$(foreach d, $(LDIRS), -L $d)
_C_COMP_ARGS:= $(CFLAGS) $(_IDIRS)
_CXX_COMP_ARGS:= $(CXXFLAGS) $(_IDIRS)
_LINK_ARGS:= $(LIBS) $(_LDIRS) $(LDFLAGS)
_ODIR:=$(ODIR)
_BINDIR:=$(BINDIR)

ifeq ($(strip $(STATICARCHIVE)),)
	_STATICARCHIVE:=lib$(CURRENT_DIR).a
else
	_STATICARCHIVE:=$(STATICARCHIVE)
endif

ifeq ($(strip $(SHAREDARCHIVE)),)
	_SHAREDARCHIVE:=lib$(CURRENT_DIR).so
else
	_SHAREDARCHIVE:=$(SHAREDARCHIVE)
endif

_STATICARCHIVEDIR:=$(dir $(_SHAREDARCHIVE))
_SHAREDARCHIVEDIR:=$(dir $(_STATICARCHIVE))

_IGNORES_FULL_PATHS:=$(foreach x, $(IGNORES), $(patsubst ./%,%,$(x)))
_EXECUTABLE_FULL_PATHS:=$(foreach x, $(EXECUTABLES), $(patsubst ./%,%,$(x)))
_BINS :=$(foreach x,$(_EXECUTABLE_FULL_PATHS),$(notdir $(basename $(x))).bin)
_BINS_MOVED :=$(foreach x,$(_EXECUTABLE_FULL_PATHS),$(_BINDIR)/$(notdir $(basename $(x))).bin)
_DEPS_H :=$(foreach d,$(_DEPDIRS),$(shell find $d/ -iname '*.h' -type f | sed 's/ /\\ /g'))
_DEPS_HPP :=$(foreach d,$(_DEPDIRS),$(shell find $d/ -iname '*.hpp' -type f | sed 's/ /\\ /g'))
_DEPS :=$(_DEPS_H) $(_DEPS_HPP) $(MKFILE_PATH)
_SRCS_C :=$(foreach d, $(SRCDIRS), $(shell find $d/ -iname '*.c' -type f | sed 's/ /\\ /g'))
_SRCS_CPP :=$(foreach d, $(SRCDIRS), $(shell find $d/ -iname '*.cpp' -type f | sed 's/ /\\ /g'))
_SRCS := $(foreach x, $(_SRCS_C) $(_SRCS_CPP), $(patsubst ./%,%,$(x)))
_SRCS_FULL_PATHS :=$(filter-out $(_EXECUTABLE_FULL_PATHS) $(_IGNORES_FULL_PATHS), $(foreach x, $(_SRCS), $(x)))
_OBJECTS :=$(foreach x, $(basename $(_SRCS)), $(x).o)
_OBJECTS_MOVED :=$(foreach x, $(_SRCS_FULL_PATHS), $(_ODIR)/$(call mangle, $(x)).o)
_EXECUTABLE_OBJECTS_MOVED :=$(foreach x, $(_EXECUTABLE_FULL_PATHS), $(_ODIR)/$(call mangle, $(x)).o)


find_mangled_executable=$(filter $(_ODIR)/$(call mangle, $1).o %$(_SEP)$(call mangle, $1).o, $(_EXECUTABLE_OBJECTS_MOVED))

# Build needed directories
createDirs := $(foreach d, $(_ODIR) $(_BINDIR) $(_STATICARCHIVEDIR) $(_SHAREDARCHIVEDIR), $(shell test -d $(d) || mkdir -p $(d)))

# -------------------------
# Print variables
# -------------------------

$(info Dependencies: $(strip ${_DEPS}))
$(info Include Dirs: ${IDIRS})
$(info Lib Dirs: ${LDIRS})
$(info Executable Files: ${_EXECUTABLE_FULL_PATHS})
$(info Nonexecutable Source Files: ${_SRCS_FULL_PATHS})
$(info Targets: ${_BINS_MOVED})
$(info Static Archive File: ${_STATICARCHIVE})
$(info Shared Object File: ${_SHAREDARCHIVE})
$(info Path: ${DIR_PATH})
$(info Host: ${_HOSTNAME})
#$(info Mangled Nonexecutable Object Files: ${_OBJECTS_MOVED})
#$(info Mangled Executable Object Files: ${_EXECUTABLE_OBJECTS_MOVED})
$(info ${\n})


# ------------------------
# Makefile pragmas
# ------------------------

# for sorcery.  just look it up in the manual.
.SECONDEXPANSION: 
# for getting rid of implicit rules, because they make debugging hard
.SUFFIXES: 


# ------------------------
# Build rules for target
# ------------------------

bin: $(_BINS)

$(_BINS): $$(call find_mangled_executable, $$@) $(_OBJECTS_MOVED) $(_DEPS) 
	$(if $(findstring .atl,$@), \
		$(ALD) -o $@ $(_OBJECTS_MOVED) $(call find_mangled_executable, $@) -latlas  $(_LINK_ARGS) ,  \
		$(LD) -o $@ $(_OBJECTS_MOVED) $(call find_mangled_executable, $@) $(_LINK_ARGS) ) 
	$(if $(call eq $(shell readlink -f $@), $(shell readlink -f $(_BINDIR)/$(notdir $@)) ), \
		, \
		mv $@ $(_BINDIR)/$(notdir $@))

# -------------------------
# atlas specific handling
# -------------------------


# Compile c code
%.atl.o: $$(call unmangle, $$@).c $(_DEPS)
	$(ACC) -c  $(call unmangle, $@).c -o $(call unmangle, $@).o $(_C_COMP_ARGS)
	mv $(call unmangle, $@).o $@

# Compile cpp code
%.atl.o: $$(call unmangle, $$@).cpp $(_DEPS)
	$(ACXX) -c $(call unmangle, $@).cpp -o $(call unmangle, $@).o $(_CXX_COMP_ARGS) 
	mv $(call unmangle, $@).o $@

# ------------------------------
# Default build rules for c/c++
# ------------------------------

# Compile c code
%.o: $$(call unmangle, $$@).c $(_DEPS)
	$(CC) -c $(call unmangle, $@).c -o $(call unmangle, $@).o $(_C_COMP_ARGS) 
	mv $(call unmangle, $@).o $@

# Compile cpp code
%.o: $$(call unmangle, $$@).cpp $(_DEPS)
	$(CXX) -c $(call unmangle, $@).cpp -o $(call unmangle, $@).o  $(_CXX_COMP_ARGS) 
	mv $(call unmangle, $@).o $@

# ---------------------
# Library build rules
# ---------------------

# builds a libraries of all 
# source (minus executables)
lib: $(_SHAREDARCHIVE) $(_STATICARCHIVE)
static : $(_STATICARCHIVE)
shared : $(_SHAREDARCHIVE)

$(_STATICARCHIVE): $(_OBJECTS_MOVED) $(_DEPS) 
	ar rcs $(_STATICARCHIVE) $(_OBJECTS_MOVED)

$(_SHAREDARCHIVE): $(_OBJECTS_MOVED) $(_DEPS) 
	gcc -shared -o $(_SHAREDARCHIVE) $(_OBJECTS_MOVED)

# ---------------------
# Auxiliary build rules
# ---------------------

clean:
	rm $(_BINS) $(ODIR)/*.o $(_SHAREDARCHIVE) $(_BINS_MOVED) $(_STATICARCHIVE) Makefile~ 2> /dev/null || true
	$(foreach d, $(SRCDIRS), $(shell rm $d/*.c~ $d/*.cpp~ $d/*.h~ $d/*.hpp~ 2> /dev/null || true) )
	$(foreach d, $(SRCDIRS), $(info rm ${d}/*.c~ ${d}/*.cpp~ ${d}/*.h~ ${d}/*.hpp~ 2> /dev/null || true) )


clean_memory:
	rm -f -r /dev/shm/* || true



























