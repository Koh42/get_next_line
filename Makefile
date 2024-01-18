test:
	@-norminette -v && norminette
	@if [ ! -d gnlTester/ ]; then git clone --depth 1 https://github.com/Tripouille/gnlTester.git; fi
	@if [ -f /sbin/apk ]; then sed -i 's/throw()//' ./gnlTester/utils/leaks.cpp; apk add g++ clang ncurses; fi
	sed -i 's/get_next_line.h/get_next_line_bonus.h/' ./gnlTester/utils/gnl.cpp ./gnlTester/tests/mandatory.cpp
	sed -i 's/..\/get_next_line_utils_bonus.c//' ./gnlTester/Makefile
	make -C gnlTester/ b
