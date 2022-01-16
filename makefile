CC=gcc -std=c11
CXX=g++ -std=c++11
SHELL=cmd
CFLAGS=-Wall -Wextra -Wpedantic
CFLAGSREL=$(CFLAGS) -O3 -fno-ident -Wl,--strip-all,--build-id=none,--gc-sections -mwindows
CFLAGSDEB=$(CFLAGS) -g
LIB=-lgdi32 -lcomctl32 -lole32 -luuid

OBJ=obj
OBJD=objd
TARGET=NSSearch

default: debug

$(OBJ):
	mkdir $(OBJ)
$(OBJD):
	mkdir $(OBJD)

$(OBJ)/resources.o: src/resources.rc src/resources.h
	windres -i $< $@ -D NDEBUG
$(OBJ)/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGSREL)
$(OBJ)/%.o: src/%.cpp
	$(CXX) -c $< -o $@ $(CFLAGSREL)

$(OBJD)/resources.o: src/resources.rc src/resources.h
	windres -i $< $@ -D _DEBUG
$(OBJD)/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGSDEB)
$(OBJD)/%.o: src/%.cpp
	$(CXX) -c $< -o $@ $(CFLAGSDEB)


release: $(OBJ) release_link
release_link: $(OBJ)/main.o $(OBJ)/common.o $(OBJ)/resources.o $(OBJ)/winproc.o $(OBJ)/search.o $(OBJ)/com_components.o
	$(CC) $^ -o $(TARGET).exe $(CFLAGSREL) $(LIB)

debug: $(OBJD) debug_link
debug_link: $(OBJD)/main.o $(OBJD)/common.o $(OBJD)/resources.o $(OBJD)/winproc.o $(OBJD)/search.o $(OBJD)/com_components.o
	$(CC) $^ -o $(TARGET)_deb.exe $(CFLAGSDEB) $(LIB)

clean:
	del *.exe
	IF EXIST $(OBJ) rd /s /q $(OBJ)
	IF EXIST $(OBJD) rd /s /q $(OBJD)