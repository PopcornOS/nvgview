CC=gcc

ifeq ($(OS),Windows_NT)
EXE=.exe
else
EXE=
endif

ifeq ($(CC),cl)
define pop-c
	@# MSVC path: compile to OBJ, then convert to raw binary
	@# Warning: does not play nice with non-inline functions.
	cl /nologo /GS- /Zi /W3 /Od /D UNICODE /D _UNICODE /c $(1) /Fo$(1).obj
	link.exe /NOLOGO /NODEFAULTLIB /ENTRY:pop_main /SUBSYSTEM:NATIVE /OUT:$(1).exe $(1).obj
	objcopy -O binary $(1).exe $(2)
	rm $(1).obj $(1).exe
endef
else
define pop-c
	@# GCC/Clang path: freestanding compile + LD to raw binary
	$(CC) -ffreestanding -fno-stack-protector -nostdlib \
		   -fno-asynchronous-unwind-tables -fshort-wchar \
		   -mno-red-zone -c $(1) -o $(1).o
	ld -nostdlib -T pop.ld $(1).o -o $(1).tmp.$(EXE)
	objcopy -O binary $(1).tmp.$(EXE) $(2)
	rm $(1).o $(1).tmp.$(EXE)
endef
endif

all:
	$(call pop-c,main.c,nvgview.bin)