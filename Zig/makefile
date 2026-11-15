.PHONY: all
all: WinGDITemplatePixels.exe

win.zig: win.h
	zig translate-c -lc -target x86_64-windows-gnu $< > $@

WinGDITemplatePixels.exe: WinGDITemplatePixels.zig win.zig
	zig build-exe -OReleaseFast $< -target x86_64-windows-gnu -lgdi32 -luser32 -lkernel32 --subsystem windows

.PHONY: check
check: WinGDITemplatePixels.check

WinGDITemplatePixels.check: WinGDITemplatePixels.zig win.zig
	zig ast-check $<