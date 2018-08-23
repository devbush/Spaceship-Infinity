.PHONY: all archive clean distclean

NAME ?= $(shell basename $(shell pwd))
LDLIBS ?= -lm -lncursesw
CPPFLAGS ?= -g -I include
CFLAGS ?= -O3 -march=native -g3 -ggdb
override CFLAGS += -std=gnu11 -pedantic -pedantic-errors \
		-Wall -Wextra \
		-Wdouble-promotion -Wformat=2 -Winit-self -Wswitch-default \
		-Wswitch-enum -Wunused-parameter -Wuninitialized -Wfloat-equal \
		-Wshadow -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align \
		-Wwrite-strings -Wconversion -Wstrict-prototypes \
		-Wold-style-definition -Wmissing-prototypes -Wmissing-declarations \
		-Wredundant-decls -Wnested-externs
# D'autres warnings intéressants (en général, certains sont inutiles dans ce
# cas particulier) mais pas encore reconnus par la version de GCC disponible
# sur une Ubuntu 14.04... :
#
# -Wmissing-include-dirs -Wnull-dereference -Wswitch-bool -Wduplicated-cond
# -Wdate-time

SRC = src/*.c
HEADER = include/*.h

EXEC = spaceship-infinity
all: $(EXEC)
spaceship-infinity: obj/spaceship-infinity.o obj/options.o obj/game.o obj/column_list.o obj/terrain.o obj/ui.o obj/column.o obj/point_list.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LOADLIBES) $(LDLIBS)

# Archive
archive:
	tar -czf $(NAME).tar.gz --transform="s,^,$(NAME)/," $(SRC) $(HEADER) Makefile

# Nettoyage
clean:
	$(RM) -r $(EXEC) obj/*.o vgcore.*
distclean: clean
	$(RM) *.tar.gz

obj/%.o: src/%.c | obj
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Création du dossier obj/ s'il n'existe pas
obj:
	mkdir $@

# Dépendances avec les en-têtes
#~ obj/spaceship-infinity.o: src/spaceship-infinity.c game.h point.h point_list.h \
#~ 	terrain.h column.h cell.h column_list.h options.h ui.h
#~ obj/ui.o: ui.c ui.h game.h point.h point_list.h terrain.h column.h cell.h \
#~ 	column_list.h options.h
#~ obj/game.o: game.c game.h point.h point_list.h terrain.h column.h cell.h \
#~ 	column_list.h options.h
#~ obj/terrain.o: terrain.c terrain.h point.h column.h cell.h column_list.h
#~ obj/column_list.o: column_list.c column_list.h column.h cell.h
#~ obj/column.o: column.c column.h cell.h
#~ obj/options.o: options.c options.h
#~ obj/point_list.o: point_list.c point_list.h point.h
