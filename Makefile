libs_args = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf
c_files = $(wildcard src/*.c)
o_files = $(patsubst src/%.c,obj/%.o, $(c_files))

obj/%.o: src/%.c
	gcc -c $< -o $@ -Wall -Werror -Iincludes 

all: $(o_files)
	gcc -o app/app.exe $(o_files) -Llib $(libs_args) 