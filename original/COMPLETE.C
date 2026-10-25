//
// ************************************************************************
//




//
// ************************************************************************
//

char *data;
VGABlock Text,Map,Graphic[3],Back;


struct PixelPos {
	float dx,dy,x,y;
};


void FillPos(PixelPos &Var,int x,int y,int x2,int y2,float total) {
	// Store stuff
	Var.dx=(x2-x)/total;
	Var.dy=(y2-y)/total;

	Var.x=x;
	Var.y=y;
}



void Do3DDemo(Shape3D Cube) {
	// Variables
	int zoom,xr,yr,zr,i,j;
	char done;

	// Fade background on to screen
	FadeOut(1);
	ClsPCX(Back.data);
	Dump();
	FadeIn(0.02,Back.palette);

	// Get on with it...
	xr=yr=zr=0;
	zoom=500;
	done=0;

	do {
		// Rotate
		xr+=1;
		yr+=1;
		zr+=2;
		Rotate(Cube,xr,yr,zr);

		xr%=360;
		yr%=360;
		zr%=360;

		// Zoom onto screen
		if(zoom) {
			for(i=0;i<Cube.maxpoints;i++) {
				trans.point[i].x*=1-(zoom/500.0);
				trans.point[i].y*=1-(zoom/500.0);
				trans.point[i].z*=1-(zoom/500.0);
			}
			zoom--;
		}

		// Translate to screen
		Translate(Cube);
		ClsPCX(Back.data);

		// Draw
		for(i=0;i<Cube.maxpolys;i++) {
			DotLine(trans.sx[Cube.poly[i].join[0]],trans.sy[Cube.poly[i].join[0]],
					trans.sx[Cube.poly[i].join[1]],trans.sy[Cube.poly[i].join[1]],7,250);
			DotLine(trans.sx[Cube.poly[i].join[1]],trans.sy[Cube.poly[i].join[1]],
					trans.sx[Cube.poly[i].join[2]],trans.sy[Cube.poly[i].join[2]],7,250);
			DotLine(trans.sx[Cube.poly[i].join[2]],trans.sy[Cube.poly[i].join[2]],
					trans.sx[Cube.poly[i].join[3]],trans.sy[Cube.poly[i].join[3]],7,250);
			DotLine(trans.sx[Cube.poly[i].join[3]],trans.sy[Cube.poly[i].join[3]],
					trans.sx[Cube.poly[i].join[0]],trans.sy[Cube.poly[i].join[0]],7,250);
		}


		// Display
		Dump();

		// Finished?
		if(!zoom && !xr && !yr && !zr)
			done=1;

	} while(!done);

	// Define Lines
	PixelPos Pos[8];
	int steps=150;

	FillPos(Pos[0],0,0,110,150,steps);
	FillPos(Pos[1],319,0,110,50,steps);

	FillPos(Pos[2],319,0,110,150,steps);
	FillPos(Pos[3],319,199,210,150,steps);

	FillPos(Pos[4],319,199,210,150,steps);
	FillPos(Pos[5],0,199,210,50,steps);

	FillPos(Pos[6],0,199,110,50,steps);
	FillPos(Pos[7],0,0,210,50,steps);

	// Move lines to cube
	for(i=0;i<steps;i++) {
		// Draw dots
		ClsPCX(Back.data);
		DotLine(110,50,210,50,7,250);
		DotLine(110,150,210,150,7,250);
		DotLine(110,50,110,150,7,250);
		DotLine(210,50,210,150,7,250);

		// Draw
		Line(Pos[0].x,Pos[0].y,Pos[1].x,Pos[1].y,250);
		Line(Pos[2].x,Pos[2].y,Pos[3].x,Pos[3].y,250);
		Line(Pos[4].x,Pos[4].y,Pos[5].x,Pos[5].y,250);
		Line(Pos[6].x,Pos[6].y,Pos[7].x,Pos[7].y,250);

		Dump();
		Waitvlb();

		// Move points
		for(j=0;j<8;j++) {
			Pos[j].x+=Pos[j].dx;
			Pos[j].y+=Pos[j].dy;
		}
	}

	// Fix lines properly
	ClsPCX(Back.data);
	Line(110,50,210,50,250);
	Line(110,150,210,150,250);
	Line(110,50,110,150,250);
	Line(210,50,210,150,250);
	Dump();
	Waitvlb();

	Delay(10);

	// Rotate wireframe cube
	zoom=-1;
	do {
		// Rotate
		xr+=1;
		yr+=1;
		zr+=2;
		Rotate(Cube,xr,yr,zr);

		xr%=360;
		yr%=360;
		zr%=360;

		// Zoom onto screen
		if(zoom>=0) {
			for(i=0;i<Cube.maxpoints;i++) {
				trans.point[i].x*=zoom/300.0;
				trans.point[i].y*=zoom/300.0;
				trans.point[i].z*=zoom/300.0;
			}
			zoom--;
		}

		// Translate to screen
		Translate(Cube);
		ClsPCX(Back.data);

		// Draw
		for(i=0;i<Cube.maxpolys;i++) {
			Line(trans.sx[Cube.poly[i].join[0]],trans.sy[Cube.poly[i].join[0]],
				 trans.sx[Cube.poly[i].join[1]],trans.sy[Cube.poly[i].join[1]],250);
			Line(trans.sx[Cube.poly[i].join[1]],trans.sy[Cube.poly[i].join[1]],
				 trans.sx[Cube.poly[i].join[2]],trans.sy[Cube.poly[i].join[2]],250);
			Line(trans.sx[Cube.poly[i].join[2]],trans.sy[Cube.poly[i].join[2]],
				 trans.sx[Cube.poly[i].join[3]],trans.sy[Cube.poly[i].join[3]],250);
			Line(trans.sx[Cube.poly[i].join[3]],trans.sy[Cube.poly[i].join[3]],
				 trans.sx[Cube.poly[i].join[0]],trans.sy[Cube.poly[i].join[0]],250);
		}


		// Display
		Dump();

		// Finished?
		if(zoom==-1 && !xr && !yr && !zr)
			zoom=300;
	} while(zoom);

	Delay(10);

	// Texture mapped cube
	SetTextureMap(Map.data,127);

	// Texture map the cube
	xr=yr=zr=0;
	zoom=490;
	done=0;
	do {
		// Rotate
		xr+=2;
		yr+=3;
		Rotate(Cube,xr,yr,0);

		xr%=360;
		yr%=360;

		// Zoom onto screen
		if(zoom) {
			for(i=0;i<Cube.maxpoints;i++) {
				trans.point[i].x*=1-(zoom/500.0);
				trans.point[i].y*=1-(zoom/500.0);
				trans.point[i].z*=1-(zoom/500.0);
			}
			zoom--;
		}

		// Translate to screen
		Translate(Cube);
		ClsPCX(Back.data);

		// Draw
		for(i=0;i<Cube.maxpolys;i++) {
			if(Visible(Cube,i)) {
			TextureMapPoly(trans.sx[Cube.poly[i].join[0]],trans.sy[Cube.poly[i].join[0]],
						   trans.sx[Cube.poly[i].join[1]],trans.sy[Cube.poly[i].join[1]],
						   trans.sx[Cube.poly[i].join[2]],trans.sy[Cube.poly[i].join[2]],
						   trans.sx[Cube.poly[i].join[3]],trans.sy[Cube.poly[i].join[3]]);
			}
		}

		// Display
		Dump();

		// Finished?
		if(!zoom && !xr && !yr)
			done=1;

	} while(!done);


	// Fade Texture mapped cube to black
	do {
		// Rotate
		xr+=2;
		yr+=3;
		Rotate(Cube,xr,yr,0);

		xr%=360;
		yr%=360;

		// Fade Palette
		done=1;
		for(i=0;i<768;i++)
			if(Back.palette[i]) {
				Back.palette[i]--;
				done=0;
			}
		SetPalette(Back.palette);
		done--;

		// Translate to screen
		Translate(Cube);
		ClsPCX(Back.data);

		// Draw
		for(i=0;i<Cube.maxpolys;i++) {
			if(Visible(Cube,i)) {
			TextureMapPoly(trans.sx[Cube.poly[i].join[0]],trans.sy[Cube.poly[i].join[0]],
						   trans.sx[Cube.poly[i].join[1]],trans.sy[Cube.poly[i].join[1]],
						   trans.sx[Cube.poly[i].join[2]],trans.sy[Cube.poly[i].join[2]],
						   trans.sx[Cube.poly[i].join[3]],trans.sy[Cube.poly[i].join[3]]);
			}
		}

		// Display
		Dump();
	} while(done);

	// Return
	BlockDealloc(Back);
	BlockDealloc(Map);
}



void Demo3D() {
	// Variables
	Shape3D Cube;

	// Set video stuff
	if(Mode!='V')
		ModeVGA();
	FadeOut(1);
	Cls(0);
	CreatePal();

	// Allocate memory for 3D
	trans.point=new Point3D[8];
	trans.sx=new int[8];
	trans.sy=new int[8];
	Cube.point=new Point3D[8];
	Cube.poly=new Poly3D[6];

	// Create 3D block
	Cube.point[0].x=-50;
	Cube.point[0].y=50;
	Cube.point[0].z=-50;
	Cube.point[1].x=50;
	Cube.point[1].y=50;
	Cube.point[1].z=-50;
	Cube.point[2].x=50;
	Cube.point[2].y=-50;
	Cube.point[2].z=-50;
	Cube.point[3].x=-50;
	Cube.point[3].y=-50;
	Cube.point[3].z=-50;

	Cube.point[4].x=-50;
	Cube.point[4].y=50;
	Cube.point[4].z=50;
	Cube.point[5].x=50;
	Cube.point[5].y=50;
	Cube.point[5].z=50;
	Cube.point[6].x=50;
	Cube.point[6].y=-50;
	Cube.point[6].z=50;
	Cube.point[7].x=-50;
	Cube.point[7].y=-50;
	Cube.point[7].z=50;

	Cube.poly[0].join[0]=0;
	Cube.poly[0].join[1]=1;
	Cube.poly[0].join[2]=2;
	Cube.poly[0].join[3]=3;
	Cube.poly[0].color=Teal+12;

	Cube.poly[1].join[0]=1;
	Cube.poly[1].join[1]=5;
	Cube.poly[1].join[2]=6;
	Cube.poly[1].join[3]=2;
	Cube.poly[1].color=Teal+10;

	Cube.poly[2].join[0]=5;
	Cube.poly[2].join[1]=4;
	Cube.poly[2].join[2]=7;
	Cube.poly[2].join[3]=6;
	Cube.poly[2].color=Teal+8;

	Cube.poly[3].join[0]=4;
	Cube.poly[3].join[1]=0;
	Cube.poly[3].join[2]=3;
	Cube.poly[3].join[3]=7;
	Cube.poly[3].color=Teal+10;

	Cube.poly[4].join[0]=0;
	Cube.poly[4].join[1]=4;
	Cube.poly[4].join[2]=5;
	Cube.poly[4].join[3]=1;
	Cube.poly[4].color=Teal+6;

	Cube.poly[5].join[0]=2;
	Cube.poly[5].join[1]=6;
	Cube.poly[5].join[2]=7;
	Cube.poly[5].join[3]=3;
	Cube.poly[5].color=Teal+6;

	Cube.maxpoints=8;
	Cube.maxpolys=6;

	// Spin Shape with effects
	Do3DDemo(Cube);
}



void Intro() {
	// Variables
	int i;

	// Initialise screen mode
	ModeVGA();
	FadeOut(1);

	// Display messages
	for(i=0;i<3;i++) {
		// Display graphic on screen centre
		Delay(10);
		Cls(0);
		BlockPut(Graphic[i],(320-Graphic[i].width)/2,(scrnheight-Graphic[i].height)/2);
		Dump();
		FadeIn(.03,Graphic[i].palette);

		// Wait a bit, before fading out
		Delay((i<2)?20:40);

		FadeOut((i<2)?0.03:0.01);

		// Free memory
		BlockDealloc(Graphic[i]);
	}

	Delay(5);
	Cls(0);
	Dump();
}



void Scroller() {
	// Variables
	int DataMark,LeftLine,i,j,MidY,x,y,height,temp;
	unsigned io,Dseg,Doff,DestSeg,DestOff;
	int ssin[360];
	char Def;

	// Set Mode
	ModeVGA();
	Waitvlb();

	// Load Text
	SetPalette(Text.palette);

	// Init
	LeftLine=-320;
	MidY=(190-Text.width)/2;
	height=Text.width;

	Dseg=FP_SEG(Text.data);
	io=FP_OFF(Text.data);

	DestSeg=FP_SEG(vscreen);
	DestOff=FP_OFF(vscreen);

	// Make lookup table
	for(i=0;i<360;i++)
		ssin[i]=MidY-15*sin(rad(i));

	// Main thing
	do {
		// Clear
		Cls(0);

		// Draw text
		x=0;
		Def=0;
		for(i=LeftLine;i<LeftLine+319;i++) {
			if(i>=0 && i<Text.height) {
				// Re-adjust offsets for scroll
				if(!Def) {
					Doff=io+i*height;
					Def=1;
				}

				// Create wave
				temp=((LeftLine>>1)+x)%360;
				y=pplookup[ssin[temp]];

				// Draw vertical line
				asm {
					// Set source
					push ds

					mov ds,Dseg
					mov si,Doff

					// Set dest
					mov es,DestSeg
					mov di,DestOff
					add di,y
					add di,x

					// Do line
					mov cx,height
					shr cx,3

					mov dx,319
				}
				loop1:
				asm {
					movsb;add di,dx
					movsb;add di,dx
					movsb;add di,dx
					movsb;add di,dx
					movsb;add di,dx
					movsb;add di,dx
					movsb;add di,dx
					movsb;add di,dx
					loop loop1

					// Update pointer
					mov Doff,si

					pop ds
				}
			}
			x++;
		}

		LeftLine+=2;

		// Display
		Waitvlb();
		Dump();

	} while(LeftLine-2<Text.height);

	// Return
	BlockDealloc(Text);
}



void Plasma() {
	// Variables
	char *data;
	char r=0,g=0,b=0,pal[768];
	int pl1,pl2,x,y;
	float teller;

	// Allocate memory
	data=(char *) farmalloc(65800l);
	if(!data)
		Error("Error allocating memory for plasma screen");

	// Create table
	for(x=0; x<256; x++)
		for(y=0; y<256; y++)
			data[x+256*y]=83*(3+(cos(0.09*sqrt((x-128)*(x-128)+(y-128)*(y-128.0))))+
						  cos(x/21.0)+cos(y/26.0));

	// Set graphics
	if(Mode!='V')
		ModeVGA();

	// Set palette
	for(x=0;x<64;x++) {
		pal[3*x]=r;
		pal[1+3*x]=g;
		pal[2+3*x]=b;
		r++;
	}
	for(x=64;x<128;x++) {
		r--;
		pal[3*x]=r;
		pal[1+3*x]=g;
		pal[2+3*x]=b;
	}
	for(x=128;x<192;x++) {
		pal[3*x]=r;
		pal[1+3*x]=g;
		pal[2+3*x]=b;
		r++;
		g++;
	}
	for(x=192;x<256;x++) {
		r--;
		g--;
		pal[3*x]=r;
		pal[1+3*x]=g;
		pal[2+3*x]=b;
	}
	FadeOut(1);

	// Do plasma
	for(teller=0;teller<900; teller++) {
		pl1=48+cos(teller/37)*47+256*(int)(48+47*(sin(teller/31)));
		pl2=48+sin(teller/24)*47+256*(int)(48+47*(cos(teller/19)))-pl1;

		// Set Destination
		_ES=0xa000;
		asm xor di,di

		// Set source
		asm push ds
		asm lds si,data
		asm add si,pl1
		asm mov bx,pl2
		asm pusha
		asm mov cl,100 					// Half height

		// Dunno
	l1: asm mov dx,160
	l2: asm lodsb
		asm add al,[si+bx]
		asm mov ah,al
		asm mov es:[di+320],ax
		asm stosw
		asm dec dx
		asm jnz l2
		asm add si,96
		asm add di,320
		asm dec cl
		asm jnz l1
		asm popa
		asm pop ds

		// Fade to palette if just appeared
		if(!teller)
			FadeIn(0.02,pal);

		Waitvlb();
	}
	farfree(data);

	// Melt screen
	WipeScreen(0);

	for(y=0;y<200;y+=2) {
		x=pplookup[y];

		asm {
			// Set Dest
			mov ax,0xa000
			mov es,ax
			mov di,x

			// Draw line
			mov cx,160
			mov ax,0

			rep stosw
		}
		Waitvlb();

		if(y==198)
			y=-1;
	}
}