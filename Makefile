all: build

build:
	cc @conanbuildinfo.args -Wall -Wextra -Werror rsalt.c -o rsalt

linux:
	cc @conanbuildinfo.args -Wall -Wextra -Werror rsalt.c -o rsalt

clean:
	rm rsalt

install:
	cp -a rsalt /usr/bin/rsalt
