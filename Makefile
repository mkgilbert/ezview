PROG=ezview
FILES=ezview.c ppmrw.c
FLAGS=-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lglfw3

all: ; gcc $(FLAGS) $(FILES) -o $(PROG)

clean: ; rm $(PROG)