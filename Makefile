all: build

build:
	cc rsalt.c -ljansson -lcurl -Wall -Wextra -Werror -o rsalt

clean:
	rm rsalt
