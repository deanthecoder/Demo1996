int order[1024],maxz[1024],cX=160,cY=100;

/****************************************************************************/

struct Point3D {
	int x,y,z;
};

struct Poly3D {
	int join[4];
	char color;
};

struct Shape3D {
	Point3D *point;
	Poly3D *poly;
	int maxpoints,maxpolys;
};

struct {
	Point3D *point;
	int *sx,*sy;
} trans;


/****************************************************************************/
void Translate(Shape3D &Shape);


void Rotate(Shape3D &shape,int x,int y,int z) {
	// Variables
	int a,b,c,i;
	static defined=0,lookup[361][2];

	// Create lookup tables
	if(!defined) {
		for(i=0;i<=360;i++) {
			lookup[i][0]=sin(rad(i))*16384;
			lookup[i][1]=cos(rad(i))*16384;
		}
		defined=1;
	}

	// Rotate points
	for(i=0;i<shape.maxpoints;i++) {
		// Rotate Y
		b=lookup[y][1];
		c=shape.point[i].x;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			mov   a,dx
		}
		b=lookup[y][0];
		c=shape.point[i].z;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			add   a,dx
		}
		trans.point[i].x=a;
		trans.point[i].y=shape.point[i].y;
		b=-lookup[y][0];
		c=shape.point[i].x;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			mov   a,dx
		}
		b=lookup[y][1];
		c=shape.point[i].z;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			add   a,dx
		}
		trans.point[i].z=a;

		// Rotate X
		if(x) {
			b=lookup[x][1];
			c=trans.point[i].y;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[x][0];
			c=trans.point[i].z;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				sub   a,dx
			}
			b=lookup[x][0];
			c=trans.point[i].y;
			trans.point[i].y=a;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[x][1];
			c=trans.point[i].z;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				add   a,dx
			}
			trans.point[i].z=a;
		}

		// Rotate Z
		if(z) {
			b=lookup[z][1];
			c=trans.point[i].x;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[z][0];
			c=trans.point[i].y;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				sub   a,dx
			}
			b=lookup[z][0];
			c=trans.point[i].x;
			trans.point[i].x=a;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[z][1];
			c=trans.point[i].y;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				add   a,dx
			}
			trans.point[i].y=a;
		}
	}
}


/****************************************************************************/


void Perspective(Shape3D shape) {
	// Variables
	int i,denom;;
	float pers;

	// Perform calculation
	for(i=0;i<shape.maxpoints;i++) {
		denom=320+trans.point[i].z;

		if(!denom) denom=1;

		pers=320.0/denom;

		trans.point[i].x*=pers;
		trans.point[i].y*=pers;
	}
}


/****************************************************************************/


void Plot3D(Shape3D shape,char color) {
	// Variables
	static defined=0,cx,cy;
	int i;

	// Initialise
	if(!defined) {
		cx=160;
		cy=scrnheight>>1;
		defined=1;
	}

	// Translate points
	Translate(shape);

	// Draw Points if on screen
	for(i=0;i<shape.maxpoints;i++)
		if(trans.sx[i]>=0 && trans.sx[i]<320 &&
		   trans.sy[i]>=0 && trans.sy[i]<scrnheight)
			PutPixel(trans.sx[i],trans.sy[i],color);
}


/****************************************************************************/


void QuickSort(int b,int array[],int array2[]) {
	// Variables
	int a,ptr,i,tmp;

	// Check size
	if(b<1)
		return;

	// Sorting loop
	a=0;
	do {
		ptr=a;
		// Find smallest value
		for(i=a+1;i<=b;i++)
			if(array[i]>array[ptr])
				ptr=i;

		// Swap
		if(ptr!=a) {
			tmp=array[a];
			array[a]=array[ptr];
			array[ptr]=tmp;

			tmp=array2[a];
			array2[a]=array2[ptr];
			array2[ptr]=tmp;
		}
	} while (++a<b);
}



void DepthSort(Shape3D shape) {
	// Reset order and find depths
	for(int i=0;i<shape.maxpolys;i++) {
		order[i]=i;
		maxz[i]=(trans.point[shape.poly[i].join[0]].z+
				 trans.point[shape.poly[i].join[3]].z);
	}

	// Sort
	QuickSort(shape.maxpolys-1,maxz,order);
}



/****************************************************************************/


void Translate(Shape3D &Shape) {
	for(int i=0;i<Shape.maxpoints;i++) {
		trans.sx[i]=cX+trans.point[i].x;
		trans.sy[i]=cY-trans.point[i].y;
	}
}


/****************************************************************************/

void Rotate(Shape3D &shape,int x,int y,int z,int ox,int oy,int oz) {
	// Variables
	int a,b,c,i;
	static defined=0,lookup[361][2];

	// Create lookup tables
	if(!defined) {
		for(i=0;i<=360;i++) {
			lookup[i][0]=sin(rad(i))*16384;
			lookup[i][1]=cos(rad(i))*16384;
		}
		defined=1;
	}

	// Rotate points
	for(i=0;i<shape.maxpoints;i++) {
		// Rotate Y
		b=lookup[y][1];
		c=shape.point[i].x-ox;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			mov   a,dx
		}
		b=lookup[y][0];
		c=shape.point[i].z-oz;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			add   a,dx
		}
		trans.point[i].x=a;
		trans.point[i].y=shape.point[i].y-oy;
		b=-lookup[y][0];
		c=shape.point[i].x-ox;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			mov   a,dx
		}
		b=lookup[y][1];
		c=shape.point[i].z-oz;
		asm {
			mov   ax,b
			imul  c
			sal   ax,1
			rcl   dx,1
			sal   ax,1
			rcl   dx,1
			add   a,dx
		}
		trans.point[i].z=a;

		// Rotate X
		if(x) {
			b=lookup[x][1];
			c=trans.point[i].y;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[x][0];
			c=trans.point[i].z;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				sub   a,dx
			}
			b=lookup[x][0];
			c=trans.point[i].y;
			trans.point[i].y=a;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[x][1];
			c=trans.point[i].z;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				add   a,dx
			}
			trans.point[i].z=a;
		}

		// Rotate Z
		if(z) {
			b=lookup[z][1];
			c=trans.point[i].x;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[z][0];
			c=trans.point[i].y;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				sub   a,dx
			}
			b=lookup[z][0];
			c=trans.point[i].x;
			trans.point[i].x=a;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				mov   a,dx
			}
			b=lookup[z][1];
			c=trans.point[i].y;
			asm {
				mov   ax,b
				imul  c
				sal   ax,1
				rcl   dx,1
				sal   ax,1
				rcl   dx,1
				add   a,dx
			}
			trans.point[i].y=a;
		}
	}
}

/****************************************************************************/


char Visible(Shape3D shape,int p) {
	// Get normals
	int x21=trans.point[shape.poly[p].join[1]].x-trans.point[shape.poly[p].join[0]].x,
		y21=trans.point[shape.poly[p].join[1]].y-trans.point[shape.poly[p].join[0]].y,
		x32=trans.point[shape.poly[p].join[2]].x-trans.point[shape.poly[p].join[1]].x,
		y32=trans.point[shape.poly[p].join[2]].y-trans.point[shape.poly[p].join[1]].y;

	float bz=y21*x32-x21*y32;

	return(bz>=0);
}



/****************************************************************************/
