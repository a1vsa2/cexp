#SHELL = cmd   D:/dev_tools/w64devkit-1.21.0/bin/sh.exe
ifneq (,)
This makefile requires GNU Make.
endif

#######################################
# for submakefile
ProjectDir ?=
SCRDIR_PREFIX ?=
SRCDIRS ?=

#INCLUDE_PATH ?= -I../../includes -I../include
#INCLUDE_PATH += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/win32

DLLS ?= -lws2_32
OUTNAME ?=out
#############################################
subdirs =src

CC=gcc
#CPPFLAGS += -MMD -MP	# preprocessor option
CFLAGS = -g -Winline # c compiler flags
#CXXFLAGS = -Winline  # c++ compiler flags
CONTROLFLAGS= -fno-default-inline
# COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c


BUILDDIR =$(ProjectDir)/$(OUTNAME)
# deferred assignment
SOURCES ?= $(patsubst ./%,%,$(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c)))
#$(info $(SOURCES))
#OBJDIRS ?= $(subst /,\,$(addprefix $(BUILDDIR)\$(SCRDIR_PREFIX)\,$(SRCDIRS))) ## cmd del path
OBJDIRS ?= $(patsubst %/.,%,$(addprefix $(BUILDDIR)/$(SCRDIR_PREFIX),$(SRCDIRS)))
OBJS = $(notdir $(SOURCES:.c=.o))
EXENAME ?= default

vpath
vpath %.c $(SRCDIRS)
vpath %.o $(OBJDIRS)
vpath %.d $(OBJDIRS)
# vpath %.dll $(OBJDIRS)

.DEFAULT_GOAL=allObj

allObj: $(OBJS) 

#buildTarget:$(notdir $(CURDIR)).dll

# immediate : immediate ; deferred
# deferred

ifeq "" "$(filter clean%,$(MAKECMDGOALS))"
include $(foreach dep, $(SOURCES:.c=.d), $(BUILDDIR)/$(SCRDIR_PREFIX)$(dep))
endif


%.o: %.c | $(OBJDIRS)
	@echo building object $@
	$(COMPILE.c) $(INCLUDE_PATH) -o $(BUILDDIR)/$(SCRDIR_PREFIX)$(subst .c,.o,$<) $<

$(BUILDDIR)/$(SCRDIR_PREFIX)%.d:%.c | $(OBJDIRS)
	@echo generating dependency file $@  $<
	@set -e; rm -f $@; \
	$(CC) $(INCLUDE_PATH) -MM  $< > $(BUILDDIR)/tmp.$$$$; \
	sed 's,\($$*\)\.o[: ]*,\1.o $(notdir $@): ,' $(BUILDDIR)/tmp.$$$$ > \
	$(BUILDDIR)/$(SCRDIR_PREFIX)$(subst .c,.d,$<); rm -f $(BUILDDIR)/tmp.$$$$
#	$(CC) $(INCLUDE_PATH) -MM $< > $(BUILDDIR)/$(SCRDIR_PREFIX)$(subst .c,.d,$<)
#	(@echo generate dependency file failed; @del $@)


%.dll:$(OBJS)
	@echo building shared library $@
	$(CC) -shared -o0 -o $(BUILDDIR)/$@  $(OBJS) $(DLLS)

$(OBJDIRS):
	@echo creating dir: $@
	mkdir -p $@

define gen_exe
	@echo creating executeable from $1
	$(CC) -o $(BUILDDIR)/$(EXENAME).exe $1 $(DLLS)
endef

run:
ifneq "" "$(EXENAME)"
	$(BUILDDIR)/$(EXENAME).exe
endif

exe:$(ts)
ifneq "" "$(ts)"
	@echo generating executeable
	$(call gen_exe, $<)

endif

.ONESHELL:
clean_obj:
	for dir in $(OBJDIRS) 
	do 
		if [ -e $$dir ]
		then
			echo clear directory $$dir
			rm -rf $$dir
		fi
	done

.ONESHELL:
clean:
	if [ -e $(BUILDDIR) ]
	then
		echo clear build directory $(BUILDDIR)
		rm -rf $(BUILDDIR)
	fi


#  ===== CMD
# .ONESHELL:
# clean_obj:
# 	for %%i in ($(OBJDIRS)) do (
# 		if exist %%i (
# 			echo clear directory: %%i
# 			rmdir -rf %%i
# 			del /Q %%i\\*.*
# 		)
# 	) 

# .ONESHELL:
# clean_all:
# 	if exist $(BUILDDIR) (
# 		@rmdir /Q /S  $(BUILDDIR)
# 		@echo output directory deleted
# 	)

#private subdirs=$(shell dir /AD /B /S src)

$(info $(wildcard src/*.c))
submfs += $(foreach subdir,$(subdirs),$(wildcard $(subdir)/*.mk) $(wildcard $(subdir)/makefile))
submakes:
# 	@for %%i in ($(submfs)) do make -C %%~dpi -f %%~nxi
	@for subdir in $(submfs) ; do echo $$subdir; done


.PHONY: clean_obj clean

.SILENT:clean_obj clean


#.NOTPARALLEL: notparallel  ## MS-DOS doesn't support multi-processing.