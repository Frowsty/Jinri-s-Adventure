build:
	g++ -O2 -Wfatal-errors -std=c++17 \
	./src/demo.cpp \
	-o game.exe \
	-luser32 \
	-lgdi32 \
	-lopengl32 \
	-lgdiplus \
	-lShlwapi \
	-lstdc++fs \
	-static
