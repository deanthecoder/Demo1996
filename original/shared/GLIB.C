#include <glib.h>


#define SGN(x)		(x==0)?0:(x>0)?1:-1
#define VGA256		256
#define FP_SEG( fp )( (unsigned )( void _seg * )( void far * )( fp ))
#define FP_OFF( fp )( (unsigned )( fp ))


// header info
extern "C" {
void _Cdecl exit(int __status);
int _Cdecl abs(int __x);
}


// Globals
typedef unsigned char far * VGA;

VGA vscreen;
unsigned sscreen,oscreen;
unsigned actStart,scrnheight;
char Page=0,Mode='T';
unsigned pplookup[400];

struct VGABlock {
	int width,height;
	unsigned long size;
	char opaque;
	unsigned char *palette;
	unsigned char *data;
};

struct {
	int t,l,b,r;
} Clip;




/**************************************************************************/
//
// Functions
//
/**************************************************************************/



void Cls(char color);
void TextMode(void);



void Waitvlb(void) {
	asm mov dx,3DAh
	l1:
	asm	in al,dx
	asm	and al,08h
	asm	jnz l1
	l2:
	asm	in al,dx
	asm	and al,08h
	asm	jz  l2
}



void Error(char *string) {
	TextMode();
	puts(string);
	exit(0);
}



void PutPixelX(int x,int y,char color) {
	_DI=pplookup[y];

	asm {
		push ds
		mov ax,x                   // Move X-value into ax
		mov cx,ax                  // And in cx
		shr ax,2                   // Divide ax by 4  (x div 4)
		add di,ax                  // Y:=Y+X   bx contains final location
		add di,actStart

		and cx,3             	   // cl:=0,1,2 or 3  (x mod 4)
		mov ah,1             	   // Start value (plane 0)
		shl ah,cl                  // Eg plane 2: 00000001 => 00000100
		mov dx,3c4h                // Sequencer Register
		mov al,2                   // Select index 2 => plane enable
		out dx,ax                  // Enable selected plane
		mov ax,0xa000              // VGA segment
		mov es,ax                  // es points to VGA
		mov al,color               // Color in al
		stosb
		pop ds
	}
}



void PutPixel(int x,int y,char color) {
	if(Mode!='V')
		return;

	if(x<0 || x>319 || y<0 || y>199)
		return;

	unsigned pos=pplookup[y]+x+FP_OFF(vscreen),sg=FP_SEG(vscreen);
	asm {
		mov es,sg
		mov di,pos

		mov al,color
		stosb
	}
}




void ModeVGA(void) {
	/* initialize graphics mode */
	vscreen=(char far *) farmalloc(64000);

	if(!vscreen) {
		puts("Error allocating screen memory.");
		exit(0);
	}

	sscreen=FP_SEG(vscreen);
	oscreen=FP_OFF(vscreen);

	asm mov ax,0x0013
	asm int 0x10

	Cls(0);

	// Set height
	scrnheight=200;

	for(int i=0;i<scrnheight;i++)
		pplookup[i]=320*i;
	Clip.l=0;
	Clip.t=0;
	Clip.r=319;
	Clip.b=scrnheight-1;
	Mode='V';
}



void SetupScreen() {
	ModeVGA();
}



#define ModeX200()	ModeX()
void ModeX(void) {
	// Set 320x200x256 unchained
	actStart=0;
	scrnheight=200;
	asm {
		mov ax,0x0013
		int 0x10

		// Set chain-4 off
		mov dx,0x3C4
		mov ax,0x604
		out dx,ax

		// Clear video display
		mov ax,0xF02
		out dx,ax
		mov dx,0xA000
		mov es,dx
		xor di,di
		xor ax,ax
		mov cx,0x8000
		rep stosw

		// Turn off CRTC's long mode
		mov dx,0x3D4
		mov ax,0x014
		out dx,ax

		// Turn on the CRTC's BYTE mode
		mov ax,0xE317
		out dx,ax
	}

	Waitvlb();

	for(int i=0;i<scrnheight;i++)
		pplookup[i]=80*i;
	Clip.l=0;
	Clip.t=0;
	Clip.r=319;
	Clip.b=scrnheight-1;
	Mode='X';
}



void ModeX400(void) {
	// (Complicated Pictogram)
	ModeX();

	// Double resolution to 320x400x256 (Urinating Dog, urinating dog)
	asm {
		mov dx,0x3d4
		mov ax,0x4009
		out dx,ax
	}

	Waitvlb();
	scrnheight=400;
	for(int i=0;i<scrnheight;i++)
		pplookup[i]=80*i;
	Clip.b=scrnheight-1;
}




void ModeX240(void) {
	// (Complicated Pictogram)
	ModeX();

	// Set resolution to 320x240x256 (Urinating Dog, urinating dog)
	asm {
		mov dx,0x3c2
		mov al,0xe3
		out dx,al


		mov dx,0x3d4

		mov ax,0x2c11
		out dx,ax
		mov ax,0x0d06
		out dx,ax
		mov ax,0x3e07
		out dx,ax
		mov ax,0xea10
		out dx,ax
		mov ax,0xac11
		out dx,ax
		mov ax,0xdf12
		out dx,ax
		mov ax,0xe715
		out dx,ax
		mov ax,0x0616
		out dx,ax
	}

	Waitvlb();
	scrnheight=240;
	for(int i=0;i<scrnheight;i++)
		pplookup[i]=80*i;
	Clip.b=scrnheight-1;
}



void TextMode(void) {
	asm {
		mov ax,0x0003
		int 0x10
	}
	while(kbhit()) getch();

	if(Mode=='V')
		farfree(vscreen);

	Mode='T';
}



void DestroyScreen() {
	TextMode();
}



void SetActive(int page) {
	actStart=(page*80*scrnheight);
}



void SetVisual(int page) {
	unsigned num=(page*80*scrnheight);

	asm {
		mov dx,0x3d4
		mov al,0x0c
		out dx,al

		inc dx
		mov ax,num
		shr ax,8
		out dx,al

		dec dx
		mov al,0x0d
		out dx,al

		inc dx
		mov ax,num
		out dx,al
	}
}



int GetPCXWidth(char *srce) {
	// Varialbes
	FILE *ptr;
	int ret;

	// Open file
	ptr=fopen(srce,"rb");

	if(!ptr)
		Error("Error - Can't open PCX file for width find");

	// Read value
	fseek(ptr,8,0);
	fread(&ret,2,1,ptr);
	fclose(ptr);

	// Return value
	return(ret+1);
}



int GetPCXHeight(char *srce) {
	// Varialbes
	FILE *ptr;
	int ret;

	// Open file
	ptr=fopen(srce,"rb");

	if(!ptr)
		Error("Error - Can't open PCX file for height find");

	// Read value
	fseek(ptr,10,0);
	fread(&ret,2,1,ptr);
	fclose(ptr);

	// Return value
	return(ret+1);
}



char GetPCXRow(FILE *Infp,char *Image,int Width) {
	// Variables
	int i,b,x,RunLen;

	// Decrunch
	x=0;
	while(x<Width) {                  /* get whole image row  */
		if(EOF==(b=getc(Infp)))
			return(0);

		if(b>0xC0) {
			RunLen=b&0x3F;
			if(EOF==(b=getc(Infp)))   			/* get the pixel value  */
				return 0;
		} else
			RunLen=1;                		/* a single pixel value */

		if(x>=Width)
		break;          					/* copy image's pixel 	*/
		for(i=0;i<RunLen;i++) { 			/* copy value to image	*/
			if(x<Width)
				Image[x]=b;
			x++;
		}
	}

	return(1);
}



void BlockAlloc(VGABlock &dest,int width,int height) {
	// Allocate memory for block
	dest.size=width*height;

	dest.data=(unsigned char *) malloc(dest.size+50);
	if(!dest.data)
		Error("Error - can't allocate data memory in BlockAlloc");

	dest.palette=new char[768];
	if(!dest.palette)
		Error("Error - can't allocate palette memory in BlockAlloc");
}



void BlockDealloc(VGABlock &dest) {
	// De-Allocate memory for block
	dest.size=0;

	if(dest.data) free(dest.data);
	delete(dest.palette);
}


void BlockLoadPCX(char *filename,VGABlock &dest) {
	// Get size of picture
	int width=GetPCXWidth(filename),height=GetPCXHeight(filename);

	// Check size
	if((unsigned long) width*height>(unsigned long) 65000)
		Error("Error - block size >65000 bytes");
	if((unsigned long) width*height<(unsigned long) 1)
		Error("Error - block size negative or zero");

	// Allocate memory
	BlockAlloc(dest,width,height);

	// Store image attributes
	dest.height=height;
	dest.width=width;
	dest.opaque=0;
	dest.size=width*height;

	// Open file
	FILE *ptr;
	if((ptr=fopen(filename,"rb"))==NULL)
		Error("Error - can't open block file");
	fseek(ptr,128,0);

	// Decode picture
	for(int p=0;p<height;p++)
		if(!GetPCXRow(ptr,&dest.data[p*width],width))
			Error("Error decrunching block from file");

	// Load palette
	fseek(ptr,-768,2);
	fread(dest.palette,768,1,ptr);
	fclose(ptr);

	// Adjust palette to VGA
	for(p=0;p<768;p++)
		dest.palette[p]>>=2;
}



void GetPalette(unsigned char *pal) {
	// Get palette info
	int c=0,p;
	for(p=0;p<256;p++) {
		outp(0x3c7,p&0xff);
		pal[c++]=inportb(0x3c9);
		pal[c++]=inportb(0x3c9);
		pal[c++]=inportb(0x3c9);
	}
}



void SetPalette(unsigned char pal[768]) {
	for(int p=0;p<256;p++) {
		outp(0x3c8,p&0xff);
		outp(0x3c9,pal[(p*3)]);
		outp(0x3c9,pal[(p*3)+1]);
		outp(0x3c9,pal[(p*3)+2]);
	}
}



void SetColor(int p,char r,char g,char b) {
	outp(0x3c8,p&0xff);
	outp(0x3c9,r);
	outp(0x3c9,g);
	outp(0x3c9,b);
}



void ReadPCXPalette(char *name,char pal[768]) {
	// Variables
	FILE *ptr;

	// Open file
	ptr=fopen(name,"rb");
	if(!ptr)
		Error("Error - File not found in LoadPCXPalette");
	fseek(ptr,-768,2);
	fread(pal,768,1,ptr);
	fclose(ptr);

	// Adjust palette to VGA
	for(int i=0;i<768;i++)
		pal[i]>>=2;
}



void SetPlane(char plane) {
	asm {
		mov al,0x02
		mov dx,0x3c4
		out dx,al
	}
	_AL=1<<(plane&3);
	asm {
		inc dx
		out dx,al
	}
}



void SetReadPlane(char plane) {
	asm {
		mov al,0x04
		mov ah,plane
		and ah,0x03

		mov dx,0x03ce
		out dx,ax
	}
}



void SetAllPlanes(void) {
	asm {
		mov dx,0x3c4
		mov ax,0x0f02
		out dx,ax
	}
}



void HLineX(int y,int x1,int x2,char color) {
	int off=pplookup[y],i,pos,w;

	pos=x1;
	for(i=x1;i<=x2;i++)
		if(i%4)
			asm {
				mov ax,i                   // Move X-value into ax
				mov cx,ax                  // And in cx
				shr ax,2                   // Divide ax by 4  (x div 4)
				mov di,off
				add di,ax                  // Y:=Y+X   bx contains final location
				add di,actStart

				and cx,3             	   // cl:=0,1,2 or 3  (x mod 4)
				mov ah,1             	   // Start value (plane 0)
				shl ah,cl                  // Eg plane 2: 00000001 => 00000100
				mov dx,3c4h                // Sequencer Register
				mov al,2                   // Select index 2 => plane enable
				out dx,ax                  // Enable selected plane
				mov ax,0xa000              // VGA segment
				mov es,ax                  // es points to VGA
				mov al,color               // Color in al
				stosb

				inc pos
			}
		else
			break;

	// Middle bit
	w=x2-pos+1;
	i=w>>3;
	asm {
		// Set planes
		mov dx,0x3c4
		mov ax,0x0f02
		out dx,ax

		// Draw line
		mov ax,0xa000
		mov es,ax

		mov di,off
		add di,actStart

		mov ax,pos
		shr ax,2
		add di,ax

		mov al,color
		mov ah,color

		mov cx,i
		rep stosw
	}
	pos+=i<<3;

	// End bit
	for(i=pos;i<=x2;i++)
		asm {
			mov ax,i                   // Move X-value into ax
			mov cx,ax                  // And in cx
			shr ax,2                   // Divide ax by 4  (x div 4)
			mov di,off
			add di,ax                  // Y:=Y+X   bx contains final location
			add di,actStart

			and cx,3             	   // cl:=0,1,2 or 3  (x mod 4)
			mov ah,1             	   // Start value (plane 0)
			shl ah,cl                  // Eg plane 2: 00000001 => 00000100
			mov dx,3c4h                // Sequencer Register
			mov al,2                   // Select index 2 => plane enable
			out dx,ax                  // Enable selected plane

			mov ax,0xa000              // VGA segment
			mov es,ax                  // es points to VGA
			mov al,color               // Color in al
			stosb
		}
}



void GetLineX(char *buf,int width,int x,int y) {
	// Variables
	unsigned off,o,p;

	// Read line
	off=(y*80)+actStart;
	for(p=0;p<width;p++) {
		o=off+((p+x)/4);
		SetReadPlane(p+x);
		asm {
			push ds

			mov ax,0xa000
			mov ds,ax
			mov si,o

			lodsb

			pop ds
		}
		buf[p]=_AL;
	}
}



void GetLine(char *buf,int width,int x,int y) {
	// Variables
	unsigned bufseg,bufoff,pos;

	bufseg=FP_SEG(buf);
	bufoff=FP_OFF(buf);

	unsigned a=sscreen,b=oscreen;

	// Display line
	pos=pplookup[y]+x;
	asm {
		// Dest
		mov es,bufseg
		mov di,bufoff

		// Source
		push ds
		mov ds,a
		mov si,pos
		add si,b

		// Read line
		mov cx,width
		rep movsb

		pop ds
	}
}



void PutLineX(char *buf,int width,int x,int y) {
	// Variables
	char plane;
	unsigned srce,dest,off;

	// Draw line
	off=(y*80)+actStart;
	for(plane=0;plane<4;plane++) {
		dest=off+(x/4);
		SetPlane(x&3);
		x++;
		srce=plane;

		_ES=0xa000;

		while(srce<width) {
			_DI=dest;
			_AL=buf[srce];

			asm stosb

			srce+=4;
			dest++;
		}
	}
}



void PutLine(char *buf,int width,int x,int y) {
	// Variables
	unsigned bufseg,bufoff;

	bufseg=FP_SEG(buf);
	bufoff=FP_OFF(buf);

	// Display line
	_DI=pplookup[y]+x;
	asm {
		// Dest
		mov es,sscreen
		add di,oscreen

		// Source
		push ds
		mov ds,bufseg
		mov si,bufoff

		// Do line
		mov cx,width
		rep movsb

		pop ds
	}
}



void BlockPut(VGABlock srce,int x,int y) {
	// Variables
	unsigned long p=0;
	int end=y+srce.height,cut=0,a;
	char plane;
	unsigned s,dest,off;

	// Check position
	if(y>=(int) scrnheight || end<0)
		return;

	if(x<0 || x>319)
		return;

	if(y<0) {
		p=-y*srce.width;
		y=0;
	}

	if(end>scrnheight) {
		end=scrnheight;
	}

	if(x+srce.width>319)
		cut=(x+srce.width)-320;

	// Draw block
	if(Mode=='X')
		for(a=y;a<end;a++) {
			// Draw line
			off=pplookup[a]+actStart;
			for(plane=0;plane<4;plane++) {
				dest=off+(x/4);
				SetPlane(x&3);
				x++;
				s=plane;

				_ES=0xa000;

				while(s<srce.width-cut) {
					_DI=dest;
					_AL=srce.data[p+s];

					asm {
						stosb

						add s,4
						inc dest
					}
				}
			}
			x-=4;
			p+=srce.width;
		}
	else
		for(a=y;a<end;a++) {
			// Draw Line
			PutLine(srce.data+(srce.width*(a-y)),srce.width,x,a);
		}
}



void BlockPutX(VGABlock srce,int x,int y) {
	BlockPut(srce,x,y);
}



void BarX(int left,int top,int right,int bottom,char color) {
	// Check position
	if(right<left)
		asm {
			push right
			push left
			pop right
			pop left
		}

	if(bottom<top)
		asm {
			push top
			push bottom
			pop top
			pop bottom
		}

	if(left<0 || top<0 || left>319 || top>=scrnheight)
		return;

	// Sort out height and width
	if(bottom>scrnheight)
		bottom=scrnheight;
	if(right>=320)
		right=319;

	// Draw block
	for(int a=top;a<=bottom;a++)
		HLineX(a,left,right,color);
}



void Bar(int left,int top,int right,int bottom,char color) {
	// Variables
	unsigned pos,count;

	if(Mode=='X') {
		BarX(left,top,right,bottom,color);
		return;
	}

	// Check position
	if(right<left)
		asm {
			push right
			push left
			pop right
			pop left
		}

	if(bottom<top)
		asm {
			push top
			push bottom
			pop top
			pop bottom
		}

	if(left<0 || top<0 || left>319 || top>=scrnheight)
		return;

	// Sort out height and width
	if(bottom>scrnheight)
		bottom=scrnheight;
	if(right>=320)
		right=319;

	// Draw block
	count=(right-left)+1;
	for(int a=top;a<=bottom;a++) {
		pos=pplookup[a]+oscreen;
		asm {
			mov ax,sscreen
			mov es,ax
			mov di,pos
			add di,left

			mov cx,count
			mov al,color

			rep stosb
		}
	}
}



void FillBox(int l,int t,int r,int b,char col) {
	Bar(l,t,r,b,col);
}



void ClsX(char color) {
	// Variables
	unsigned c=40*scrnheight;

	asm {
		// Select all planes
		mov dx,0x3c4
		mov ax,0x0f02
		out dx,ax

		// Write color
		mov dx,0xa000
		mov es,dx

		mov di,actStart

		mov cx,c
		mov ah,color
		mov al,color

		rep stosw
	}
}



void Cls(char color) {
	if(Mode=='X')
		ClsX(color);
	else
		// Fill 'vs' with color
		asm {
			mov es,sscreen
			mov di,oscreen

			mov cx,32000

			mov al,color
			mov ah,color

			rep stosw
		}
}



void LineX(int a,int b,int c,int d,char color) {
	int i,s,d1x,d1y,d2x,d2y,u,v,m,n,qy;

	// Check boundries
	if((a<Clip.l || a>Clip.r) && (b<Clip.t || b>Clip.b)
	   && (c<Clip.l || c>Clip.r) && (d<Clip.t || d>Clip.b))
		return;

	// Set plane intially
	SetPlane(a&3);

	// Work out gradients
	u=c-a;
	v=d-b;
	d1x=SGN(u);
	d1y=SGN(v);
	d2x=SGN(u);
	d2y=0;
	m=abs(u);
	n=abs(v);
	if(!(m>n)) {
		d2x=0;
		d2y=SGN(v);
		m=abs(v);
		n=abs(u);
	}
	s=m>>1;

	// Draw the line
	for(i=0;i<=m;i++) {
		// Inline putpixel
		if(a>=Clip.l && a<=Clip.r && b>=Clip.t && b<=Clip.b) {
			qy=pplookup[b];
			asm {
				// Put pixel
				mov di,qy

				mov dx,a
				shr dx,2
				add di,dx
				add di,actStart

				mov dx,0xa000
				mov es,dx

				mov al,color
				stosb
			}
		}

		s+=n;
		if(!(s<m)) {
			s-=m;
			a+=d1x;
			b+=d1y;

			// Set new plane
			asm {
				mov al,0x02
				mov dx,0x3c4
				out dx,al
			}

			_AL=1<<(a&3);

			asm {
				inc dx
				out dx,al
			}
		} else {
			a+=d2x;
			b+=d2y;

			// Set new plane
			asm {
				mov al,0x02
				mov dx,0x3c4
				out dx,al
			}

			_AL=1<<(a&3);

			asm {
				inc dx
				out dx,al
			}
		}
	}
}



void Line(int a,int b,int c,int d,char color) {
	// Variables
	int i,s,d1x,d1y,d2x,d2y,u,v,m,n;

	if(Mode=='X') {
		LineX(a,b,c,d,color);
		return;
	}

	// Check boundries
	if((a<Clip.l || a>Clip.r) && (b<Clip.t || b>Clip.b)
	   && (c<Clip.l || c>Clip.r) && (d<Clip.t || d>Clip.b))
		return;

	// Work out stuff
	u=c-a;
	v=d-b;
	d1x=SGN(u);
	d1y=SGN(v);
	d2x=SGN(u);
	d2y=0;
	m=abs(u);
	n=abs(v);
	if(!(m>n)) {
		d2x=0;
		d2y=SGN(v);
		m=abs(v);
		n=abs(u);
	}
	s=m>>1;
	for(i=0;i<=m/2;i++) {
		// Inline putpixel 1
		if(a>=Clip.l && a<=Clip.r && b>=Clip.t && b<=Clip.b) {
			_DI=pplookup[b]+a;
			asm {
				mov es,sscreen
				add di,oscreen

				mov al,color
				stosb
			}
		}

		// Inline putpixel 2
		if(c>=Clip.l && c<=Clip.r && d>=Clip.t && d<=Clip.b) {
			_DI=pplookup[d]+c;
			asm {
				mov es,sscreen
				add di,oscreen

				mov al,color
				stosb
			}
		}

		s+=n;
		if(!(s<m)) {
			s-=m;
			a+=d1x;
			b+=d1y;

			c-=d1x;
			d-=d1y;
		} else {
			a+=d2x;
			b+=d2y;

			c-=d2x;
			d-=d2y;
		}
	}
}



void DotLine(int x1,int y1,int x2,int y2,int dots,char col) {
	// Variables
	int i;
	float x1x2,y1y2,x,y;

	// Get translation vectors
	x1x2=x2-x1;
	y1y2=y2-y1;

	// Quarter it (for four dots per line)
	x1x2/=(dots-1);
	y1y2/=(dots-1);

	// Draw dots
	x=x1;
	y=y1;

	for(i=0;i<dots;i++) {
		if(x>=0 && x<320 && y>=0 && y<scrnheight)
			PutPixel(x,y,col);

		x+=x1x2;
		y+=y1y2;
	}
}



void ScreenLoadPCX(char *filename,int x,int y) {
	// Get size of picture
	int width=GetPCXWidth(filename),height=GetPCXHeight(filename);

	// Open file
	FILE *ptr;
	if((ptr=fopen(filename,"rb"))==NULL)
		Error("Error - can't open PCX file");
	fseek(ptr,128,0);

	// Decode picture
	char *data;
	unsigned char pal[770];

	data=new char[width+50];
	for(int p=0;p<height;p++) {
		if(!GetPCXRow(ptr,data,width))
			Error("Error decrunching block from file");

		if(y+p<scrnheight)
			if(Mode=='X')
				PutLineX(data,(width>320)?320:width,x,y+p);
			else
				PutLine(data,(width>320)?320:width,x,y+p);
	}

	delete data;

	// Load palette
	fseek(ptr,-768,2);
	fread(pal,768,1,ptr);
	fclose(ptr);

	// Adjust palette to VGA
	for(p=0;p<768;p++)
		pal[p]/=4;

	SetPalette(pal);
	Waitvlb();
}



char GetPixelX(int x,int y) {
	// Get y into ax
	_AX=pplookup[y];
	asm {
		// Get x into bx
		mov bx,x
		mov cx,x
		shr bx,2

		// Get offset into bx
		add bx,ax
		add bx,actStart

		// Read pixel
		mov al,0x04
		mov ah,cl
		and ah,0x03

		mov dx,0x03ce
		out dx,ax

		xor ah,ah
		mov al,es:[bx]
	}

	return(_AL);
}



char GetPixel(int x,int y) {
	if(x<0 || x>319 || y<0 || y>199)
		return(0);

	return(vscreen[pplookup[y]+x]);
}



void BlockGet(VGABlock &dest,int left,int top,int right,int bottom) {
	// Variables
	int width,height,x,y;

	// Check area
	if(left>right || top>bottom)
		Error("Error - Invalid area in BlockGetX");

	if(left<0 || right<0 || left>319 || right>319)
		Error("Error - Invalid area in BlockGetX");

	if(top<0 || bottom<0 || top>=scrnheight || bottom>=scrnheight)
		Error("Error - Invalid area in BlockGetX");

	// Get size
	width=right-left+1;
	height=bottom-top+1;

	// Store image attributes
	dest.height=height;
	dest.width=width;
	dest.opaque=0;
	dest.size=width*height;

	// Get image
	if(Mode=='X') {
		for(y=0;y<height;y++)
			GetLineX(&dest.data[y*width],width,left,y+top);
	} else
		for(y=0;y<height;y++)
			GetLine(&dest.data[y*width],width,left,y+top);

	// Get Palette
	GetPalette(dest.palette);
}



void BlockGetX(VGABlock &dest,int left,int top,int right,int bottom) {
	BlockGet(dest,left,top,right,bottom);
}



char BlockSave(char *filename,VGABlock Data) {
	// Variables
	FILE *ptr;
	int i;

	// Create file
	ptr=fopen(filename,"wb");
	if(!ptr)
		return(0);

	// Dump block information
	fwrite(&Data.width,2,1,ptr);
	fwrite(&Data.height,2,1,ptr);
	fwrite(&Data.size,4,1,ptr);
	fwrite(&Data.opaque,1,1,ptr);

	// Dump Palette
	fwrite(Data.palette,768,1,ptr);

	// Dump data
	fwrite(Data.data,Data.size,1,ptr);

	// Close
	fclose(ptr);
	return(1);
}



char BlockLoad(char *filename,VGABlock &Data) {
	// Variables
	FILE *ptr;

	// Open file
	ptr=fopen(filename,"rb");
	if(!ptr)
		return(0);

	// Read block information
	fread(&Data,9,1,ptr);

	// Allocate memory for block
	BlockAlloc(Data,Data.width,Data.height);

	// Read Palette
	fread(Data.palette,768,1,ptr);

	// Read data
	fread(Data.data,Data.size,1,ptr);

	// Close
	fclose(ptr);
	return(1);
}



void CopyScreenX(char srce,char dest) {
	// Variables
	unsigned a,b,c;

	// Return if error
	if(srce==dest)
		return;

	// Get address offsets
	a=srce*80*scrnheight;
	b=dest*80*scrnheight;

	// Set counter
	c=80*scrnheight;

	// Copy screen
	for(char l=0;l<4;l++) {
		// Set write plane
		asm {
			mov al,0x02
			mov dx,0x3c4
			out dx,al
			mov al,1
			xor cx,cx
			mov cl,l
			rep shl al,cl
			inc dx
			out dx,al

			// Set read plane
			mov al,0x04
			mov ah,l
			mov dx,0x03ce
			out dx,ax

			// Push registers
			push ds

			// Set ds and es
			mov ax,0xa000
			mov ds,ax
			mov es,ax

			// Set offsets
			mov si,a
			mov di,b

			// Write bytes
			mov cx,c
			rep movsb

			// Pop registers
			pop ds
		}
	}
}



void Dump(void) {
	unsigned a=sscreen,b=oscreen;

	if(Mode=='V')
	asm {
		push ds
		mov ax,0xa000
		mov es,ax
		xor di,di

//		mov cx,32000
		mov cx,16000

		mov ds,a
		mov si,b

//		rep movsw
		db 0xf3,0x66,0xa5

		pop ds
	}
}




void UpdateAreaX(char p1,char p2,int left,int top,int right,int bottom) {
	// Check areas
	if(left<0)
		left=0;
	else if(left>319)
		left=319;
	if(right<0)
		right=0;
	else if(right>319)
		right=319;
	if(top<0)
		top=0;
	else if(top>=scrnheight)
		top=scrnheight-1;
	if(bottom<0)
		bottom=0;
	else if(bottom>=scrnheight)
		bottom=scrnheight-1;

	// Check ranges
	if(left>right)
		asm {
			push left
			push right
			pop left
			pop right
		}
	if(top>bottom)
		asm {
			push top
			push bottom
			pop top
			pop bottom
		}

	// Variables
	unsigned soff,doff,ptr,sof,dof,height;

	// Set pointers
	soff=(80*p1*scrnheight)+(80*top);
	doff=(80*p2*scrnheight)+(80*top);
	height=bottom-top+1;

	// Draw line
	for(int plane=0;plane<4;plane++) {
		SetReadPlane(left+plane);
		SetPlane(left+plane);
		for(int p=left+plane;p<=right;p+=4) {
			ptr=p>>2;
			sof=soff+ptr;
			dof=doff+ptr;

			asm {
				push ds

				mov ax,0xa000
				mov ds,ax
				mov es,ax

				// Loop
				mov cx,height
			}
			loop1:
			asm {
				mov si,sof
				mov di,dof

				movsb
				add sof,80
				add dof,80

				loop loop1

				pop ds
			}
		}
	}
}



void CreatePal(void) {
	// Define colors
	int c=16,p;

	// Grey
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,4*p);
		outp(0x3c9,4*p);

		c++;
	}

	// Red
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,0);
		outp(0x3c9,0);

		c++;
	}

	// Green
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,0);
		outp(0x3c9,4*p);
		outp(0x3c9,0);

		c++;
	}

	// Blue
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,0);
		outp(0x3c9,0);
		outp(0x3c9,4*p);

		c++;
	}

	// Yellow
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,4*p);
		outp(0x3c9,0);

		c++;
	}

	// Cyan
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,0);
		outp(0x3c9,4*p);
		outp(0x3c9,4*p);

		c++;
	}

	// Magenta
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,0);
		outp(0x3c9,4*p);

		c++;
	}

	// Orange
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,2*p);
		outp(0x3c9,0);

		c++;
	}

	// Mint
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,0);
		outp(0x3c9,4*p);
		outp(0x3c9,2*p);

		c++;
	}

	// Pink
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,0);
		outp(0x3c9,2*p);

		c++;
	}

	// Sky Blue
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,0);
		outp(0x3c9,2*p);
		outp(0x3c9,4*p);

		c++;
	}

	// Purple
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,2*p);
		outp(0x3c9,0);
		outp(0x3c9,4*p);

		c++;
	}

	// Teal
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,p);
		outp(0x3c9,2*p);
		outp(0x3c9,2*p);

		c++;
	}

	// Teal
	for(p=0;p<16;p++) {
		outp(0x3c8,c&0xff);
		outp(0x3c9,4*p);
		outp(0x3c9,2*p);
		outp(0x3c9,p);

		c++;
	}
}



/**************************************************************************/
//
//		Texture Mapping Routines
//
/**************************************************************************/


int lefttable[400][3],righttable[400][3];
int TmapMinY,TmapMaxY;

struct {
	int size[4];
	unsigned bobseg,boboff;
} Texture;



void SetTextureMap(char *data,int size) {
	Texture.size[0]=size;
	Texture.size[1]=size<<7;
	Texture.size[2]=(size-1)<<7;
	Texture.size[3]=(-size)<<7;

	Texture.bobseg=FP_SEG(data);
	Texture.boboff=FP_OFF(data);
}


void scanleftside(int x1,int x2,int ytop,int lineheight,char side) {
	int xadd,pxadd,pyadd;
	unsigned x,px,py,y;

	lineheight++;
	xadd=((x2-x1)<<7)/lineheight;
	if(side==1) {
		px=Texture.size[2];
		py=0;
		pxadd=Texture.size[3]/lineheight;
		pyadd=0;
	} else
	if(side==2) {
		px=Texture.size[1];
		py=Texture.size[1];
		pxadd=0;
		pyadd=Texture.size[3]/lineheight;
	} else
	if(side==3) {
		px=0;
		py=Texture.size[1];
		pxadd=Texture.size[1]/lineheight;
		pyadd=0;
	} else
	if(side==4) {
		px=0;
		py=0;
		pxadd=0;
		pyadd=Texture.size[1]/lineheight;
	}

	x=x1<<7;

	for(y=0;y<=lineheight;y++) {
		lefttable[ytop+y][0]=x>>7;
		lefttable[ytop+y][1]=px>>7;
		lefttable[ytop+y][2]=py>>7;

		x+=xadd;
		px+=pxadd;
		py+=pyadd;
	}
}



void scanrightside (int x1,int x2,int ytop,int lineheight,char side) {
	int xadd,pxadd,pyadd;
	unsigned x,px,py,y;

	lineheight++;

	xadd=((x2-x1)<<7)/lineheight;
	if(side==1) {
		px=0;
		py=0;
		pxadd=Texture.size[1]/lineheight;
		pyadd=0;
	} else
	if(side==2) {
		px=Texture.size[1];
		py=0;
		pxadd=0;
		pyadd=Texture.size[1]/lineheight;
	} else
	if(side==3) {
		px=Texture.size[1];
		py=Texture.size[1];
		pxadd=Texture.size[3]/lineheight;
		pyadd=0;
	} else
	if(side==4) {
		px=0;
		py=Texture.size[1];
		pxadd=0;
		pyadd=Texture.size[3]/lineheight;
	}

	x=x1<<7;

	for(y=0;y<=lineheight;y++) {
		righttable[ytop+y][0]=x>>7;
		righttable[ytop+y][1]=px>>7;
		righttable[ytop+y][2]=py>>7;

		x+=xadd;
		px+=pxadd;
		py+=pyadd;
	}
}



void Texturemap() {
	// Variables
	int px1,py1,px2,py2,polyx1,polyx2,y,linewidth,pxadd,pyadd;
	unsigned bob,boboff;

	// Get addresses
	bob=Texture.bobseg;
	boboff=Texture.boboff;

	// Check limits
	if(TmapMinY>199) return;
	if(TmapMaxY<0) return;
	if(TmapMinY<0) TmapMinY=0;
	if(TmapMaxY>199) TmapMaxY=199;
	if(TmapMaxY-TmapMinY<2) return;

	// Draw texture
	for(y=TmapMinY;y<=TmapMaxY;y++) {
	  polyx1=lefttable[y][0];     	// X Starting position
	  px1=lefttable[y][1]<<7;   		// Texture X at start
	  py1=lefttable[y][2]<<7;   		// Texture Y at stary

	  polyx2=righttable[y][0];    	// X Ending position
	  px2=righttable[y][1]<<7;  		// Texture X at end
	  py2=righttable[y][2]<<7;  		// Texture Y at end

	  linewidth=polyx2-polyx1;    	// Width of line
	  if(linewidth<=0) linewidth=1;

	  pxadd=(px2-px1)/linewidth;
	  pyadd=(py2-py1)/linewidth;
		asm {
		  push    ds
		  mov     bx,polyx1
		  mov     di,bx

		  mov     dx,y
		  mov     bx, dx
		  shl     dx, 8
		  shl     bx, 6
		  add     dx, bx
		  add     di, dx
		  add  	  di,oscreen
		  mov  	  ax,sscreen
		  mov     es,ax

		  mov     bx, px1

		  mov     cx,linewidth
		  mov     dx, bob
		  mov     ds, dx

		  mov     dx,py1
		}
	Loop1 :
		asm {
		  mov 	si,boboff
		  mov     ax,bx
		  and     ax,0xff00   	// Get rid of fixed point
		  add     si,ax
		  mov     ax,dx
		  shr     ax,7
		  add     si,ax         // get the pixel in our texture
		  movsb                 // draw the pixel to the screen
		  mov     ax,pxadd
		  add     bx,ax
		  mov     ax,pyadd
		  add     dx,ax         // increment our position in the texture
		  loop    Loop1
		  pop     ds
		}
	}
}




void TextureMapPoly(int x1,int y1,int x2,int y2,int x3,int y3,int x4,int y4) {
	TmapMinY=32767;
	TmapMaxY=0;

	if(y1<TmapMinY) TmapMinY=y1;
	if(y1>TmapMaxY) TmapMaxY=y1;
	if(y2<TmapMinY) TmapMinY=y2;
	if(y2>TmapMaxY) TmapMaxY=y2;
	if(y3<TmapMinY) TmapMinY=y3;
	if(y3>TmapMaxY) TmapMaxY=y3;
	if(y4<TmapMinY) TmapMinY=y4;
	if(y4>TmapMaxY) TmapMaxY=y4;

	if(TmapMinY>TmapMaxY-5) return;	 	// Why paint slivers?

	if(y2<y1) scanleftside(x2,x1,y2,y1-y2,1);
	else scanrightside(x1,x2,y1,y2-y1,1);

	if(y3<y2) scanleftside(x3,x2,y3,y2-y3,2);
	else scanrightside(x2,x3,y2,y3-y2,2);

	if(y4<y3) scanleftside(x4,x3,y4,y3-y4,3);
	else scanrightside(x3,x4,y3,y4-y3,3);

	if(y1<y4) scanleftside(x1,x4,y1,y4-y1,4);
	else scanrightside(x4,x1,y4,y1-y4,4);

	Texturemap();
}



/**************************************************************************/
//
// VGA Mode stuff
//
/**************************************************************************/


void ClsPCX(char far *src) {
	unsigned a=sscreen,b=oscreen;
	unsigned c=FP_SEG(src),d=FP_OFF(src);

	asm {
		push ds
		mov es,a
		mov di,b

		mov ds,c
		mov si,d

//		mov cx,32000
//		rep movsw
		mov cx,16000
		db 0xf3,0x66,0xa5

		pop ds
	}
}



void VGASquarePixel(void) {
	outp(0x3c2,0xe3);
}



void FadeOut(float speed) {
	// Variables
	unsigned char pal[768],blank[768];
	float llevel;
	int p;

	// Read palette
	GetPalette(pal);

	// Fade to black
	for(llevel=1.0;llevel>=0;llevel-=speed) {
		for(p=0;p<768;p++)
			blank[p]=pal[p]*llevel;
		SetPalette(blank);
		Waitvlb();
	}
}



void FadeIn(float speed,unsigned char pal[768]) {
	// Variables
	unsigned char blank[768];
	float llevel;
	int p;

	// Fade to user palette
	for(llevel=0;llevel<=1.0;llevel+=speed) {
		for(p=0;p<768;p++)
			blank[p]=pal[p]*llevel;
		SetPalette(blank);
		Waitvlb();
	}
}



void WipeScreen(int start) {
	// Variables
	int line,plus,count,
		s=FP_SEG(vscreen),o=FP_OFF(vscreen);

	// Check video mode
	if(Mode!='V')
		return;

	// Check bounds
	if(start>=scrnheight-1)
		start=scrnheight-2;

	// Wipe
	for(line=scrnheight-2;line>start;line--) {
		// Wipe from 'line' to base of screen
		plus=pplookup[line];
//		count=160*((scrnheight-1)-line);
		count=80*((scrnheight-1)-line);

		asm {
			// Set source
			push ds
			mov ax,0xa000
			mov ds,ax
			mov si,plus

			// Set dest
			mov es,ax
			mov di,plus
			add di,320

			// Mimic line
			mov cx,count
//			rep movsw
			db 0xf3,0x66,0xa5

			pop ds
		}
	}

	// Copy 'wiped' screen to vscreen
	asm {
		// Set source
		push ds
		mov ax,0xa000
		mov ds,ax
		xor si,si

		// Set dest
		mov es,s
		mov di,o

		// Copy
//		mov cx,32000
//		rep movsw
		mov cx,16000
		db 0xf3,0x66,0xa5

		pop ds
	}
}



void Delay(int tenths) {
	// Variables
	float stoptime;

	// Get stop time
	stoptime=(clock()/CLK_TCK)+(0.1*tenths);

	// Loop until done
	while((clock()/CLK_TCK)<stoptime);
}
