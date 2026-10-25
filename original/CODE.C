#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <time.h>

#include <colors.c>
#include <glib.c>
#include <asm3d.c>

#include "complete.c"



void main()
{
	// Allocate memory segments
	if(!BlockLoad("data.000",Graphic[0])) {
		Error("Error - Unable to open <data.000>");
	}
	if(!BlockLoad("data.001",Graphic[1])) {
		BlockDealloc(Graphic[0]);
		Error("Error - Unable to open <data.001>");
	}
	if(!BlockLoad("data.002",Graphic[2])) {
		BlockDealloc(Graphic[0]);
		BlockDealloc(Graphic[1]);
		Error("Error - Unable to open <data.002>");
	}
	if(!BlockLoad("data.003",Back)) {
		BlockDealloc(Graphic[0]);
		BlockDealloc(Graphic[1]);
		BlockDealloc(Graphic[2]);
		Error("Error - Unable to open <data.002>");
	}
	if(!BlockLoad("data.004",Map)) {
		BlockDealloc(Graphic[0]);
		BlockDealloc(Graphic[1]);
		BlockDealloc(Graphic[2]);
		Error("Error - Unable to open <data.004>");
	}
	if(!BlockLoad("data.005",Text)) {
		BlockDealloc(Graphic[0]);
		BlockDealloc(Graphic[1]);
		BlockDealloc(Graphic[2]);
		BlockDealloc(Map);
		Error("Error - Unable to open <data.005>");
	}

	// Play music

	// Intro
	Intro();

	// 3D dot, wire, texturemap
	Demo3D();

	// Scrolly message
	Scroller();

	// Plasma
	Plasma();

	// Done
	TextMode();
}