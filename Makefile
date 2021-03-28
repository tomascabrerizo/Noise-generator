
INCLUDE_PATH="D:\thirdparty\mingw32\include"
LIB_PATH="D:\thirdparty\mingw32\lib"

main:
	g++ main.cpp -o main.exe -I$(INCLUDE_PATH) -L$(LIB_PATH) -lraylib -lopengl32 -lgdi32 -lwinmm 
run:
	.\main.exe
