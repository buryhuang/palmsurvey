VERSION		=	$(shell if [ -f version ]; then cat version; else echo 0.0.1; fi)
LANG			=	En
#LANG			= Ge
#LANG			= Fr
#LANG			= It
OPT_COMP_FLAGS = -DERROR_CHECK_LEVEL="ERROR_CHECK_FULL"
#OPT_COMP_FLAGS = -DERROR_CHECK_LEVEL="ERROR_CHECK_NONE"


APP 			=	Survey$(LANG)
APPID 		=	NRSY
ICONTEXT	=	"Survey$(LANG)"
RCP 			=	$(APP).rcp
RCP_IN		=	$(RCP).in
PRC 			=	$(APP).prc
DEF 			=	$(APP).def
INSTALLTOOL = pilot-xfer
INSTALLFLAGS = -i

SRC 			=	Projects.c \
						DefaultCategoriesForm.c \
						ProjectsDB.c \
						ExtAboutDlg.c \
						MainForm.c \
						ProjectForm.c \
						NoteForm.c \
						MemoLookup.c \
						DateDB.c \
						AllToDosDlg.c \
						HndrFuncs.c

CONFIG_H	=	config.h
SECTIONS 	=	$(APP)-sections
CC 				=	m68k-palmos-gcc
CPP				=	cpp
PILRC 		=	pilrc
BUILDPRC 	=	build-prc
MULTIGEN 	=	m68k-palmos-multigen

#Uncomment this if you want to build a GDB-debuggable version
#CFLAGS =-O2 -g
CFLAGS = -O2 -Wall -pipe

all:$(PRC)

$(PRC) : $(APP) bin.stamp
	$(BUILDPRC) $(DEF) $(ICONTEXT) *.bin -o $(PRC)
	ls -l *.prc
	sed -n -e '/^#define/p' $(CONFIG_H)

$(APP) : $(SRC:.c=.o) $(SECTIONS).o $(SECTIONS).ld
	$(CC) $(CFLAGS) -o $@ $^

bin.stamp : $(RCP_IN) version
	$(CPP) $(RCP_IN) -o $(RCP)~ -P
	sed 's/##VERSION##/$(VERSION)/g' < $(RCP)~ > $(RCP)
	$(PILRC) -allowEditID -q $(RCP)
	touch $@

$(RCP_IN) : $(CONFIG_H)
	touch $@

%.o : %.c $(CONFIG_H) Projects.h
	$(CC) $(CFLAGS) -c $(OPT_COMP_FLAGS) $< -o $@

#		touch $<
#Enable the previous line if you want to compile EVERY time.

$(SECTIONS).o : $(SECTIONS).s
	$(CC) $(CFLAGS) -c $< -o $@

$(SECTIONS).s $(SECTIONS).ld : $(DEF)
	$(MULTIGEN) $(DEF)

depend dep:
	$(CC)-M $(SRC) > .dependencies

clean:
	rm -rf *.o $(APP) *.rcp *.bin *.stamp *.s *.ld

distclean:clean
	rm -rf *.prc *.rcp *.bak *~ tags

resclean:
	rm -rf *.bin bin.stamp

res:resclean
	make

force : clean
	make

install : $(APP)
	$(INSTALLTOOL) $(INSTALLFLAGS) $(APP).prc
