/*
    tergen - terrain generator for freeciv (2.6-3.1)
		Simulates planet creation, plate tectonics, weather & erosion,
		hoping to arrive at interesting or realistic terrains.

		Inputs:
		map size (x,y)              64,64
		land percentage             30
		hill/mountain percentage    30
		tempered percentage         60
		water on land percentage    50

		Later:
		wrap EW                     y
		wrap NS                     
    hexagonal
*/

/*
See https://freeciv.fandom.com/wiki/Map_format
*/


/*****************
Freeciv topologies & tile neighbourhoods
******************

Sample terrain array. 3x6 microworld
"a" on (0,0), "b" on (1,0), "d" on (0,1)
t00="abc"
t01="def"
t02="ghi"
t03="jkl"
t04="mno"
t05="pqr"

(0,0) is top left tile. x increase to the right, y increase down


Square nonISO topo 0
=============
(0,0) to (1,1) is 45 degrees for geometrical purposes. But it is a down direction on the display, as tiles have a diamond orientation.

"a" is the top corner in the rendering, "c" is right corner.
abc      ..***..   Asteroid impact, 7×7
def      .*...*.
ghi      *.....*
jkl      *..*..*
mno      *.....*
pqr      .*...*.
         ..***..
h neighbours: egik (with diagonals: defgijkl)
Directions: 0,90,180,270
Diagonals: 45,135,225,315 

Tile neighbour offsets for odd & even lines:
(-1,0) (1,0) (0,-1) (0,1)
May also use diagonal neigbours:
(-1,-1) (-1, 1) (1, -1) (1, 1)

|----->  x' = x
|
|
v y' = y
Distance is pythagorean: sqrt(dx*dx+dy*dy)
Squarede dist: dx*dx+dy*dy


square iso topo 1
==========
Each tile a diamond, closest neighbours diagonally
a   b   c       .   *   *   *   .    Asteroid impact, 5×9
  d   e   f       *   .   .   *   .  Centered on an EVEN line
g   h   i       *   .   .   .   * 
  j   k   l       .   .   .   .   .
m   n   o       *   .   *   .   * 
  p   q   r       .   .   .   .   .
                *   .   .   .   * 
                  *   .   .   *   .
                .   *   *   *   . 


h neighbours: dejk (with diagonals: bdegijkn)
k neighbours: hino (with diagonals: ehijlnoq)
Directions: 45,135,225,315
Diagonals: 0,90,180,270

tile neighbours  odd lines: ( 0,-1) (1,-1) (0, 1) (1, 1)
tile neighbours even lines: (-1,-1) (0,-1) (-1,1) (0, 1)

odd & even diagonals: (0,-2) (-1,0) (1,0) (0,2)
game         array indices xy    game indices gxgy
             00  10  20  30           00
    /\         01  11  21  31       01  10     
gy|/  \| gx  02  12  22  32       02  11  20
               03  13  23  33   03  12  21 30
	                                13  22  31
Distances
---------
The coordinate system is rotated 45 degrees. 
A step in x direction adds 1 to gx and subtract 1 from gy
2 steps in y direction add 1 to gy, and add 1 to gx
1 y-step from even y: add 1 to gx
1 y-step from odd y: add 1 to gy

game dist is sqrt(dx' * dx'   +   dy' * dy')
for even y:
dx' =  dx + (dy+1)/2   
dy' = -dx + dy/2
for odd y:
dx' =  dx + dy/2
dy' = -dx + (dy+1)/2

Combined:
gdx = dx + (dy + (~y & 1))/2
gdy = -dx + (dy + (y & 1))/2


hex nonISO topo 2
==========
The hexagons are flat (connecting) on the sides, corners point up and down.
a b c       ◦ ◦ • • • • ◦    7×7 asteroid strike, centered on ODD line
 d e f       ◦ • ◦ ◦ ◦ • ◦   If the asteroid strike diagram looks bad, get a proper unicode font
g h i       ◦ • ◦ ◦ ◦ ◦ •    for viewing this. Ascii is not enough, and never was.
 j k l       • ◦ ◦ • ◦ ◦ •   If you REALLY want to display this on a VT100, use sw that
m n o       ◦ • ◦ ◦ ◦ ◦ •    substitutes characters for you.
 p q r       ◦ • ◦ ◦ ◦ • ◦  
            ◦ ◦ • • • • ◦
h neighbours: degijk
k neighbours: hijlno
Directions: 0,60,120,180,240,300

Tile neighbours  odd lines: (0,-1)  (1,-1) (-1,0) (1,0) (0,1)  (1,1)
Tile neighbours even lines: (-1,-1) (0,-1) (-1,0) (1,0) (-1,1) (0,1)

distances
a to b equals a to d and b to d this distance is "1.0"
each row is sqrt(3)/2 lower than the previous, and centers offset 1/2 to the side

offset from even y:
gdx = dx + (0.5, if dy is odd)
gdy = sqrt(3)/2 * dy

offset from odd y:
gdx = dx - (0.5, if dy is odd)
gdy = sqrt(3)/2 * dy


hex ISO topo 3
=======
hexagons are flat (connecting) above and below, corners point to the sides.
a   b   c       ◦     ◦     •     ◦     Asteroid strike, 4×13  
  d   e   f        ◦     •     •     ◦  Centered on EVEN line
g   h   i       ◦     •     ◦     •       
  j   k   l        •     ◦     ◦     •   
m   n   o       ◦     ◦     ◦     ◦       
  p   q   r        •     ◦     ◦     •   
                ◦     ◦     •     ◦        
                   •     ◦     ◦     • 
                ◦     ◦     ◦     ◦       
                   •     ◦     ◦     •  
                ◦     •     ◦     •      
                   ◦     •     •     ◦   
                ◦     ◦     •     ◦       


h neighbours: bdejkn
k neighbours: ehinoq
Directions: 30, 90, 150, 210, 270, 330

Tile neighbours  odd lines: (0,-2) (0,-1)  (1,-1) (0,1)  (1,1) (0,2)
Tile neighbours even lines: (0,-2) (-1,-1) (0,-1) (-1,1) (0,1) (0,2)

a-g, a-d and g-d has distance 1.0
a-b has distance sqrt(3)
Adding 2 to y, increase gdy by one.
Adding 1 to x, increase gdx by sqrt(3).
Even y:
adding 1 to y, increase gdy by 0.5, and gdx by sqrt(3)/2
Odd y: 
adding 1 to y, increase gdy by 0.5, and subtract sqrt(3)/2 from gdx
So, gdy = dy/2 in all cases.
Even y:
gdx = dx*sqrt(3) + sqrt(3)/2
Odd y:
gdx = dx*sqrt(3) - sqrt(3)/2
*/

/*
 Prevailing winds: see https://en.wikipedia.org/wiki/Prevailing_winds
 35 deg s to 35 deg n: east to west, but also somewhat towards equator
 air returns at 12-15km height

 35 deg to 65 deg: west to east
 Northern hemisphere: sw
 Southern hemisphere: nw

 North pole 65-90: weak se
 South pole 65-90: weak ne 

 sea breeze (sea->land) when land is hotter
 land breeze (land->sea) when land is colder

 little wind in the 30-35 degree area where opposing directions meet. Here and at the poles, air comes down.

 At equator and at 60-65, the air rise. High up, (8km?) the wind direction is opposite.

*/

/*
Temperature map:
sea: 
  negative at the poles, so a sea pole will freeze in a suitable radius. 
	+25 or so at equator, with a gradient towards the poles

land:
  -20 or so at poles, +50 at equator, gradient. Lower temperatures up in mountains.
	So, greenland will be ice, even if the sea isn't frozen there.

Several averaging passes. This way, land and sea will influence each other. 


Without y-wrap: N is up, S is down, the poles are narrow bands. (width at least 2 with ISO)

with wrap: round poles of some suitable size. 

*/
/*
  Weather simulations for two topologies:
	Weather simulations assign temperatures and prevailing wind directions to tiles. Temperature is a function of latitude. Wind is easterly or westerly, also depending on latitude.

  This is easy on a map without WRAPY. The poles are stripes at the top and botttom of the array, the y coordinate is the latitude. WRAPX does not matter.

	On a WRAPX|WRAPY map, the rule is the same. Latitude matters. But there is usually two round poles, so where is the equator and other specific latitudes?

	The poles has to be maximally far apart. For example, one centered on the middle of the map, and one centered on the maxX maxY corner. With the wrapping, nobody sees that one pole is split in four.

	For any point on this map, calculate distance to both poles, using the shortest straight line. This will give a useable "latitude". If both poles are equally far away, we are on the equator. And so on, for other latitudes. "East/west" is perpendicular to the vector to the closest pole. So, temperatures can be assigned based on latitude, and so can direction for the prevailing wind.

	 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define log2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))


#ifdef INTERNAL_SINCOSF
/*
	Apparently, some don't have sincosf
	This implementation is no better than just calling sinf() and cosf()
	Doing better is up to the math library vendor.

	To use this, compile with -DINTERNAL_SINCOSF
	 */
void my_sincosf(float x, float *sinx, float *cosx) {
	*sinx = sinf(x);
	*cosx = cosf(x);
}

#define sincosf(a,b,c) my_sincosf(a,b,c)
#endif


typedef struct {
	short height; //Meters above lowest sea bottom. Range 0–10000
	char terrain; //Freeciv terrain letter
	char plate;   //id of tectonic plate this tile belongs to
	char mark; //for depth-first search, volcano calculations etc.
	char river; //for river assignment during map output
	char lake_ix; //If tile is a lake '+', index into lake table
	char lowestneigh; //direction of lowest neighbour
	char steepness; //1+log2 of height difference to lowest neighbour. -1 if unset
	char temperature; //in celsius
	int wetness; //rainfall adds, evaporation subtracts, river runoff subtracts
	int waterflow; //amount in rivers. Wetness running off, plus incoming flow from higher tiles. Sum of incoming waterflows and runoff from wetness
	char oldflow; //fourth root of prev. flow. Used for re-routing rivers
	int rocks; //erosion, rocks that may follow the rivers and become sediment
	int erosion; //Erosion, deferred to the next round
	int rockflow;
} tiletype;


typedef struct {
	signed char sea_temp;  //in celsius
	signed char land_temp; //At 0m above sea
	char prevailing1; //index into neighbourtype array. Direction of prevaling wind
	char prevailing2; //second prevailing wind direction
	char prevailing_strength; //1, 2, 3
} weatherdata;

//Simple weather simulation need only one parameter, 
//the water content in a volume of air.
typedef struct {
	int water; //water content in the air
	int new;   //cloudwater recently moved by wind
} airboxtype;
//Air layers: 50m, 100m, 200m, 500m, 1000m, 2000m, 5000m, 10000m, 20000m

typedef struct {
	float cx, cy; //Position of plate center
	float ocx, ocy;//Old center
	float vx, vy; //Plate velocity, in tiles/round
	int rx, ry;   //max tile dist from cx or cy, limits searches
	char ix;      //plate number
} platetype;

typedef struct {
	short dx, dy; //Add (dx,dy) to (x,y), to find the neighbour tile to (x,y) 
} neighbourtype;

typedef struct {
	int angle;    //Direction to neighbour tile
	float dx, dy; //distance vector to neighbour tile (computed from angle)
} neighpostype;

typedef struct {
	int outflow_x, outflow_y; //rivertile handling exit from this lake
	int tiles;              //number of tiles in the lake.
	int river_serial;       //for checking whether lakes were created in the same river run or not.
	short height;           //Lake height above terrain reference zero height.
	int priq_len;						//# of entries in priority queue
	tiletype **priq;        //priority queue (array heap), tile pointers
	int merged_into;        //-1, or id of lake it merged into. Subject to path compression
} laketype;

#define MAX_LAKES 127
#define MAX_PRIQ 20000

//Globals
int mapx, mapy; //Map dimensions
int topo; //map topology 0:square, 1:square iso, 2:hex, 3:hex iso
int tileset; //0=normal. Otherwise, extended tileset toonhex+ where lowland
             //tiles also have a hill form. (arctic hill, desert hill, ...)
int wrapmap; //0:no map wrap, 1: x-wrap 2:xy-wrap

char *wraptxt[3] = {"", "WRAPX", "WRAPX|WRAPY"};
char *topotxt[4] = {"", "ISO", "HEX", "ISO|HEX"};

char paramtxt[1024]; //parameter list
char *nametxt;

int airheight[9] = {50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
int rounds;

/*  odd & even neighbour arrays for the topologies  */
//neighbourtype n0o[] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
//neighbourtype n0e[] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};

neighbourtype n0o[] = {{0,1},{1,1},{1,0},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}};
neighbourtype n0e[] = {{0,1},{1,1},{1,0},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}};

//neighbourtype n1o[] = {{ 0,-1},{1,-1},{ 0,1},{1, 1},{0,-2},{-1,0},{0,2},{0,1}};
//neighbourtype n1e[] = {{-1,-1},{0,-1},{-1,1},{0, 1},{0,-2},{-1,0},{0,2},{0,1}};

neighbourtype n1o[] = {{0,1},{1, 1},{0,2},{ 0,1},{-1,0},{ 0,-1},{0,-2},{1,-1}};
neighbourtype n1e[] = {{0,1},{0, 1},{0,2},{-1,1},{-1,0},{-1,-1},{0,-2},{0,-1}};

//neighbourtype n2o[] = {{ 0,-1},{1,-1},{-1,0},{1,0},{ 0,1},{1,1}};
//neighbourtype n2e[] = {{-1,-1},{0,-1},{-1,0},{1,0},{-1,1},{0,1}};

neighbourtype n2o[] = {{ 1, 0},{ 1, 1},{ 0, 1},{-1, 0},{ 0,-1},{ 1,-1}};
neighbourtype n2e[] = {{ 1, 0},{ 0, 1},{-1, 1},{-1, 0},{-1,-1},{ 0,-1}};

//neighbourtype n3o[] = {{0,-2},{ 0,-1},{1,-1},{ 0,1},{1,1},{0,2}};
//neighbourtype n3e[] = {{0,-2},{-1,-1},{0,-1},{-1,1},{0,1},{0,2}};

neighbourtype n3o[] = {{1,1},{0,2},{ 0,1},{ 0,-1},{0,-2},{1,-1}};
neighbourtype n3e[] = {{0,1},{0,2},{-1,1},{-1,-1},{0,-2},{0,-1}};

neighbourtype *nodd[4] = {n0o, n1o, n2o, n3o};
neighbourtype *nevn[4] = {n0e, n1e, n2e, n3e};

/*  geometric position of neighbour tiles for the topologies  */
//neighpostype np0[8] = {{180},{90},{270},{0},{225},{135},{315},{45}};
neighpostype np0[8] = {{0},{45},{90},{135},{180},{225},{270},{315}};

//neighpostype np1[8] = {{225},{315},{135},{45},{270},{180},{90},{0}};
neighpostype np1[8] = {{0},{45},{90},{135},{180},{225},{270},{315}};

//neighpostype np2[6] = {{240},{300},{180},{  0},{120},{ 60}};
neighpostype np2[6] = {{  0},{ 60},{120},{180},{240},{300}};

//neighpostype np3[6] = {{270},{210},{330},{150},{ 30},{ 90}};
neighpostype np3[6] = {{ 30},{ 90},{150},{210},{270},{330}};

neighpostype *nposition[4] = {np0, np1, np2, np3};

/*  This many tile neighbours in each topology */
int neighbours[4] = {8,8,6,6}; //8,4,6,6 ?

/* Heightmap changes due to asteroid strike */
short chicxulub[4][13][7] = {
	{ {    0, 1000, 6000, 6000, 6000, 1000,    0}, //topology 0, 7×7
		{ 1000, 6000,-2000,-3000,-2000, 6000, 1000},
		{ 6000,-2000,-3000,-3000,-3000,-2000, 6000},
		{ 6000,-3000,-3000, 7000,-3000,-3000, 6000}, //even or odd
		{ 6000,-2000,-3000,-3000,-3000,-2000, 6000},
		{ 1000, 6000,-2000,-3000,-2000, 6000, 1000},
		{    0, 1000, 6000, 6000, 6000, 1000,    0}
	},
	{ {    0, 6000, 6000, 6000,    0},  //topology 1, 5×9
		{ 6000,-3000,-3000, 6000,    0},
		{ 6000,-3000,-3000,-3000, 6000},
		{-3000,-3000,-3000,-3000,    0},
		{ 6000,-3000, 7000,-3000, 6000},  //EVEN line in tile[][]
		{-3000,-3000,-3000,-3000,    0},
		{ 6000,-3000,-3000,-3000, 6000},
		{ 6000,-3000,-3000, 6000,    0},
		{    0, 6000, 6000, 6000,    0}
	},
	{ {    0,    0, 5000, 6000, 6000, 5000,    0},   //topology 2,  7×7
		{    0, 6000,-2000,-3000,-2000, 6000,    0},
		{    0, 6000,-3000,-3000,-3000,-3000, 6000},
		{ 5000,-2000,-3000, 7000,-3000,-2000, 5000},   //ODD line in tile[]
		{    0, 6000,-3000,-3000,-3000,-3000, 6000},
		{    0, 6000,-2000,-3000,-2000, 6000,    0},
		{    0,    0, 5000, 6000, 6000, 5000,    0}
	},
	{ {    0,    0, 5000,    0},   //topology 3,  4×13
		{    0, 6000, 6000,    0},
		{    0, 6000,-2000, 6000},
		{ 5000,-3000,-3000, 5000},
		{    0,-2000,-3000,-2000},
		{ 6000,-3000,-3000, 6000},
		{    0,-3000, 7000,-3000},   //EVEN line in tile[]
		{ 6000,-3000,-3000, 6000},
		{    0,-2000,-3000,-2000},
		{ 5000,-3000,-3000, 5000},
		{    0, 6000,-2000, 6000},
		{    0, 6000, 6000,    0},
		{    0,    0, 5000,    0}
	}
};

/* Arrays indexed by topology number */
int asteroidx[4] = {7, 5, 7, 4};     //columns in strike map
int asteroidy[4] = {7, 9, 7, 13};    //rows in strike map
int asteroid_yadj[4] = {0, 2, 1, 2}; //0: any y, 1: odd y, 2: even y


void fail(char *s) {
	printf("%s\n", s);
	exit(1);
}

void percentcheck(int const x) {
	if (x < 0 || x > 100) fail("Percentages must be in the 0-100 range.");
}

//Finds square of distance between two points.
//Shortest distance, using wrapx, wrapy, both or none.
int sqdist(int const x1, int const y1, int const x2, int const y2) {
	int dx, dy;
	//Check if wrapping gives shorter dist. The sign matters for some of the conversions!
	dx = x1-x2; 
	if (dx > mapx/2) dx -= mapx;
	else if (dx < -mapx/2) dx += mapx;

	dy = y1-y2;
	if (dy > mapy/2) dy -= mapy;
	else if (dy < -mapy/2) dy += mapy;

	switch (topo) {
		case 0: //Ordinary square grid. game x,y and array x,y is the same
		default: 
			return dx*dx + dy*dy;
		case 1: //square with ISO, game x,y computed from array x,y
			{
				int gdx =  dx + (dy + (~y1 & 1))/2; 
				int gdy = -dx + (dy + ( y1 & 1))/2;
				return gdx * gdx + gdy * gdy;
			}
		case 2: //hex non-iso
			{
				float gdy = dy * fsqrt(3) / 2; 
				float gdx = dx;
				if (dy & 1) gdx += (y1 & 1) ? -0.5 : 0.5;
				return gdx*gdx + gdy*gdy;
			}
		case 3: //hex iso
			{
				float gdy = (float)dy / 2;
				float gdx = dx * sqrtf(3);
				gdx += y1 & 1 ? -sqrtf(3)/2 : sqrtf(3)/2;
				return gdx*gdx + gdy*gdy;
			}
	}
}

/* Random number in a range (inclusive) */
float frand(float min, float max) {
	float range = max - min;
	return min + random() * range / RAND_MAX;
}

//Recover (x,y) from a pointer into the tile array
void recover_xy(tiletype tile[mapx][mapy], tiletype *t, int *x, int *y) {
	int diff = t - &tile[0][0];
	*y = diff % mapy;
	*x = diff / mapy;
}

/* Make a tectonic plate, not too close to other plates. */
int mkplate(int const plates, int const ix, platetype plate[plates], int platedist) {
	//Pick a random position, then retry if too close to some other plate.
	//Try many times. Giving up then is ok, we'll go with fewer plates.
	int tries = 25;
	//Correct with ⅔, or we get very few plates
	int sq_p_dist = platedist * platedist * 2 / 3;
	float movedist = (platedist/2.0 + 1)/rounds;
	while (--tries) {
		float x = frand(0, mapx); 
		float y = frand(0, mapy); 
		//Distance test
		for (int i = 0; i < ix; ++i) if (sqdist(x,y,plate[i].cx,plate[i].cy) < sq_p_dist) goto tryagain;
		plate[ix].cx = plate[ix].ocx = x;
		plate[ix].cy = plate[ix].ocy = y;
		plate[ix].ix = ix + 1;
		plate[ix].vx = frand(-movedist,movedist);
		plate[ix].vy = frand(-movedist,movedist);
		plate[ix].rx = plate[ix].ry = 0;
		break;
		tryagain:;
	}
	return tries;
}

//Debug, see the tectonic plates
void print_platemap(tiletype tile[mapx][mapy]) {
	char *sym="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.";
	for (int y = 0; y < mapy; ++y) {
		for (int x = 0; x < mapx; ++x) {
  		char s = tile[x][y].plate;
			if (s>62) s = 62;
			printf("%c", sym[s]);
		}
		printf("\n");
	}	
}


//Wraps a coordinate so it stays within 0..size-1
int wrap(int q, int size) {
	if (q < 0) return q + size;
	if (q >= size) return q - size;
	return q;
}

//Comparison function for qsort, sort tiles by height
int q_compare_height(void const *p1, void const *p2) {
	tiletype const *tp1 = *(tiletype **)p1;
	tiletype const *tp2 = *(tiletype **)p2;
	return (int)tp1->height - (int)tp2->height;
}

//Comparison function for qsort, sort tiles by wetness
int q_compare_wetness(void const *p1, void const *p2) {
	tiletype const *tp1 = *(tiletype **)p1;
	tiletype const *tp2 = *(tiletype **)p2;
	return (int)tp1->wetness - (int)tp2->wetness;
}

//Comparison function for qsort, sort tiles by waterflow
int q_compare_waterflow(void const *p1, void const *p2) {
	tiletype const *tp1 = *(tiletype **)p1;
	tiletype const *tp2 = *(tiletype **)p2;
	return tp1->waterflow - tp2->waterflow;
}

//Comparison function for qsort, sort tiles by temperature
int q_compare_temperature(void const *p1, void const *p2) {
	tiletype const *tp1 = *(tiletype **)p1;
	tiletype const *tp2 = *(tiletype **)p2;
	return tp1->temperature - tp2->temperature;
}

//Test for sea, deep or shallow 
bool is_sea(char c) {
	return (c == ' ') | (c == ':');
}

//Test for arctic terrain. 
bool is_arctic(char c) {
	return (c == 'a') | (c == 'A');
}

//Test for mountain
bool is_mountain(char c) {
	return (c == 'm') | (c == 'v');
}

//Test if a tile is wet. Sea, lake, or land with river on it
bool is_wet(tiletype *t) {
	return (t->terrain == ' ') | (t->terrain == ':') | (t->terrain == '+') | (t->river != 0);
}

//counts sea neighbours around [x][y].  The central tile is not counted
int seacount(int x, int y, tiletype tile[mapx][mapy]) {
	int cnt = 0;
	neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
	for (int n = 0; n < neighbours[topo]; ++n) {
		if (is_sea(tile[wrap(x+nb[n].dx, mapx)][wrap(y+nb[n].dy, mapy)].terrain)) ++cnt;
	}
	return cnt;
}

//Pieces of sea smaller than this, becomes lakes instead:
#define MAX_LAKE 12

char dfs_mark; //1 or 0
int dfs_cnt;
//Depth-first search to find the size of an ocean.
void dfs_lake(int x, int y, tiletype tile[mapx][mapy], int mklake) {
	if (tile[x][y].mark == dfs_mark) return;
	tile[x][y].mark = dfs_mark;
	if (mklake) tile[x][y].terrain = '+';
  if (++dfs_cnt > MAX_LAKE) return;
	neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
	for (int n = 0; n < neighbours[topo]; ++n) {
		int nx = wrap(x+nb[n].dx, mapx);
		int ny = wrap(y+nb[n].dy, mapy);
		if (is_sea(tile[nx][ny].terrain)) dfs_lake(nx, ny, tile, mklake);
	}
}

void start_dfs_lake(int x, int y, tiletype tile[mapx][mapy]) {
	dfs_cnt = 0;
	dfs_mark = 1;
	dfs_lake(x, y, tile, 0);
	if (dfs_cnt > MAX_LAKE) {
		dfs_cnt = 0;
		dfs_mark = 0;
		dfs_lake(x, y, tile, 0); //reset marks
	} else {
		dfs_cnt = 0;
		dfs_mark = 0;
		dfs_lake(x, y, tile, 1); //reset marks, make lake
	}
}

//Convert lake to shallow sea, because it touches sea.
//Recursively, so nothing remains
//Should not be needed, not called any more.
void lake_to_sea(int x, int y, tiletype tile[mapx][mapy]) {
	if (tile[x][y].terrain == '+') {
		tile[x][y].terrain = ' ';
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		for (int n = 0; n < neighbours[topo]; ++n) {
			x = wrap(x + nb[n].dx, mapx);
			y = wrap(y + nb[n].dy, mapy);
			lake_to_sea(x, y, tile);
		}
	}
}

/*
		Fixups. 
		 - Small landlocked pieces of sea become lakes
		 - some single-tile islands become shallow sea
		 - some single-tile islands grows
		 - deep sea with more than one land neighbours become shallow 
		 - lake connected to sea become shallow sea
		 - river in the ice removed, unless there is a non-arctic neighbour (ice edge may have river)
		 - volcanoes sometimes spread to neighbour mountain tiles
		 - volcanoes affect neighbour terrain
		 - thin too dense river grids

Using for x.. for y... loops to do fixups, may impose a pattern on the terrain, a pattern
depending on the scanning direction. This happens when fixups depend on neighbour tiles that
got "fixed" in the previous step. Thinning of dense river grids have this problem: instead of
an unsightly dense grid, we get a regular pattern.

Imposed patterns are avoided by randomizing the tile order. A simple way is to access
the tiles through tp[], instead of tile[][].  At this point, tp is sorted. First on
height, then parts are sorted on temperature & wetness. Finally, land tiles are re-sorted
on waterflow. The tile order is then very different from the order in tile[][].

To further randomize, step through tp[] with large jumps, similar to the search order
in a hash table. It is then nexessary to find a step size relative prime to mapx*mapy,
which is easy enough.

Using the sorted tp has other advantages too. Deep sea correction should not iterate through land tiles.
Land corrections should not iterate through sea. Better performance...
tp[] order at this point:
deep sea tiles
shallow sea tiles
land sorted on waterflow
	 */
void terrain_fixups(tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy], int seatiles, int deepsea, int landtiles) {
	int const neighcount = neighbours[topo];
	int n;

	//Reset mark on sea tiles, start_dfs_lake() depends on that
	tiletype **tt = &tp[seatiles];
	while (tt-- != tp) (*tt)->mark = 0;

	//Get rid of single-tile islands, except unbuildable ones. (Avoid cities with no land around them)
	tt = &tp[mapx*mapy];
	tiletype **last = &tp[seatiles];
	while (tt-- != last) {
		int x, y;
		recover_xy(tile, *tt, &x, &y);
		tiletype *const t = *tt;
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		if (!is_sea(t->terrain) && !is_arctic(t->terrain) && !is_mountain(t->terrain)) {
			for (n = 0; n < neighcount; ++n) if (!is_sea(tile[wrap(x+nb[n].dx, mapx)][wrap(y+nb[n].dy, mapy)].terrain)) break;
			if (n == neighcount) {
				//Double the island, or drown it. Either way avoids single tile islands
				int num = random() % (neighcount*2);
				if (num >= neighcount) {
					t->terrain = ' '; //Drown the island
					t->river = 0;
					t->mark = 0; //Sea tiles must not be marked
				} else {
					int nx = wrap(x+nb[num].dx, mapx);
					int ny = wrap(y+nb[num].dy, mapy);
					tile[nx][ny].terrain = t->terrain; //Raise a random neighbour. Ought to not plug a fjord doing this...
				}
			}
		}
	}

	//if a lake touches sea:
	//lake_to_sea(x, y, tile)
	//improved erosion code means this don't happen, so no need for lake_to_sea() now.

	//Fix some (but not all!) cases of deep sea next to land
	//Turn small pieces of sea into lakes instead.

	tt = &tp[seatiles];
	while (tt-- != tp) {
		int x, y;
		recover_xy(tile, *tt, &x, &y);
		tiletype * const t = *tt;
		if (is_sea(t->terrain)) { //test, as a handful may have changed...
			//Find deep sea next to land, and small lakes
			int seacnt = seacount(x, y, tile);
			int landcnt = neighcount - seacnt;
			if (seacnt == 0) t->terrain = '+'; //Single-tile sea->lake. Ought to run a river to near sea, if possible.
			else if (landcnt >= 2) {
				t->terrain = ' '; //Make it shallow
				//Convert to lake, if a small piece of sea is trapped:
				start_dfs_lake(x, y, tile);
			} else if (landcnt == 1 && (random() & 7)) t->terrain = ' '; //Trireme benefit
		}
	}

	/*
	River corrections. In wet swampy terrain, every tile may have a river.
	This looks like an ugly grid. The fix is to thin the grids.
	Neighboring beach tiles having river looks strange, as the game render
	a river along the beach. Fix: break it up, so we get small short rivers
	going straight to sea.
  It is important not to break up long rivers.

	Old correction: a rivertile surrounded by river tiles, or with only one non-river
	neighbour was removed.

	Better: remove if the tile is not an endpoint, and the river don't need this tile
	The rivertile is needed, if it has river on two sides and dry land on two sides.
	A rivertile where all river neighbours are connected, is usually not needed. Unless
	it is the sole connection to the sea/lake.

	Simpler: if it has two unconnected sets of dry land neighbours, then the river tile
	is needed. It is then the connection between 2 river tiles, or river/lake, or lake/sea.

	A river endpoint is also needed, even though it don't connect disjoint water.
	Endpoints have less than 2 river neighbours.

	So keep those with <2 river neighbours, or >=2 disjoint dry neighbour sets.

	ice correction: no river on arctic, unless there is a non-arctic neighbour with river/sea
	In this case, a 'm' with low temperature IS arctic.

	nonhex topologies: don't consider corner neighbours, no diagonal rivers! (even neighbours)
	hex topologies: all neighbours

	*/
	tt = &tp[mapx*mapy];
	last = &tp[seatiles];
	int n_inc = (topo < 2) ? 2 : 1; //Rivers don't have diagonal neighbours
	while (--tt != last) {
		tiletype *t = *tt;
		if (!t->river) continue;
		int x, y;
		recover_xy(tile, t, &x, &y);
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		int last_nb = neighbours[topo] - n_inc;
		bool prev_dry = !is_wet(&tile[wrap(x+nb[last_nb].dx,mapx)][wrap(y+nb[last_nb].dy,mapy)]);
		int river_neigh = 0; //simple count of neighbour tiles with river on them
		int dry_neigh = 0;   //count of disjoint neighbours with dry land
		for (int n = 0; n < neighbours[topo]; n += n_inc) {
			tiletype *tnb = &tile[wrap(x+nb[n].dx,mapx)][wrap(y+nb[n].dy,mapy)];
			river_neigh += tnb->river;
			bool dry = !is_wet(tnb);
			dry_neigh += (dry && !prev_dry);
			prev_dry = dry;
		}
		if (river_neigh >= 2 && dry_neigh < 2) t->river = 0;
	}
}


//Sorts the tiles on height, determining the sea level because
//x% of the tiles are sea, so the last sea tile gives the sea height.
//Also determine tile temperatures based on being sea or land
//Done every round, as erotion & tectonics change tile heights & sea level
short sealevel(tiletype *tp[mapx*mapy], int land, tiletype tile[mapx][mapy], weatherdata weather[mapx][mapy]) {
	//Find the sea level by sorting on height. "land" is the percentage of land tiles
	int tilecnt = mapx*mapy;
	qsort(tp, tilecnt, sizeof(tiletype *), &q_compare_height);
	int landtiles = land * tilecnt / 100;
	int seatiles = tilecnt - landtiles;
	short level = tp[seatiles ? seatiles-1 : 0]->height;
	//Update the temperature array, based on sea or land status
	//assign sea/land status
	for (int i = 0; i < seatiles; ++i) {
		tp[i]->terrain = ':';
		tp[i]->wetness = 1000; //In case the tile surfaces later, avoid too much fake wetness
		tp[i]->lake_ix = -1;   //Avoid lake remnants in the sea
	}
	for (int i = seatiles; i < tilecnt; ++i) {
		if (tp[i]->terrain != '+') tp[i]->terrain = 'm';
	}
	//temperatures. Land temperatures fall with elevation. About 10C per km up
  for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		if (tile[x][y].terrain == ':') tile[x][y].temperature = weather[x][y].sea_temp;
		else tile[x][y].temperature = weather[x][y].land_temp - (tile[x][y].height-level)/100;
	}
	//two rounds of weighted averaging. Sea may thaw slightly frozen land, very cold land may freeze some sea.
	signed char tmptemp[mapx][mapy];
	int half = (neighbours[topo]+2)/2;
	memset(tmptemp, 0, mapx*mapy*sizeof(signed char));
	for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		int sum = 2 * (int)tile[x][y].temperature;
		for (int n = 0; n < neighbours[topo]; ++n) {
			int nx = wrap(x + nb[n].dx, mapx);
			int ny = wrap(y + nb[n].dy, mapy);
			sum += tile[nx][ny].temperature;
		}
		if (sum < 0) sum -= half; else sum += half; //Ensure correct rounding		
		tmptemp[x][y] = sum / (neighbours[topo] + 2);
	}
	for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		int sum = 2 * tmptemp[x][y];
		for (int n = 0; n < neighbours[topo]; ++n) {
			int nx = wrap(x + nb[n].dx, mapx);
			int ny = wrap(y + nb[n].dy, mapy);
			sum += tmptemp[nx][ny];
		}
		if (sum < 0) sum -= half; else sum += half;
		tile[x][y].temperature = sum / (neighbours[topo] + 2);
	}
	return level;
}

//Set a terrain type, unless it is already a lake '+'
void set(char *t, char ttyp) {
	if (*t == '+') return;
	*t = ttyp;
}

void set_parts(int tempered, int wateronland, float *dp, float *pp, float *gp, float *fp, float *jp, float *sp, float *psum) {
	*dp = 80.0 * tempered/100.0 * (100.0-wateronland)/100.0; // 0–80, 20 normal
	float pg_part = 40.0;
	float fj_part = 20.0;
	*sp = 40.0 * wateronland/100.0; //0–40, 20 normal
	*fp = fj_part * (100.0-tempered) / 100.0;
  *jp = fj_part - *fp;
	*pp = pg_part * (100.0-wateronland) / 100.0;
	*gp = pg_part * wateronland / 100.0;
	*psum = *dp + *pp + *gp + *fp + *jp + *sp;
}

void assign_rivers(tiletype **tp, int seatiles, int landtiles, int wateronland) {
	qsort(tp + seatiles, landtiles, sizeof(tiletype *), &q_compare_waterflow);
	int limit, j = 0;
	int rivertiles = landtiles * wateronland / 200; //More than 50% river tiles is useless anyway
	int nonrivers = landtiles - rivertiles;
	for (limit = seatiles + nonrivers; j < limit; ++j) {
		tp[j]->river = 0;
	}
	for (limit += rivertiles; j < limit; ++j) {
		tp[j]->river = (tp[j]->terrain != '+' && tp[j]->waterflow > 0) ? 1 : 0; 
	}	
}

void output_terrain(FILE *f, tiletype tile[mapx][mapy], bool extended_terrain) {
	//pre-terrain stuff
	fprintf(f, "[scenario]\n");
	fprintf(f, "game_version=3000100\n");
	fprintf(f, "is_scenario=TRUE\n");
	fprintf(f, "name=\"%s\"\n", nametxt);
	fprintf(f, "description=\"Made by the tergen terrain generator\\n\\nInvocation:\\n%s\"\n",paramtxt);
	fprintf(f, "save_random=FALSE\n");
	fprintf(f, "players=FALSE\n");
	fprintf(f, "startpos_nations=FALSE\n");
	fprintf(f, "lake_flooding=TRUE\n");
	fprintf(f, "ruleset_locked=TRUE\n");
	fprintf(f, "ruleset_caps=\"+std-terrains\"\n");

  fprintf(f, "[savefile]\n");
	fprintf(f, "options=\" +version3\"\n");
	fprintf(f, "rulesetdir=\"%s\"\n", extended_terrain ? "hh" : "civ2civ3");
	fprintf(f, "version=40\n");
	fprintf(f, "reason=\"Scenario\"\n");
	fprintf(f, "revision=\"3.0.1\"\n");
	fprintf(f, "technology_size=88\n");
	fprintf(f, "technology_vector=\"A_NONE\",\"Advanced Flight\",\"Alphabet\",\"Amphibious Warfare\",\"Astronomy\",\"Atomic Theory\",\"Automobile\",\"Banking\",\"Bridge Building\",\"Bronze Working\",\"Ceremonial Burial\",\"Chemistry\",\"Chivalry\",\"Code of Laws\",\"Combined Arms\",\"Combustion\",\"Communism\",\"Computers\",\"Conscription\",\"Construction\",\"Currency\",\"Democracy\",\"Economics\",\"Electricity\",\"Electronics\",\"Engineering\",\"Environmentalism\",\"Espionage\",\"Explosives\",\"Feudalism\",\"Flight\",\"Fusion Power\",\"Genetic Engineering\",\"Guerilla Warfare\",\"Gunpowder\",\"Horseback Riding\",\"Industrialization\",\"Invention\",\"Iron Working\",\"Labor Union\",\"Laser\",\"Leadership\",\"Literacy\",\"Machine Tools\",\"Magnetism\",\"Map Making\",\"Masonry\",\"Mass Production\",\"Mathematics\",\"Medicine\",\"Metallurgy\",\"Miniaturization\",\"Mobile Warfare\",\"Monarchy\",\"Monotheism\",\"Mysticism\",\"Navigation\",\"Nuclear Fission\",\"Nuclear Power\",\"Philosophy\",\"Physics\",\"Plastics\",\"Polytheism\",\"Pottery\",\"Radio\",\"Railroad\",\"Recycling\",\"Refining\",\"Refrigeration\",\"Robotics\",\"Rocketry\",\"Sanitation\",\"Seafaring\",\"Space Flight\",\"Stealth\",\"Steam Engine\",\"Steel\",\"Superconductors\",\"Tactics\",\"The Corporation\",\"The Republic\",\"The Wheel\",\"Theology\",\"Theory of Gravity\",\"Trade\",\"University\",\"Warrior Code\",\"Writing\"\n");
	fprintf(f, "extras_size=34\n");
	fprintf(f, "extras_vector=\"Irrigation\",\"Mine\",\"Oil Well\",\"Pollution\",\"Hut\",\"Farmland\",\"Fallout\",\"Fortress\",\"Airbase\",\"Buoy\",\"Ruins\",\"Road\",\"Railroad\",\"River\",\"Gold\",\"Iron\",\"Game\",\"Furs\",\"Coal\",\"Fish\",\"Fruit\",\"Gems\",\"Buffalo\",\"Wheat\",\"Oasis\",\"Peat\",\"Pheasant\",\"Resources\",\"Ivory\",\"Silk\",\"Spice\",\"Whales\",\"Wine\",\"Oil\"\n");
	fprintf(f, "action_size=44\n");
	fprintf(f, "action_vector=\"Establish Embassy\",\"Establish Embassy Stay\",\"Investigate City\",\"Investigate City Spend Unit\",\"Poison City\",\"Poison City Escape\",\"Steal Gold\",\"Steal Gold Escape\",\"Sabotage City\",\"Sabotage City Escape\",\"Targeted Sabotage City\",\"Targeted Sabotage City Escape\",\"Steal Tech\",\"Steal Tech Escape Expected\",\"Targeted Steal Tech\",\"Targeted Steal Tech Escape Expected\",\"Incite City\",\"Incite City Escape\",\"Establish Trade Route\",\"Enter Marketplace\",\"Help Wonder\",\"Bribe Unit\",\"Sabotage Unit\",\"Sabotage Unit Escape\",\"Capture Units\",\"Found City\",\"Join City\",\"Steal Maps\",\"Steal Maps Escape\",\"Bombard\",\"Suitcase Nuke\",\"Suitcase Nuke Escape\",\"Explode Nuclear\",\"Destroy City\",\"Expel Unit\",\"Recycle Unit\",\"Disband Unit\",\"Home City\",\"Upgrade Unit\",\"Paradrop Unit\",\"Airlift Unit\",\"Attack\",\"Conquer City\",\"Heal Unit\"\n");
	fprintf(f, "action_decision_size=3\n");
	fprintf(f, "action_decision_vector=\"nothing\",\"passive\",\"active\"\n");
	fprintf(f, "terrident={\"name\",\"identifier\"\n\"Inaccessible\",\"i\"\n\"Lake\",\"+\"\n\"Ocean\",\" \"\n\"Deep Ocean\",\":\"\n\"Glacier\",\"a\"\n\"Desert\",\"d\"\n\"Forest\",\"f\"\n\"Grassland\",\"g\"\n\"Hills\",\"h\"\n\"Jungle\",\"j\"\n\"Mountains\",\"m\"\n\"Plains\",\"p\"\n\"Swamp\",\"s\"\n\"Tundra\",\"t\"\n");
	if (extended_terrain) fprintf(f, "\"Arctic hills\",\"A\"\n\"Desert hills\",\"D\"\n\"Forest hills\",\"F\"\n\"Jungle hills\",\"J\"\n\"Tundra hills\",\"T\"\n\"Volcanoes\",\"v\"\n");
	fprintf(f, "}\n\n");

	fprintf(f, "[game]\n");
	fprintf(f, "server_state=\"S_S_INITIAL\"\n");
	fprintf(f, "meta_patches=\"none\"\n");
	fprintf(f, "meta_server=\"https://meta.freeciv.org/metaserver.php\"\n");
	fprintf(f, "id=\"\"\n");
	fprintf(f, "serverid=\"\"\n");
	fprintf(f, "phase_mode=\"Concurrent\"\n");
	fprintf(f, "phase_mode_stored=\"Concurrent\"\n");
	fprintf(f, "phase=0\n");
	fprintf(f, "turn=0\n");
	fprintf(f, "year=-4000\n");
	fprintf(f, "year_0_hack=FALSE\n");
	fprintf(f, "globalwarming=0\n");
	fprintf(f, "heating=0\n");
	fprintf(f, "warminglevel=8\n");
	fprintf(f, "nuclearwinter=0\n");
	fprintf(f, "cooling=0\n");
	fprintf(f, "coolinglevel=8\n");
	fprintf(f, "save_players=FALSE\n");
	fprintf(f, "save_known=FALSE\n");
	fprintf(f, "\n");

	fprintf(f, "[random]\n");
	fprintf(f, "saved=FALSE\n");
	fprintf(f, "\n");

	fprintf(f, "[script]\n");
	fprintf(f, "code=$$\n");
	fprintf(f, "vars=$$\n");
	fprintf(f, "\n");

	fprintf(f, "[settings]\n");
	fprintf(f, "set={\"name\",\"value\",\"gamestart\"\n");
	fprintf(f, "\"topology\",\"%s%s%s\",\"%s%s%s\"\n",wraptxt[wrapmap],(wrapmap && topo) ? "|" : "", topotxt[topo],wraptxt[wrapmap], (wrapmap && topo) ? "|" : "", topotxt[topo]);
	fprintf(f, "\"xsize\",%i,%i\n", mapx, mapx);
	fprintf(f, "\"ysize\",%i,%i\n", mapy, mapy);
	fprintf(f, "\"generator\",\"SCENARIO\",\"RANDOM\"\n");
	fprintf(f, "}\n");
	fprintf(f, "set_count=33\n");
	fprintf(f, "gamestart_valid=FALSE\n");
	fprintf(f, "\n");

	//print the terrain
	fprintf(f, "[map]\n");
	fprintf(f, "have_huts=FALSE\n");
	fprintf(f, "have_resources=FALSE\n");
	for (int y = 0; y < mapy; ++y) {
		fprintf(f, "t%04i=\"", y);
		for (int x = 0; x < mapx; ++x) fprintf(f, "%c", tile[x][y].terrain);
		fprintf(f, "\"\n");
	}
	fprintf(f, "startpos_count=0\n");

	//other layers. No info, but avoids warnings about incomplete map:
	for (int layer = 0; layer <= 8; ++layer) if (layer != 3) {
		for (int y = 0; y < mapy; ++y) {
			fprintf(f, "e%02i_%04i=\"", layer, y);
			for (int x = 0; x < mapx; ++x) fprintf(f, "0");
			fprintf(f, "\"\n");
		}
	} else {
		//Layer 3, where "2" is river
		for (int y = 0; y < mapy; ++y) {
			fprintf(f, "e03_%04i=\"", y);
			for (int x = 0; x < mapx; ++x) fprintf(f, tile[x][y].river ? "2" : "0");
			fprintf(f, "\"\n");
		}		
	}
}

#define T_SEAICE -1
#define T_GLACIER -6
#define T_TUNDRA 2

//Assign freeciv terrain types, and output a freeciv map
/*
Terrain assignment for normal freeciv tileset:
1. Sort the tileset on height. "land" is the percentage of land tiles, so we know land from sea
   Sea is divided into shallow and deep sea, based on height. 
	 Sufficiently cold sea freeze to polar ice.
	 The highest land tiles become mountains or hills depending on height and given percentages
	 Sufficiently cold hills becomes ice or tundra instead.
	 Remaining tiles are flat land
2. Sort the flat land on temperature. Freeze the coldest to ice or tundra, leave the rest.
   The arctic is now done, the rest is tempered or tropic. It is also flat.
3. Sort the remaining on wetness, divide it into desert, plain, grass, forest and swamp
4. Sort the forest on temperature, the hottest half is jungle
 

Handling parameters "temperate" and "wateronland":

"temperate" influence the temperature map, and therefore how much ice we get.
It also affect the amount of desert and the forest/jungle allocation. 50 is normal

"wateronland" gives twice the percentage of river tiles. Actual number will be lower, because rivers merge to prevent ugly "river on every tile in the grid". Wateronland also affect the desert/swamp balance, and p/g allocation. 50 is normal
*/
void output0(FILE *f, int land, int hillmountain, int tempered, int wateronland, tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy], weatherdata weather[mapx][mapy]) {
	int i = mapx*mapy;
	sealevel(tp, land, tile, weather); //Also sorts on height
	int landtiles = land * i / 100;
  int seatiles = i - landtiles;
	int shallowsea = seatiles/3;
	int deepsea = seatiles - shallowsea;

	int highland = hillmountain * landtiles / 100;
	int lowland = landtiles - highland;
	int mountains = highland/3;
	int hills = highland-mountains;
	int j;
	int limit;

	//These are parts, not percentages. They sum to 100 in the default case though
	float d_part, p_part, g_part, f_part, j_part, s_part, partsum;
	set_parts(tempered, wateronland, &d_part, &p_part, &g_part, &f_part, &j_part, &s_part, &partsum);

	j = 0;
  for (limit = deepsea; j < limit; ++j) tp[j]->terrain = tp[j]->temperature < T_SEAICE ? 'a' : ':';
  for (; j < seatiles; ++j) tp[j]->terrain = tp[j]->temperature < T_SEAICE ? 'a' : ' ';
	int firsthill = seatiles + lowland;
	limit = firsthill + hills;
	for (j = firsthill; j < limit; ++j) {
if (tp[j]->temperature < T_GLACIER) tp[j]->terrain = 'a';
		else if (tp[j]->temperature < T_TUNDRA) set(&tp[j]->terrain, 't');
		else set(&tp[j]->terrain, 'h');
	}
	for (;j < i; ++j) if (tp[j]->terrain == '+' && tp[j]->temperature < T_GLACIER) tp[j]->terrain='a';
	else set(&tp[j]->terrain, 'm');

	//The rest is flat, sort on temperature first
	qsort(tp + seatiles, lowland, sizeof(tiletype *), &q_compare_temperature);
	for (j = seatiles; tp[j]->temperature < T_GLACIER; ++j) tp[j]->terrain = 'a';
	for (; tp[j]->temperature < T_TUNDRA; ++j) set(&tp[j]->terrain, 't');
	int firsttempered = j;

	//Sort firsttempered to firsthill on wetness,
	//divide into desert, plain, grass, forest, jungle, swamp
	qsort(tp+firsttempered, firsthill-firsttempered, sizeof(tiletype *), &q_compare_wetness);
	int total = firsthill - firsttempered;
#ifdef DBG
	printf("First d wetness: %i\n", tp[j]->wetness);
#endif
	//int fifth = (firsthill-firsttempered) / 5;
	limit = firsttempered + (d_part/partsum)*total;
	for (; j < limit; ++j) set(&tp[j]->terrain, 'd');
#ifdef DBG
	printf("  First p wetness: %i\n", tp[j]->wetness);
#endif
	for (limit += (p_part/partsum)*total; j < limit; ++j) set(&tp[j]->terrain, 'p');
#ifdef DBG
	printf("  First g wetness: %i\n", tp[j]->wetness);
#endif	
	for (limit += (g_part/partsum)*total; j < limit; ++j) set(&tp[j]->terrain, 'g');
#ifdef DBG
	printf("First f/j wetness: %i\n", tp[j]->wetness);
#endif

	//Split the forests on temperature. The warmer part is jungle
	int forest = (f_part+j_part)/partsum * total;
	qsort(tp + limit, forest, sizeof(tiletype *), &q_compare_temperature);
	int firstswamp = limit + forest;
	for (limit += f_part/partsum*total; j < limit; ++j) set(&tp[j]->terrain, 'f');
	for (limit = firstswamp; j < limit; ++j) set(&tp[j]->terrain, 'j');
#ifdef DBG
	printf("First s wetness: %i\n", tp[j]->wetness);
#endif

	for (limit = firsthill; j < limit; ++j) set(&tp[j]->terrain, 's');
#ifdef DBG
	printf(" Last s wetness: %i\n", tp[j-1]->wetness);
#endif

	//Assign the rivers
	assign_rivers(tp, seatiles, landtiles, wateronland);

  terrain_fixups(tile, tp, seatiles, deepsea, landtiles);
	
	output_terrain(f, tile, false);

}

void set_tile(tiletype *t, char lowtype, char hilltype) {
	switch (t->terrain) {
		case '+':
			return; //Lakes remain lakes
		case 'l':
			t->terrain = lowtype;
			return;
		case 'h':
			t->terrain = hilltype;
			return;
		default:
			printf("Impossible tiletype '%c' (%i) seen. Could not assign '%c' or '%c'\n", t->terrain, t->terrain, lowtype, hilltype);
			fail("Internal error, expected only terrain types 'l', 'h' or '+' at this point.");
	}
}

/* Places a volcano at x,y.
	 Some small chance that it 'spreads' to neighbour riverless mountain tiles.
	 Volcanoes also melt ice and fertilize terrain around them. */
void place_and_spread_volcano(tiletype tile[mapx][mapy], int x, int y) {
	tile[x][y].terrain = 'v';
	neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
	for (int n = 0; n < neighbours[topo]; ++n) {
		int nx = wrap(x+nb[n].dx, mapx);
		int ny = wrap(y+nb[n].dy, mapy);
		tiletype *tnb = &tile[nx][ny];
		switch (tnb->terrain) {
			case 'm': //might spread the volcano out
				if (!tnb->river && !(random() & 7) ) place_and_spread_volcano(tile, nx, ny);
				break;
			case 'A': //melt to tundra hill
				tnb->terrain = 'T';
				break;
			case 'a': //melt to tundra
				tnb->terrain = 't';
				break;
			case 'p': //improve to grassland
				tnb->terrain = 'g';
				break;
			case 's': //improve to ggrass. Or possibly forest/jungle, the tile was wet...
				tnb->terrain = 'g';
		}
	}
}

/*

Parameters: number, initially number of mountain tiles. We want ca. mountains/25 volcanoes.
            tp: the tile array, seatiles first, then land tiles. We consider only land tiles.
To avoid interrupted rivers, don't make a volcano where there is river.

Pass 1: count eligible tiles. They are mountains (or hills), without river, and sit on a tectonic plate edge
        
        Using the number of eligible tiles and number of volcanoes, make a probability for such a tile to erupt

Pass 2: for each eligible tile, consult the random generator and possibly place a volcano.
        When placing a volcano, consider spreading it to adjacent mountain tiles.
	 */
void assign_volcanoes(tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy], int number, int seatiles) {
	int eligible = 0;
	number /= 25;
	number = !number ? 1 : number;
	//Pass 1
	tiletype **tt = &tp[mapx*mapy];
	tiletype **last = &tp[seatiles];
	while (tt-- != last) {
		tiletype *t = *tt;
		if (!t->river && (t->terrain == 'm' || t->terrain == 'h' || t->terrain == 'A' || t->terrain == 'T'
		               || t->terrain == 'F' || t->terrain == 'J' || t->terrain == 'D')) {
			//Tile is high, and no river. Check if it is on a plate edge
			int x, y;
			recover_xy(tile, t, &x, &y);
			neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
			for (int n = 0; n < neighbours[topo]; ++n) {
				int nx = wrap(x+nb[n].dx, mapx);
				int ny = wrap(y+nb[n].dy, mapy);
				if (tile[nx][ny].plate != t->plate) {
					//Tile MAY go volcanic
					++eligible;
					//swap eligible tiles last in the tp array, so we won't need to search for them in pass 2
					*tt = tp[mapx*mapy-eligible];
					tp[mapx*mapy-eligible] = t;
					break;
				}
			}
		}
	} //pass 1 done
	if (!eligible) return;
	int chance = (eligible > number) ? 16*eligible / number : 16; // *16, fixed point arithmetic
	//Pass 2
	tt = &tp[mapx*mapy];
	last = &tp[mapx*mapy - eligible];
	while (tt-- != last) {
		int x, y;
		recover_xy(tile ,*tt, &x, &y);
		if ( (random() % chance) < 16) place_and_spread_volcano(tile, x, y);
	}
}

/*
Terrain assignment for extended tile set:
1. Sort the tileset on height. 
   The land percentage decides land/sea. 
	 Sea is deep or shallow, depending on height
	 Cold sea tiles freeze to ice.
	 Land is low, hill or mountain depending on height
2. Sort the land on temperature. Sufficiently cold tiles become 
   arctic, arctic hill, tundra or tundra hill. No change to mountains.
3. Sort remaining land on wetness. Divide it into
   desert, plain, grass, forest, and swamp   (lowland)
   desert,  hill,  hill, forest, forest      (hill terrain)
4. Sort the forest part on temperature, the hotter part is jungle instead

*/
void output1(FILE *f, int land, int hillmountain, int tempered, int wateronland, tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy], weatherdata weather[mapx][mapy]) {

	int i = mapx * mapy;
	int landtiles = land * i / 100;
	int seatiles = i - landtiles;
	int deepseatiles = 2 * seatiles / 3;
	int highland = hillmountain * landtiles / 100;
	int lowland = landtiles - highland;
	int mountains = highland / 3;
	int hills = highland - mountains;

	float d_part, p_part, g_part, f_part, j_part, s_part, partsum;
	set_parts(tempered, wateronland, &d_part, &p_part, &g_part, &f_part, &j_part, &s_part, &partsum);

	//Classify terrain by height, and sometimes temperature
	//Sort on height, determine sea level, correct tile temperatures
	sealevel(tp, land, tile, weather);

	int limit, j = 0;
	//Deep sea or ice
	for (; j < deepseatiles; ++j) tp[j]->terrain = tp[j]->temperature < T_SEAICE ? 'a' : ':';
	//Shallow sea or ice
	for (; j < seatiles; ++j) tp[j]->terrain = tp[j]->temperature < T_SEAICE ? 'a' : ' ';
	int firstland = j;
	//Assign low land
	for (limit = j + lowland; j < limit; ++j) set(&tp[j]->terrain, 'l');
	//Assign hill land
	for (limit = j + hills; j < limit; ++j) set(&tp[j]->terrain, 'h');
	//Assign mountains
	for (; j < i; ++j) tp[j]->terrain = 'm';

	//Sort land tiles on temperature, except mountains. Separate arctic / nonarctic land
	qsort(tp + firstland, landtiles-mountains, sizeof(tiletype *), &q_compare_temperature);

	//Assign arctic terrain types
	for (j = firstland; tp[j]->temperature < T_GLACIER; ++j) set_tile(tp[j], 'a', 'A');
	//Assign tundra terrain types
	for (; tp[j]->temperature < T_TUNDRA; ++j) set_tile(tp[j], 't', 'T');

	int firsttempered = j;
	int total = i - firsttempered - mountains;
#ifdef DBG
	printf("%i tempered/tropical tiles\n", total);
#endif
	//Sort tempered/tropic low/hills on wetness, classify on wetness
	qsort(tp + firsttempered, total, sizeof(tiletype *), &q_compare_wetness);

#ifdef DBG
	printf("%i dD desert tiles\n", (int)(d_part/partsum*total));
#endif
	//Assign deserts (flat+hills)
	limit = firsttempered + (d_part/partsum) * total;
	for (; j < limit; ++j) {
		set_tile(tp[j], 'd', 'D');
		//printf("%c wetness:%5i\n",tp[j]->terrain, tp[j]->wetness);
	}
#ifdef DBG
	printf("%i ph plain/hill tiles\n", (int)(p_part/partsum*total));
#endif
	//Assign plains & hills
	for (limit += (p_part/partsum)*total; j < limit; ++j) {
		set_tile(tp[j], 'p', 'h');
		//printf("%c wetness:%5i\n",tp[j]->terrain, tp[j]->wetness);
	}

#ifdef DBG
	printf("%i gh grass/hill tiles\n", (int)(g_part/partsum*total));
#endif
	//Assign grassland & hills
	for (limit += (g_part/partsum)*total; j < limit; ++j) {
		set_tile(tp[j], 'g', 'h');
		//printf("%c wetness:%5i\n",tp[j]->terrain, tp[j]->wetness);
	}
	//Forests & forested hills. Sort on temperature, separating out jungle/jungle hills
	limit += (f_part+j_part) / partsum * total;
	qsort(tp + j, limit - j, sizeof(tiletype *), &q_compare_temperature); 
	int	flimit = j + (f_part) / partsum * total;
	for (; j < flimit; ++j) set_tile(tp[j], 'f', 'F');
	for (; j < limit; ++j) set_tile(tp[j], 'j', 'J');

	//swamps & forested hills. Sort on temperature, separating out jungle hills
	//Hills are too steep to be swampy, the water runs off. So, forest/jungle instead.
	qsort(tp + j, i-j - mountains, sizeof(tiletype *), &q_compare_temperature);
	flimit = j + s_part/partsum * total * (f_part/(f_part+j_part));
	for (; j < flimit; ++j) {
		set_tile(tp[j], 's', 'F');
		//printf("%c wetness:%5i\n",tp[j]->terrain, tp[j]->wetness);
	}
	limit = i - mountains;
	for (; j < limit; ++j) set_tile(tp[j], 's', 'J');

	//Terrain done, set up the rivers
	assign_rivers(tp, seatiles, landtiles, wateronland);

	assign_volcanoes(tile, tp, mountains, seatiles);

	terrain_fixups(tile, tp, seatiles, deepseatiles, landtiles);

	output_terrain(f, tile, true);
}


//Ensures recursively that mountains stay below 10000m when plates collide,
//or an asteroid strikes.
//Spill the excess onto neighbours, then check them too.
//Asteroid: spread in all directions, direction==-1
//Plate movement: spread in direction of plate movement, and two side directions
void mountaincheck(int x, int y, int direction, tiletype tile[mapx][mapy]) {
	tiletype *this = &tile[x][y];
	if (this->height > 10000) {
		//This mountain will be cut down to the 8000–9000 range.
		//The excess is scattered.
		short excess = this->height - 9000 + (random() & 1023);
		this->height -= excess;
		excess /= (direction == -1) ? neighbours[topo] : 3;
		neighbourtype *neigh = (y & 1) ? nodd[topo] : nevn[topo];
		int istart = (direction == -1) ? 0 : direction-1;
		int istop = (direction == -1) ? neighbours[topo]-1 : direction+1;
		for (int i = istart; i <= istop; ++i) {
			int ix = (i + neighbours[topo]) % neighbours[topo]; //Stay within 0..neighbours[topo]-1, either end may be outside
			int nx = wrap(x+neigh[ix].dx, mapx);
			int ny = wrap(y+neigh[ix].dy, mapy);
			tile[nx][ny].height += excess;
			mountaincheck(nx, ny, direction, tile);
		}
	}
}

//pl: a textonic plate. direction: index into neighbour arrays 
//moves the plate's tiles in that direction, effecting terrain changes
//move tiles in a suitable order, so this plate's tiles can be overwritten
//as the previous was moved in a earlier step. (Leading before trailing tiles)
//Leading tiles merges onto whatever they crash into
//Trailing tiles leaves a deep trench
void moveplate(platetype *pl, int direction, tiletype tile[mapx][mapy]) {
	neighbourtype *ne_odd = nodd[topo]+direction;
	neighbourtype *ne_evn = nevn[topo]+direction;
	//For stepping through the plate area in suitable order:
	int stepx, stepy, startx, starty, stopx, stopy; 
 	if (ne_odd->dx > 0 || ne_evn->dx > 0) {
		stepx = -1;
		startx = wrap((int)pl->cx + pl->rx, mapx);
		stopx = wrap((int)pl->cx - pl->rx, mapx); 

	} else {
		stepx = 1;
		stopx = wrap((int)pl->cx + pl->rx, mapx);
		startx = wrap((int)pl->cx - pl->rx, mapx);
	}

 	if (ne_odd->dy > 0 || ne_evn->dy > 0) {
		stepy = -1;
		starty = wrap((int)pl->cy + pl->ry, mapy);
		stopy = wrap((int)pl->cy - pl->ry, mapy); 

	} else {
		stepy = 1;
		stopy = wrap((int)pl->cy + pl->ry, mapy);
		starty = wrap((int)pl->cy - pl->ry, mapy);
	}
	
	//double loop for the area containing this plate
	//Must handle cases with stop and start on the same tile!
	//(A plate sometimes wraps around a small world.)
	int x = startx;
	do {
		int y = starty;
		do {
			int nxy,nxx; //next y, next x
			tiletype *this, *next, *prev;
			this = &tile[x][y];
			if (this->plate == pl->ix) { //Tile on this plate
				if (y & 1) {
					next = &tile[nxx=wrap(x+ne_odd->dx,mapx)][nxy=wrap(y+ne_odd->dy,mapy)];
					prev = &tile[wrap(x-ne_evn->dx,mapx)][wrap(y-ne_evn->dy,mapy)];
				} else {
					next = &tile[nxx=wrap(x+ne_evn->dx,mapx)][nxy=wrap(y+ne_evn->dy,mapy)];
					prev = &tile[wrap(x-ne_odd->dx,mapx)][wrap(y-ne_odd->dy,mapy)];
				}
			
				//Is this a trailing tile? Leave a rift
				short splitheight = this->height;
				if (prev->plate != pl->ix) {
					this->height *= frand(0.50, 0.75);
					splitheight -= this->height;
				}

				//Is this a leading tile?
				if (next->plate != pl->ix) {
					next->height += this->height;
					mountaincheck(nxx, nxy, direction, tile);
					//Try to avoid long perfectly straight mountain ranges:
					if (next->plate == 0) {
						//Normally, take the tile so the plate seems to move forward.
						//Occationally don't, so plate edges get notches
						if (random() & 15) next->plate = this->plate;
					} else {
						//Normally, don't take a tile from the plate this one is crashing into
						//But occationally do, so the edges get jagged
						if (!(random() & 7)) next->plate = this->plate;

					}
				} else *next = *this; //moves the entire tile

				//Is this trailing?
				if (prev->plate != pl->ix) {
					this->height = splitheight;
					//Normally, abandon the tile.
					//Occationally keep it, so trenches won't be perfectly straight
					if (random() & 7) this->plate = 0;
				}
			}
			y = wrap(y+stepy, mapy);
		} while (y != stopy);
		x = wrap(x+stepx, mapx);
	} while (x != stopx);

}

//Finds which two tile neighbours that best fits the angle.
void find_windnb(float angle, char *wind1, char *wind2) {
	neighpostype *np = nposition[topo];
	int state = 0;
	int maxangle = 360 / neighbours[topo]; //45 or 60 for freeciv topologies
	for (int n = 0; n < neighbours[topo]; ++n) {
		if (fabsf(angle - np[n].angle) <= maxangle) {
			++state;
			if (state == 1) *wind1 = n; else {
				*wind2 = n;
				break;
			}
		}
	}
	if (state == 1) {
		//Wind has direction exactly towards one neighbour, so that one gets all.
		*wind2 = *wind1; 
	}
}

/* Calculates wind direction & strength from latitude and east direction 
   The east direction is always 0 on mercator maps. It can be any direction on
	 WRAPY maps, where the poles are circles.
 
 */
void find_wind(float latitude, int east, char *wind1, char *wind2, char *wstrength) {
	float angle; 	//Direction the air moves. 0 degrees are right, 90 degrees are down
	/*
		Prevailing winds depend on latitude. 
		We set two directions, (tile neighbourhood references). This compensates 
		for a lack of exact angles. It also helps spread the rain, avoiding stripes on the map.
    Even without prevailing wind, there will still be random winds spreading clouds to
		neighbour tiles.
		 */
	if (latitude < -65.0) {
		//South pole, air moves NW (which we call a SE wind.)
		//-90->270, -65->225
		angle = 108.0-latitude*1.8;
		*wstrength = 2;
	} else if (latitude < -60.0) {
		//border between polar and westerly. weak changing wind, some go UP
		*wstrength = 0; //Only random winds, no preferred direction
	} else if (latitude < -35.0) {
		//westerly trade winds, air go SEE
		//-35->90, -60->45
		angle = 153.0 + latitude*1.8;
		*wstrength = 2;
	} else if (latitude < -30.0) {
		//border between cells. weak changing winds N & S
		*wstrength = 0;
	} else if (latitude < -5.0) {
		//strong easterly trade wind. Air goes NWW
		//-30->270 -5->210
		*wstrength = 3;
		angle = 198.0-2.4*latitude;
	} else if (latitude < 5.0) {
		//weak wind. Air goes E, some go UP
		//-5->210 5->150
		*wstrength = 1;
		angle = 180 - 5 * latitude;
	} else if (latitude < 30.0) {
		//strong trade wind. Air goes SWW
		//30->90, 5->150
		*wstrength = 3;
		angle = 162.0 - 2.4*latitude;
	} else if (latitude < 35.0) {
		//cell border. weak changing winds N & S
		*wstrength = 0;
	} else if (latitude < 60.0) {
		//westerly trade wind, air go NWW
		//35->270, 60->315
		angle = 207.0 + latitude*1.8;
		*wstrength = 2;
	} else if (latitude < 65.0) {
		//border to polar. weak changing wind, some go UP
		*wstrength = 0;
	} else {
		//North polar. Air go SE
		//90->90, 65->135
		angle = 252.0 - 1.8*latitude;
		*wstrength = 2;
	}
	angle += east; //Compensate for odd map projection
	if (angle < 0) angle += 360.0;
	else if (angle > 360.0) angle -= 360.0;

	if (*wstrength) find_windnb(angle, wind1, wind2);
}



/*  Initialize weather data. Tile temperatures, atmosphere above them
*/
void init_weather(tiletype tile[mapx][mapy], airboxtype air[mapx][mapy][9], weatherdata weather[mapx][mapy], int tempered) {
	for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		tiletype *t = &tile[x][y];
		t->wetness = 10;
		t->waterflow = 0;
		t->rocks = 0;
		for (int z = 0; z < 9; ++z) {
			air[x][y][z].water = 10;
			air[x][y][z].new = 0;
		}
	}
	int sea_min =  -14 * (100-tempered) / 100 + 2;
	int sea_max = 20 * tempered / 100 + 20;
	int land_min = -55 * (100-tempered) / 100 + 15;
	int land_max = 20 * tempered / 100 + 50;
	float latitude; //maps may be taller than 180...

	if (wrapmap == 2) {
		//System with two round poles:
	  //One at mapx/2,mapy/2, one at 0,0
		for (int x = 0; x < (mapx+1)/2; ++x) {
			for (int y = 0; y < (mapy+1)/2; ++y) {
				float dx1 = (float)x / mapx;
				float dx2 = (float)(mapx/2-x) / mapx;
				float dy1 = (float)y / mapy;
				float dy2 = (float)(mapy/2-y) / mapy;

				int isocorr = (topo & 1) + 1;

				float r1 = sqrtf(dx1*dx1+dy1*dy1/isocorr);
				float r2 = sqrtf(dx2*dx2+dy2*dy2/isocorr);

				float total = r1+r2;
				//Computed "latitude"
				latitude = r1/total * 180 - 90;
				float east;
			  if (r1 < r2) {
					//closer to (0,0) south pole. For lower left quadrant, [x,y] points north 
					east = 180 * atan2f(dy1, dx1) / M_PI + 90.0;							
				} else {
					east = 180.0 - 180*atan2f(dy2, dx2)/M_PI +90.0;					
				}	
				signed char sea_temp = (90 - fabsf(latitude))/90 * (sea_max-sea_min) + sea_min;
				signed char land_temp = (90 - fabsf(latitude))/90 * (land_max-land_min) + land_min;
				weatherdata *w = &weather[x][y];
				w->land_temp = land_temp;
				w->sea_temp = sea_temp;
				find_wind(latitude, east, &w->prevailing1, &w->prevailing2, &w->prevailing_strength);

				w = &weather[mapx-x-1][y];
				w->land_temp = land_temp;
				w->sea_temp = sea_temp;
				find_wind(latitude, 180.0-east, &w->prevailing1, &w->prevailing2, &w->prevailing_strength);

				w = &weather[x][mapy-y-1];
				w->land_temp = land_temp;
				w->sea_temp = sea_temp;
				find_wind(latitude, 360.0-east, &w->prevailing1, &w->prevailing2, &w->prevailing_strength);

				w = &weather[mapx-x-1][mapy-y-1];
				w->land_temp = land_temp;
				w->sea_temp = sea_temp;
				find_wind(latitude, 179.9+east, &w->prevailing1, &w->prevailing2, &w->prevailing_strength);

			}
		}
	} else {
		//Mercator map, y coordinate gives latitude.
		//0=90 degrees north, mapy-1=90 degrees south, mapy/2=equator
		for (int y = 0; y < mapy; ++y) {
			//Latitudes from -90 to 90
			latitude = (float)y / (mapy - 1) * 180 - 90.0;
			signed char sea_temp = (90 - fabsf(latitude))/90 * (sea_max-sea_min) + sea_min;
			signed char land_temp = (90 - fabsf(latitude))/90 * (land_max-land_min) + land_min;
			char wind1, wind2, wstrength;
			find_wind(latitude, 0, &wind1, &wind2, &wstrength);
			for (int x = 0; x < mapx; ++x) {
				weatherdata *wd = &weather[x][y];
				wd->sea_temp = sea_temp;
				wd->land_temp = land_temp;
				wd->prevailing1 = wind1;
				wd->prevailing2 = wind2;
				wd->prevailing_strength = wstrength;	
			}
		}
		
	}
}


int airtemp(int height, int groundheight, int groundtemp) {
	if (groundheight > 11000) return groundtemp; //No temperature fall above 11000
  if (height > 11000) height = 11000;
	int gdist = height - groundheight;
	return groundtemp - 6.5 * gdist/1000;
}

//Temperature drops with 6.5 Celcius per km up from ground [wikipedia] 
//this goes on to 11km, then no further drop.
//How much water may an airbox hold?
//More with higher temperature (8% per celsius) 
//parameters are:
//height of air above sea
//height of ground below
//ground temperature
int cloudcapacity(int height, int groundheight, int groundtemp) {
	int atemp = airtemp(height, groundheight, groundtemp);
	//Capacity at 50 celsius is arbitrarily set to 50 000 "units" of water
	return 50000 * powf(1.08, atemp-50);
}

/*
	Push a cloud somewhere. Go up, if the airbox is underground
	Return whatever layer the cloud went to.
	 */
int pushcloud(int h, int x, int y, int amount, int abovesea, airboxtype air[mapx][mapy][9]) {
	while (airheight[h] < abovesea) ++h;
	air[x][y][h].new += amount;
	return h;
}

//How steep a tile is, based on how much lower its lowest neighbour is:
int steepness(short heightdiff) {
	if (heightdiff < 0) return -1;
	return (heightdiff == 0) ? 0 : 1+log2(heightdiff);
	/*
  Steepness  Next tile is this lower
	       -1  negative. Uphill, i.e. river exiting a deep lake
	        0          0. Flat, next tile has same height as this one
	        1          1. One lower, pretty flat
				  2        2–3.
				  3        4–7.
				  4       8–15.
				  5      16–31.
				  6      32–63.
				  7     64–127.
				  8    128–255.
				  9    256–511.
				 10   512–1023. Mountains...
				 11  1024–2047.
				 12  2048–4095.
				 13  4096–8192.
				 14 8192–10000.
		 */
}
/*
	Find next tile in a river flow. If there is nothing else, use the lowest neighbour.
	If there is a neighbour river with more flow, merge into that instead, as long is it is on a lower tile.  Such coalescing improves the river systems a lot.

	Running into the sea is preferred over merging into another river.

	Also calculate steepness, based on the height difference between this tile and the outflow tile. Steepness is used for erosion, and for deciding how much of the wetness that leaves in the river.

	The outflow tile may not be lower than this, if this tile is lower than all neighbours. That is a problem for the caller to solve.
*/
void find_next_rivertile(int x, int y, tiletype tile[mapx][mapy], short seaheight) {
	neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
	char lownb = 0;        //Finds the lowest neighbour tile
	char flowlownb = -127; //Finds the best river to merge with (most flow, and on a lower tile)
	int maxflow = 1;
	short lowheight = 32767; //Higher than highest, some neighbour will be chosen
	tiletype *t = &tile[x][y];
	tiletype *neigh;
	int n_inc = (topo < 2) ? 2 : 1; //No rivers through corners
	short flowlowheight = lowheight;
	for (int n = 0; n < neighbours[topo]; n += n_inc) {
		neigh = &tile[wrap(x+nb[n].dx, mapx)][wrap(y+nb[n].dy, mapy)];
		if (neigh->height < lowheight) {
			lowheight = neigh->height;
			lownb = n;
		}
		if ((neigh->height < t->height) && (neigh->oldflow > maxflow)) {
			maxflow = neigh->oldflow;
			flowlownb = n;
			flowlowheight = neigh->height;
		}
	}

	//No lower tile with riverflow, or the sea is available:
	if (flowlownb == -127 || lowheight <= seaheight) {
		flowlownb = lownb; //use lowest neighbour instead
		flowlowheight = lowheight;
	}

	tile[x][y].lowestneigh = flowlownb;

	/* Don't use height difference underwater */
	if (flowlowheight < seaheight) flowlowheight = seaheight; 

	short heightdiff = tile[x][y].height - flowlowheight;
	tile[x][y].steepness = (heightdiff <= 0) ? 0 : 1+log2(heightdiff);
}

/*
	Called when a river gets stuck in a hole. It is known that all neighbours
	have higher elevation – and the suggested way out is higher than
	where the water came in from.
	Check if there is a neighbour of a neighbour that is sea or lower elevation
  than x,y.  If so, set newx,newy to the near neighbour, and flatten the near
  neighbour	so the river may escape that way.
	Return true on success, false on failure.
	NO LONGER USED, we have big lakes now.
*/
bool river_dambreak(int x, int y, short below, tiletype tile[mapx][mapy], int *newx, int *newy, short seaheight) {
	short maxheight = below - 2; // "-2" avoids loops, allow only strictly lower tiles
	int mmx, mmy;
	neighbourtype *nb0 = (y & 1) ? nodd[topo] : nevn[topo];
	char newneighbour;
	char n_inc = (topo < 2) ? 2 : 1;
	for (char n = 0; n < neighbours[topo]; n += n_inc) {
		int nx = wrap(x + nb0[n].dx, mapx);
		int ny = wrap(y + nb0[n].dy, mapy);
		neighbourtype *nb1 = (ny & 1) ? nodd[topo] : nevn[topo];
		for (char m = 0; m < neighbours[topo]; m += n_inc) {
			int mx = wrap(nx + nb1[m].dx, mapx);
			int my = wrap(ny + nb1[m].dy, mapy);
			if (mx == x && my == y) continue;
			//Find the lowest neighbour of near neigbour.
			//But not (x,y) !
			//There may be more than one near neighbour, prefer
			//flattening the lowest one.
			if (tile[mx][my].height < maxheight || 
					(tile[mx][my].height == maxheight && tile[nx][ny].height < tile[*newx][*newy].height)) {
				maxheight = tile[mx][my].height;
				*newx = nx;
				*newy = ny;
				mmx = mx;
				mmy = my;
				newneighbour = n;
			}
		}
	}
	if (maxheight == below - 2) return false; //Found no way out.
	//Break the dam
//	short newheight = (tile[x][y].height + tile[mmx][mmy].height) / 2;
	short newheight = (below + tile[mmx][mmy].height) / 2;
	int rocks = tile[*newx][*newy].height - newheight;

	tile[*newx][*newy].height = newheight;
	tile[*newx][*newy].rocks += rocks;

	//Heightmap was changed by breaking a dam. Re-run find_next_rivertile on the neighbourhood.
	nb0 = (*newy & 1) ? nodd[topo] : nevn[topo];
	for (char n = 0; n < neighbours[topo]; n += n_inc) {
		int nx = wrap(*newx + nb0[n].dx, mapx);
		int ny = wrap(*newy + nb0[n].dy, mapy);
		find_next_rivertile(nx, ny, tile, seaheight);
	}


	tile[x][y].lowestneigh = newneighbour;
	return true;
}

void asteroid_strike(tiletype tile[mapx][mapy]) {
	int x = random() % mapx;
	int y = random() % mapy;
	switch (asteroid_yadj[topo]) {
		case 0:
			break;
		case 1:
			y |= 1;
			break;
		case 2:
			y &= ~1;
			break;
	}
	printf("Asteroid strike at %i,%i\n",x,y);
	int xstart = -(asteroidx[topo] / 2);
	int xend   = asteroidx[topo] + xstart - 1;
	int ystart = -(asteroidy[topo] / 2);
	int yend   = asteroidy[topo] + ystart - 1;
	for (int cx = xstart; cx <= xend ; ++cx) for (int cy = ystart; cy <= yend; ++cy) {
		int heightchange = (int)chicxulub[topo][cy-ystart][cx-xstart];
		int nx = wrap(x+cx, mapx);
		int ny = wrap(y+cy, mapy);
		tile[nx][ny].height += heightchange;
		if (tile[nx][ny].height < 0) tile[nx][ny].height = 0;
		else if (tile[nx][ny].height > 10000) mountaincheck(nx, ny, -1, tile);
		if (tile[nx][ny].terrain == '+') tile[nx][ny].terrain = 'm'; //Lakes evaporate when hit by an asteroid
	}
}

/*
Running rivers:
Sort tiles on height. Already done when sealevel was calculated, "tp" has tile pointers sorted on height.

Iterate through land tiles (prep):
  recover x,y from tile pointer
  "wetness" is accumulated rainfall. use find_next_rivertile() to set steepness. Calculate waterflow originating here based on wetness and steepness. Reset marks. Unmarked means this tile has not yet flowed to sea.

Iterate through land tiles again, highest to lowest.
  marked tile, or no water to move? Skip to the next tile
  recover x,y from tile pointer
  Have movable water, it must now move to the sea (or a dead sea lake):
  from_height = 20000
  do {  //river loop
		accumulate movable water from this tile
	  accumulate eroded rocks from this tile
		if next tile is higher, create a lake here
    if this is a lake:
		   drop most rocks. But fill no more than to min(from_height-1, or next tile height+1)
			 if filled to next tile height+1, change the lake back to terrain. It filled up with rocks.
	  if (next tile height < from_height) {
			drop some rocks depending on steepness. Fill no more than to from_height-1
		} else {
			Next tile is higher than the lake inlet :-(
			Attempt a dam break: Find lowest of neighbours of neighbours. If lower than from_height {
			  Break the dam, by lowering the neighbour to between from_height-1 and second neighbour height+1
				accumulate the height difference as a large amount of rocks
				set the this tile's next tile to be that neighbour.
			} else {
			   Give up, this is now the dead sea!
				 Future approach: flood fill the landscape, create BIG lakes until some outlet is found.
			}
			from_height = max(this heigh, next tile height)
			tile = next ; and go on with the river loop
		}
  } while !sea
*/

/*
Various problems with erosion. 
1. Terrain changes during the round, a cached next rivertile could be wrong.
2. Water may get to the sea, but terrain is changed so the rivers are left discontinous.
   Next round may fix that, but there may not be a next round.

solution: 
 * Each round leaves a consistent river system, suitable for outputting a terrain file
 * The round leaves data like "this much waterflow, this much erosion, this many rocks
   to deposit.  But no actual terrain change during river running.
	 So at the end of a round, a consistent terrain is there.
 * Accumulated changes (mountain eroded to lower height, gravel deposited...) are done
   immediately before running rivers. A possibly broken terrain is created, and fixed during river running.

Order of operations:
1. plate tectonics, may raise mountain ranges and create rifts. Also, rare asteroid strikes, plate edge vulcanism
2. Apply deferred height map changes. Erosion makes tiles lower, deposits make the tiles higher,
   may plug lakes, may turn shallow sea to land. Some of the scattered rocks merge into terrain as sediments.
	 Erosion planned in last round's step 7, lowers terrain and becomes loose rocks.
	 
	 All lakes replaced with maximally wet land (may or may not
	 become a lake during the next river run.)

3. Recompute the sea level
4. Weather simulation, with evaporation, cloud transport and rain.
5. Run rivers. Find each tile's "next" river tile. Make lakes – big or small when necessary. No height changes.
   This establishes the waterflow from/through each tile, possibly a sum of many small flows.
	 Assume some flooding in flat landscapes; a large river will wet a flat tile to some extent.
	 A river from the mountains may go through a desert, occationally making grassland.
6. Mass transport.  Rocks from previous rounds of erosion moves, if there is sufficient steepness and waterflow.
   Again, assume some flooding in flat landscapes. This leaves some rocks, even if the waterflow is capable
	 of carrying them. A steep landscape may set lots of
	 rocks moving. If this flow reaches a flatter calmer landscape, excess rocks may stop. Rocks reaching a lake mostly stop
	 Rocks reaching a sea tile: drop many, scatter the rest to neighbouring sea tiles, if any.
	 Mass transport also establish a rock flow through each tile.
7. Erosion. Waterflow, rock flow and steepness cause erosion from movement. Low temperature+steepness cause
   erosion as water freeze and crack mountain surfaces. Glaciers may cause their own kind of erosion.



Ice ages/glaciers:
Water in frozen landscapes moves as glaciers instead. Very little wetness runs off in a slow flow, most of it accumulates in
an ice sheet. (tile->wetness interpreted as ice thickness) All rocks are transported, no limit to capacity, but they move
only one tile each round.  

Mountains higher than the ice thickness see little erosion. (height difference to next tile exceed ice thickness.) In flat landscapes, the erosion is much stronger.

Ice age:  Different temperature map, freezing much more of the world. Sea level lowered, as water is tied up in ice. Lots of erosion and gravel production.
As the ice age ends, temperatures normalize. Wetness starts running off as water, the sea reaches normal levels. Hopefully, we get a landscape with fjords and u-shaped valleys...

	 */

/*
BIG lake system (instead of dambreaks that may fail)

laketype {
	int tiles; //# of tiles in the lake
  short height; //Height of water, same as height of outlet
	int out_x, out_y; //coordinates of outlet tile

}

Each tile get a "lake number". if nonzero, it refers to some lake. water transfers to the outlet tile and flows from there.

A river runs from a mountaintop. At some point, the exit tile is higher:
1.the problem tile becomes the current lake tile:
2.set lake height to tile height. Add the tile to the lake
3.iterate through the tiles neighbours.
  Skip any that are already in the lake, or in the priority queue (use an efficient check, i.e. O(1)
	a.add tiles higher than the lake to the priority queue / heap
	b.if any tile LOWER than the lake, use this tile as the lakes outflow
	  tile, and set the lowest outside tile as the next rivertile.
		Lake processing finishes at this point, the river keeps flowing.
	c.No low tile: pick the lowest tile from the heap.
	  make this tile current and goto (2)

This procedure establishes a lake of any size, by floodfilling 
the terrain till an exit is found. 

When water enter a tile, check if it is a lake. If so, transfer to
the exit tile and go on from there.

Similar for rock flow. When entering a lake, drop the rocks and 
end the rockflow right there.

As water go on, a bigger lake might form. What if it rises to engulf
other lakes? (Through the outflow tile, or other border tiles of the same height)

We cannot simply let the lake terrain fill again from the other side. This could
lead to an infinite loop, where water repeatedly flow back and forth filling two lakes.

So. When a lake want to spill over an edge and there is a lake on the other side:
- if the lake on the other side is lower, fine. Let waterflow enter that lake and
  proceed to its exit which was found in an earlier run_rivers() this round.

- the lake on the other side cannot be higher. If it was higher, that edge
  tile would be submerged!

- The other lake may have the same height, which could happen is a flat landscape.
  If the other lake is from an earlier run_rivers() this round, we can use it.
	Let water flow to its exit as usual.

- If the other lake is the same height and created earlier in this run_rivers(),
  we have a problem. We cannot use its exit, because that exit has filled the
	lake we are now processing.

	The lakes must merge, and will likely also have to rise to find a new common
	outflow. (MAY not need to rise, if another same-height tile can be used for outflow.)

Lake merging:
A. Track down every tile of the other lake. (They are connected, so a DFS ought to be
   much faster than searching the entire map.)  Move them to THIS lake. Add all such tiles
	 neighbours to the priqueue, unless they are there already or are lake. Except for the
	 lakes's outflow tile, none have a lower neighbour not inside the lake.
	 End with selecting the other lakes old outflow tile, and process it at point 3 above. This
	 because a tile have many neighbours. The first outflow led to a dead end and this
	 lake merger. There may be another way out that works. If not, the priority queue
	 will find the next place to grow the lake. Eventually, an outflow to the sea will
	 be found.

	 */

int lakes;
laketype lake[MAX_LAKES];
tiletype *priq[MAX_PRIQ];

//Add to a priority queue/heap
void addto_priq(laketype *l, tiletype *t) {
	if (l->priq - priq + l->priq_len == MAX_PRIQ) fail("Ran out of space for priority queues. Recompile with higher MAX_PRIQ\n");
printf("adding height %i to priq\n", t->height);
	l->priq[l->priq_len] = t;

	//Push the new entry up the heap, if necessary
	int x = l->priq_len;
	while (x) {
		int above_x = (x-1) / 2;
		if (l->priq[x]->height < l->priq[above_x]->height) {
			tiletype *tmp = l->priq[above_x];
			l->priq[above_x] = l->priq[x];
			l->priq[x] = tmp;
			x = above_x;
		} else x = 0;
	}

	l->priq_len++;
}

//Extract minimum from priority queue/heap
tiletype *minfrom_priq(laketype *l) {
	if (!l->priq_len) fail("Program bug, cannot extract from an empty priority queue.\n");
	tiletype *ret = l->priq[0];
	l->priq[0] = l->priq[--l->priq_len];
	//Get the heap in order, swap the top element down
	int i = 0;
	do {
		int child = i*2+1;
		if (child < l->priq_len) {
			int child2 = child+1;
			if ((child2 < l->priq_len) && (l->priq[child2]->height < l->priq[child]->height)) child = child2;
			if (l->priq[child]->height < l->priq[i]->height) {
				tiletype *tmp = l->priq[child];
				l->priq[child] = l->priq[i];
				l->priq[i] = tmp;
			} else break;
		}
		i = child;
	} while (i < l->priq_len);
printf("minfrom() returning height %i\n", ret->height);
	return ret;
}

//Lookup function for a lake's lake id. The lake may be merged into a second lake,
//which may be merged into a third, and so on. Resolve this, and use
//path compression for speeding up future lookups
int lake_id(int lake_number) {
	laketype *l = &lake[lake_number];
	if (l->merged_into == -1) return lake_number;
	l->merged_into = lake_id(l->merged_into);
	return l->merged_into;
}

//Lookup function for a tile's lake ix.
//automatically corrects lake_ix if the lake was merged earlier.
//Must be used instead of accessing tiletype->lake_ix directly, except for lake_merge()
int lookup_lake_ix(tiletype *t) {
	if (t->lake_ix != -1) t->lake_ix = lake_id(t->lake_ix);
	return t->lake_ix;
}

/*
  Lake merge process:
	* old lake merges into the new lake that is eating it
	* the old lake's priq tiles are added to the new priq, unless already discovered
	  - ensures that all lake neighbourhood tiles are on the priq
	* the old lake's outlet must be added to the new priq too
	* lake tiles is not added immediately. This is deferred to lake_ix lookup time, for efficiency
	  - a lake_ix() lookup function is needed, instead of using ->lake_ix directly
		- laketype needs a field specifying what lake it has been merged into
		- path compression is used on looking up this field.
	 */

void merge_lakes(int old_lake, int new_lake) {
	printf("merge lakes(), merging lake %i into lake %i\n", old_lake, new_lake);


	fail("not ready\n");
}
/*
// ./tergen lakes 13 2 100 200 34 29
//bug, gets a lake at 59,182 with outflow to 59,181 which flow directly into the lake again. Should not happen.
//
//makes one lake. the lake output reaches the sea.
//makes sec lake, reaches sea
//      3rd lake, sea
//      4th lake, sea
        5, sea
				6, sea
				7, sea
				8, sea outf 7,177 27 tiles

Gets a 1-tile lake where the ourtflow tile flows back into the lake. But immediately before that:
DBG: lake reached other lake!

So:
  start lake 16 on  60,182, height 2283
	possible drain on 59,181, height 2285 lake raised to 2285
	drain into lake 15, height 2285, on ?    lake 15 has different river serial, SHOULD be ok

	mk_lake ends
	water to 59,181 (outflow tile).
	water proceeds to 69,182 which is lake 15. But lake 15 also has outflow tile 59,181 - loop

	two lakes MAY share an outflow tile. But then, the out-tile must not flow into the other lake!

	Why was not the working outflow kept ???

	* fix: use nnheight instead of mistakenly using tnn->height (which discards the lakeheight above)

But still failures:
	Now, makes a 3-tile lake with outflow tile 74,154 (has different river_serial lake as neighbour)
	uses 74,154 as outflow, further flow into that lake.

	The lake completes. River flow resumes at 74,154 as it should. But straight to
	mk_lake(74,154)  WHY.  Don't make a lake on the outlet, when we know the outlet IS a feasible
	outlet.

	run_rivers unwilling to flow to same height, even though it is a lake?

	* fixed that.

But, still failures:
	minfrom() returning height 2274
	impossible, tile lower than lake?

	Here, tiles WITH lake was added as lake neighbours. That should not happen. Of course they
	were lower, tile height only goes up to the lake bottom.

  * fixed that too.

	Other bug:
	./tergen lakes 13 1 40 60 4255555 29

  Now:
	./tergen lakes 13 1 40 60 425 29
	
  Minor bugs: many lakes in the arctic. They should freeze
	            loops im mk_lake should use step=2 for some topologies. See loops in run_river and fixups
							- early attempt lead to bugs, postponed.

  Major bug: a lake touching the ocean. why? All ocean tiles are lower than land, a lake should not
             spread to the ocean edge. Did the sealevel change after last river run?
						 FOUND. output() runs sealevel a last time. Needs fixing: no erosion or deposits the last round
             - attempted fix by "no height change in last round". Helped, but did not fix.
						 - second fix, explicitly allow exit to sea tile of the same height. Helped, but did not fix

             supremely annoying - WHY lake+sea?

						 disabled dfs_lake(). Actually fixed some cases, do we have stray tile->mark again?
						                      But did not fix ALL cases :-(
						 fix: seatiles->lake_ix = -1.  No lakes in the sea!

						 fix: reset tile mark when islands are drowned in fixups.
						 Finally, no lakes in the sea.


						 Next problem: changing the neighbour ORDER in mk_lake
						 should have no effect on correctness, although
						 lakes may grow slightly different (when there is choice.)

						 Sadly, this leads to inf.loops again.
						 Making lake 10 serial 1884, outflow to lake 2 serial 2191
						 made single tile lake 10, outflow to 14,27
             run_rivers:
						 tile 14,27 height 2259 (m)
						 tile 14,25 height 2259 (+) hit lake 2 [as expected]
						            outflow to 14,27  - which of course loops forever

             WHY was not the orgiginal next rivertile for 14,27 kept then? mk_lake is making a mistake here?
						 Fixed that too.

*/
void mk_lake(int x, int y, tiletype tile[mapx][mapy], int river_serial) {
#ifdef DBG
	printf("mk_lake(%i, %i, tile, %i)\n", x,y,river_serial);
#endif
	int lake_ix = lakes;
	if (lake_ix > MAX_LAKES) fail("Too many lakes, recompile with bigger MAX_LAKES");
	laketype *l = &lake[lakes++];

	l->tiles = 0;
	l->river_serial = river_serial;
	l->height = -32768; //So the first tile WILL be higher
	l->priq_len = 0;
	l->merged_into = -1;
	laketype *prev = &lake[lake_ix-1];
	l->priq = lake_ix ? prev->priq + prev->priq_len : priq;
	tiletype *t = &tile[x][y];
	//Add tiles until an outflow tile with a lower neighbour is found.
	do {
		//The tile t is the lake's lowest neighbour.
		//Now, it will either become the next lake tile,
		//or become the lake outlet.

		if (t->height < l->height) fail("impossible, tile lower than lake?\n");

		//Adjust lake height to tile height. A lower tile is not possible
		l->height = t->height;
		//Add tile to lake, Undo if it becomes an outlet
		l->tiles++;
		t->terrain = '+';
		t->lake_ix = lake_ix;
printf("mk_lake tile loop. x=%3i  y=%3i   tile height:%5i    lake height:%i\n", x,y,t->height,l->height);
		//Iterate through tile neighbours, in search of a drain.
		//looking for a lower tile (or lower lake/sea)
		//Find the best/lowest of possibly several outlets. Or none.
		//Not the same as the next rivertile, because the lowest
		//neighbour may be inside this lake already.
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		int best_x = -1, best_y = -1;
		short best_h = l->height;
		int best_n = -1;
printf("search for drain...\n");
		int lakes_to_merge = 0; //We may find one. Or in rare cases, two.
		int n_inc = (topo < 2) ? 2 : 1; //No diagonal rivers in square topologies
//		for (int n = neighbours[topo]; n--;) {
		printf("lowest neighbour before drain search: %i\n", t->lowestneigh);
		for (int n = 0; n < neighbours[topo]; n += n_inc) {
			int nx = wrap(x+nb[n].dx, mapx);
			int ny = wrap(y+nb[n].dy, mapy);
			tiletype *tnn = &tile[nx][ny]; //tnn: tile neighbour's neighbour...
printf("n=%i ",n);
			if (lookup_lake_ix(tnn) == lake_ix) continue; //Skip already found tiles
			//Heigh of neighbour tile, or any lake on it:
			short nnheight = (tnn->terrain == '+') ? lake[lookup_lake_ix(tnn)].height : tnn->height;
printf("nnheight=%i  ",nnheight);
			if ( (nnheight > l->height) || (nnheight > best_h)) continue; //Skip useless
printf("not useless\n");
			if ((tnn->terrain == '+') && (lookup_lake_ix(tnn) != lake_ix)) {
				printf("DBG: lake reached other lake!\n");
				printf("this ix:     %5i other ix:     %5i\n",lake_ix,lookup_lake_ix(tnn));
				printf("this height: %5i other height: %5i  other tile height: %5i\n", l->height, lake[lookup_lake_ix(tnn)].height, tnn->height);
				printf("this rserial:%5i other rserial:%5i\n", l->river_serial, lake[lookup_lake_ix(tnn)].river_serial);
				printf("laketile is neighbour number:  %5i\n",n);
				//not necessarily a problem. But this case precedes many a bug...
			}
			//The neighbour is <= lake height AND <= best_h
			//always select < best_h, no further questions

			//Outflow must be to a lower tile. Or to a same-height lake with different river_serial
			//Not to a same_height tile, as it may eventually drain into the same lake again.

			//exception: if the same-height tile is ok, because a lake with different river_serial drains into it.
			//that guarantees the exit works (or we wouldn't get here)

			//Also, never ever drain into a lake that drains into THIS tile.

			//Same-height tiles may instead add to the lake later

			//if == best_h, select only if it is a lake with different river serial
			//Some land tiles and some sea tiles may have the same height. So test for sea tiles.


			if ( (nnheight < best_h) || (tnn->terrain == ':') || ( (tnn->terrain == '+') && (lake[lookup_lake_ix(tnn)].river_serial != river_serial)) ) {
				//Found a possible outlet!
				//But keep looking, the tile might have an even lower neighbour.
				best_h = nnheight;
				best_x = nx;
				best_y = ny;
				if ( tnn->terrain == '+' && lake[lookup_lake_ix(tnn)].river_serial != river_serial && lake[lookup_lake_ix(tnn)].outflow_x == x && lake[lookup_lake_ix(tnn)].outflow_y == y ) best_n = t->lowestneigh; //Other lake drains here, so NO CHANGE
				else best_n = n; //Normal case

			} else lakes_to_merge += (tnn->terrain == '+' && (lake[lookup_lake_ix(tnn)].river_serial == river_serial));
		} //drain search

		//Did we find an outlet?  If so, set up its use and return.
		//Did we find a same_height lake with same river_serial?  If so, merge the lakes. 
		//worse: in theory, there could be several lakes to merge!
		if (best_n != -1) { //Found something
			tiletype *tn = &tile[best_x][best_y];
			if ( (best_h == t->height) && (lake[lookup_lake_ix(tn)].river_serial == river_serial) ) fail("shouldn't get here!\n");//can't happen now
			else {
				//What we found, was a useable outlet. So, use it.
				l->outflow_x = x;
				l->outflow_y = y;
				t->lowestneigh = best_n; //original might be different.
				t->terrain = 'm'; //The exit tile is a land tile with river on it, not a lake part.
				l->tiles--;
printf("mk_lake() done. ix=%i outflow tile %3i,%3i. Lake tiles:%i  (flow neighbour:%i)\n",lake_ix,x,y,l->tiles,best_n);
				return;
			}
		}

		//No drain was found, so this tile will be part of the lake.
		//If any neighbours were other lakes, they must merge during the next scan:

printf("search for lake tile shore neighbours. %i lakes to merge\n", lakes_to_merge);
		//No outlet yet. Scan neighbours again, add to the priority queue.
		//Then pick the lowest tile t from the priority queue, and keep going.
//		for (int n = neighbours[topo]; n--;) {
		for (int n = 0; n < neighbours[topo]; n += n_inc) {
			int nx = wrap(x+nb[n].dx, mapx);
			int ny = wrap(y+nb[n].dy, mapy);
			tiletype *tn = &tile[nx][ny];
			if (lookup_lake_ix(tn) == lake_ix) continue; //Skip already found tiles

			//Higher tiles go on the priority queue. Neighbour lakes gets merged.
			if (tn->terrain == '+') merge_lakes(lookup_lake_ix(tn), lake_ix); else {
				//Add the tile to the priority queue:
if (tn->terrain == ':') printf("SEA neighbour???\n");
				tn->lake_ix = lake_ix; //Also mark it
printf("addto_priq()...\n");
				addto_priq(l, tn);
			}
		}

		//Find the next lowest lake edge tile
		t = minfrom_priq(l);
		recover_xy(tile, t, &x, &y);
	} while (true);
}

//Let rain water flow from every tile to the sea.
//tp is pointers into the tile array, sorted on height. Tallest is last.
//unmarked tiles have unmoved water. marked tiles has a precomputed path for water flow
void run_rivers(short seaheight, tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy]) {
	//Iterate through land tiles, prepare waterflow, find river directions, clear marks
  for (int i = mapx*mapy - 1; (tp[i]->terrain != ':') && (i >= 0); --i) {
		tiletype *t = tp[i];
		//Cancel existing lakes. They get recreated in the next pass, if still viable.
		//This way, no need to deal with lake trouble when the terrain changes.
		//Lakes gets plugged by eroded rocks. Plate tectonics may rip a lake apart.
		if (t->terrain == '+') {
			t->terrain = 'm';
			t->wetness = 1000;
		}
		t->lake_ix = -1;
		int x, y;
		recover_xy(tile, t, &x, &y);
		//Find lowest neighbour & steepness.
		find_next_rivertile(x, y, tile, seaheight); //steepness 0–12
		/*Less runoff from flat land, more from steeper, most from mountains.
			Steepness from -1 to 14. 3/(7-steepness/4) yields 4/7, 3/7, 3/6, 3/5, 3/4
		 */
		t->waterflow = 3*t->wetness / (7 - t->steepness/4);
		t->wetness -= t->waterflow;
		t->mark = 0;
		t->rockflow = 0;
	}

	lakes = 0; //Until we find some

	//Iterate through land tiles again. This time, run rivers to the sea.
	//When there is no lower tile, create a lake and grow it until some exit is found.
	//A world with little sea, may grow BIG lakes until that sea is found!
  for (int i = mapx*mapy - 1; tp[i]->terrain != ':' && i >= 0; --i) {
		tiletype *t = tp[i];
		if (t->mark || !t->waterflow) continue;
		int x, y;
		recover_xy(tile, t, &x, &y);
printf("run_rivers() start from %i,%i  height:%i   (sea:%i)\n",x,y,t->height,seaheight);
		short from_height = 20000; //Height the water came in from. Sky, or previous tile.
		int flow = 0;
		do {
			//Flooding in flat landscapes. Give some water back to the tile:
			if (t->terrain != '+') {
				int floodwater = flow / (t->steepness + 10);
				flow -= floodwater;
				t->wetness += floodwater;
			}
printf("do:tile: %3i,%3i    height:%5i   (%c)", x,y,t->height,t->terrain);
			//Accumulate waterflow through the tile
			t->waterflow += flow;
			if (!t->mark) {
				//Pick up water only the first time water flows through this tile
				flow = t->waterflow;
			}
			t->mark = 1; //Been here...


			//Determine the next tile. For land tiles, t->lowestneigh
			//For a lake tile, the lake outflow tile

			//Dead end? (impossible)
			if (t->lowestneigh == -1) {
				fail("DEBUG: impossible lowestneigh\n");
				break;
			}

			if (t->terrain == 'm') {
printf("mountain  ");
				//Look up the next tile
				neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
				int nx = wrap(x + nb[t->lowestneigh].dx, mapx);
				int ny = wrap(y + nb[t->lowestneigh].dy, mapy);
				tiletype *next = &tile[nx][ny];

				//If the tile outlet is higher up (or equal), make a lake
				//exception: flowing to the same height is ok, if it is a lake/sea
				if ( (next->height > t->height) || (next->height == t->height && next->terrain == 'm') ) {
					mk_lake(x, y, tile, i);
					laketype *l = &lake[lookup_lake_ix(t)];
					x = l->outflow_x;
					y = l->outflow_y;
					t = &tile[x][y];
					from_height = l->height;
printf("made lake, transfer to outflow %3i,%3i height:%i", x,y,t->height);
				} else {
					//River proceeds downhill
					from_height = t->height;
					//Make the next tile current:
					t = next;
					x = nx; y = ny;
				}
printf("\n");
			} else if (t->terrain == '+') {
				//The river ran into a lake. Transfer flow to the lake exit:
				laketype *l = &lake[lookup_lake_ix(t)];
printf("hit a lake, pos:%3i,%3i ix=%5i, at lake height %5i  ",x,y,lookup_lake_ix(t),l->height);
				x = l->outflow_x;
				y = l->outflow_y;
				t = &tile[x][y];
				from_height = l->height;
printf("to outflow %3i,%3i height:%5i (%c)\n", x,y,t->height,t->terrain);
			}
		} while (t->terrain != ':');
printf("reached sea\n");
	}
}

//How much rocks a waterflow may move.
int rock_capacity(int waterflow, int steepness) {
	return waterflow * steepness / 8;
}

//Move rocks with the waterflow. They may fill lakes or sea, or scatter along the way
//The waterways should be ready, no changes needed
void mass_transport(tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy]) {
  for (int i = mapx*mapy - 1; tp[i]->terrain != ':' && i >= 0; --i) {
		tiletype *t = tp[i];
		if (!t->rocks || t->terrain != 'm') continue;

		//Pick up rocks:
		int rocks = t->rocks;
		t->rocks = 0;
//if (rocks < 0) printf("A rocks=%i ?\n",rocks); //WHY negative?
		int x, y;
		recover_xy(tile, t, &x, &y);
		while (rocks && t->terrain != ':' && t->terrain != '+') {
			//Waterflow with rocks arrived here.
			//Drop rocks if the flow holds many:
			int capacity = rock_capacity(t->waterflow, t->steepness) - t->rockflow;
			if (rocks > capacity) {
				t->rocks += rocks-capacity;
				rocks = capacity;
			}	else if (t->steepness <= 5) {
				//Flooding in flat landscapes, scatter some rocks
				int scatter = rocks / (t->steepness+2);
				rocks -= scatter;
				t->rocks += scatter;
			}

			//Remaining rocks move, add to rockflow:
			t->rockflow += rocks;

			//No next tile? (run_rivers() gave up here)
			if (t->lowestneigh == -1) {
				fail("impossible, no next rivertile? (rock flow)\n");
				t->rocks += rocks;
				rocks = 0;
				break;
			}

			//Look up the next tile
			neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
			int nx = wrap(x + nb[t->lowestneigh].dx, mapx);
			int ny = wrap(y + nb[t->lowestneigh].dy, mapy);
			tiletype *next = &tile[nx][ny];

			//Make next tile current
			t = next;
			x = nx; y = ny;
		}
		//Scatter remaining rocks on this and neighbouring sea/lake tiles:
		if (rocks) {
			neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
			int scatter = rocks/10;
			for (int n = neighbours[topo]; n--;) {
				int nx = wrap(x+nb[n].dx, mapx);
				int ny = wrap(y+nb[n].dy, mapy);
				tiletype *tn = &tile[nx][ny];
				if (tn->terrain == ':' || tn->terrain == '+') {
					tn->rocks += scatter;
					rocks -= scatter;
				}
			}
			t->rocks += scatter;
		}
	}
}

void mkplanet(int const land, int const hillmountain, int const tempered, int const wateronland, tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy]) {
	//Phase 1: initialization
	
	//Phase shifts, so a different seed will make a different map:
	float xphase = frand(-M_PI, M_PI);
	float yphase = frand(-M_PI, M_PI);

	//Initialize a wavy height map

	/*Avoid very oval stuff on very non-square maps
    To avoid discontinuities at world wraparound, we need
		a whole number of sine waves along mapx and mapy. (for h1)
    If mapx is 256 and mapy is 64, we should have 4 times more waves
		in the x direction, to preserve roundness. Exactly 4 times,
		to avoid discontinuities.	Similar for 257,67...

		ISO uses different tile layouts. To avoid wide flat continents,
		consider that an ISO 3x6 world is square.
	*/
	float mapy2 = mapy, mapx2 = mapx;
	if (mapx > mapy) {
		int factor = (mapx + 0.5 * mapy)/mapy;
		mapx2 = (float)mapx / factor;
	} else if (mapx < mapy) {
		int factor = (mapy + 0.5 * mapx)/mapx;
		mapy2 = (float)mapy / factor;
	}
	if (topo & 1) mapy2 *= 2; //ISO correction
	float halfx = mapx / 2.0 - 0.5, halfy = mapy / 2.0 - 0.5;

  for (int x = mapx; x--;) for (int y = mapy; y--;) {
		float fx = 2*x*M_PI / mapx2, fy = 2*y*M_PI / mapy2;

		float fxb = (x-halfx)*2*M_PI/mapx;
		float fyb = (y-halfy)*2*M_PI/mapy;
		if (topo & 1) fyb /= 2; //ISO correction

		//Random fuzziness
		float frndx = frand(-.5, .5);
		float frndy = frand(-.5, .5);

		//Regular pattern, yields 8 round continents on quadratic grid
		//edge errors on hex grid hidden in fuzz.
		//with fussy edges
		float h1 =  sinf(fx*2+frndx+xphase)*cosf(fy*2+frndy+yphase);

		//Nice irregular pattern, breaks up repetition:
		float h2 =  cosf(fxb*1.8*(1.0+sinf(fyb+yphase)/2))*cosf(fyb*1.8*(1.0+sinf(fxb+xphase)/2));

		//High frequency noise, for more variation:
		float h3 =  sinf(fxb*13+frndy)*sinf(fyb*13+frndx);
//		tile[x][y].height = 4000 + 1000*h1 + 1400*h2 + 1000*h3*h2; // 600..7400
		tile[x][y].height = 2000 + 500*h1 + 700*h2 + 500*h3*h2; // 300..3700
	}

	//2 rounds of neighbourhood height averaging.  Improves chances of shallow sea near land,
	//avoid excessive amounts of lakes
	for (int x = mapx; x--;) for (int y = mapy; y--;) {
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		int heightsum = 2*tile[x][y].height;
		for (int n = neighbours[topo]; n--;) {
			neighbourtype *neigh = &nb[n];
			heightsum += tile[wrap(x+neigh->dx,mapx)][wrap(y+neigh->dy,mapy)].height;
		}
		tile[x][y].rocks = heightsum / (neighbours[topo]+2); //rocks is not in use at this stage
	}
	for (int x = mapx; x--;) for (int y = mapy; y--;) {
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		int heightsum = 2*tile[x][y].rocks;
		for (int n = neighbours[topo]; n--;) {
			neighbourtype *neigh = &nb[n];
			heightsum += tile[wrap(x+neigh->dx,mapx)][wrap(y+neigh->dy,mapy)].rocks;
		}
		tile[x][y].height = heightsum / (neighbours[topo]+2); //rocks is not in use at this stage
	}

	//Number of rounds for tectonics & weather
	//Use the largest coordinate, so clouds will have time to 
	//get around the world.
	rounds = mapx > mapy ? mapx : mapy;

	//Phase 2: plate tectonics, weather & erosion
	//Make the tectonic plates
	int plates = 3 * (mapx+mapy) / 32; //3, for the smallest (16×16) map

	//Area divided by plates, is area per plate. The root gives a diameter
	int plate_dist = sqrtf(mapx*mapy/plates); //9, for the smallest map. 20, for 80x80
	if (plates > 255) plates = 255;

	printf("Plate tectonics, trying %i plates\n", plates);
	platetype plate[plates];
	int done = 0;
	while (!done) {
		int i = 0;
		for (i = 0; i < plates; ++i) {
			//i from 0 to 254, plate numbers 1 to 255.
			if (!mkplate(plates, i, plate, plate_dist)) break;
		} 
		if (i >= 3) {
			plates = i;
			done = 1;
		}	
	}

	//Assign each tile to the nearest plate:
	//correct only for square topology, probably "close enough" anyway
	//correction for ISO
	int isocorr = 1 + (topo & 1);
	isocorr = isocorr * isocorr;
	for (int x = mapx; x--;) for (int y = mapy; y--;) {
		int best_plate = 0;
		int best_sqdist = sqdist(x, y, plate[0].cx, plate[0].cy);
		for (int p = plates; --p;) {
			int dist = sqdist(x, y, plate[p].cx, plate[p].cy);
			if (dist < best_sqdist) {
				best_plate = p;
				best_sqdist = dist;
			}
		}
		tile[x][y].plate = plate[best_plate].ix;
		//update plate radius, if necessary.
		//Handle wraparound
		int rx, ry, wrap;
		if (x > plate[best_plate].cx) rx = x - (int)plate[best_plate].cx;
		else rx = (int)plate[best_plate].cx - x;
		wrap = mapx - rx;
		if (wrap < rx) rx = wrap;
		if (y > plate[best_plate].cy) ry = y - (int)plate[best_plate].cy;
		else ry = (int)plate[best_plate].cy - y;
		wrap = mapy - ry;
		if (wrap < ry) ry = wrap;
		if (rx > plate[best_plate].rx) {
			plate[best_plate].rx = rx;
		}
		if (ry > plate[best_plate].ry) {
			plate[best_plate].ry = ry;
		}
	}

	/* The commented-out fails for mapx=1000 and mapy=2000
	airboxtype air[mapx][mapy][9];
	weatherdata weather[mapx][mapy];
	  do the equivalent heap allocation: 
	 Therefore, more complicated allocation of large arrays:
	 */

	weatherdata (*weather)[mapy];
	weather = malloc(sizeof(*weather) * mapx);

	airboxtype (*air)[mapy][9];
	air = malloc(sizeof(*air) * mapx);

	init_weather(tile, air, weather, tempered);


	//print_platemap(tile); //dbg

	//Initialize other fields in the tile array
	for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		tiletype *t = &tile[x][y];
		t->rocks = 0;
		t->erosion = 0;
	}

	//Terrain BEFORE plate tectonics (debug):
	//output(stdout, land, hillmountain, tempered, wateronland, tile, tp, weather);
	
	neighpostype *np = nposition[topo];
	/* Move plates */
	printf("Plate tectonics with %i plates\n", plates);
	int asteroids = mapx / 16;
	for (int i = 1; i <= rounds; ++i) {

		//No heightmap changes the LAST round.
		//This prevents sealevel changes that could leave rivers stopped
		//before the coast, or drown the edge between a lake and the sea.
		if (i < rounds) { //Allow height changes from tectonics/erosion/deposits

			//Move the plates
			for (int p = 0; p < plates; ++p) {
				platetype *pl = &plate[p];
				pl->cx += pl->vx;
				pl->cy += pl->vy;
				//Is the plate now closer to some neighbour tile, than the old center?
				float dx = pl->ocx - pl->cx;
				float dy = pl->ocy - pl->cy;
				float sqdisto = dx*dx+dy*dy;
				//Find the closest, if any
				float sqdist_best = sqdisto;
				int nearest_n = -1;

				for (int n = neighbours[topo]; n--;) {
					neighpostype *neigh = np+n;
					dx = pl->ocx + neigh->dx - pl->cx;
					dy = pl->ocy + neigh->dy - pl->cy;
					float sqdistn = dx*dx+dy*dy;
					if ((sqdistn < sqdisto) && (sqdistn < sqdist_best)) {
						//Find the best plate move, if any.
						sqdist_best = sqdistn;
						nearest_n = n;
					}
				}
				if (nearest_n > -1) {
					//Move the plate in direction of the closest neighbour tile
					moveplate(pl, nearest_n, tile);
					pl->ocx += np[nearest_n].dx;
					pl->ocy += np[nearest_n].dy;
				}

			}
			/* Run weather & erosion */
			if (asteroids && !(random() % (mapx/16)) ) {
				--asteroids;
				asteroid_strike(tile);
			}

			//Let moved rocks become sediments.
			//Apply erosion planned the previous round.
			for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
				tiletype *t = &tile[x][y];
				int sediment_percent;
				switch (t->terrain) {
					case ':': //sea
						sediment_percent = 90;
						break;
					case '+': //lake
						sediment_percent = 75;
						break;
					default:
						sediment_percent = 20;
						break;
				}

				//Some loose rocks becomes sediments:
				int rocks = t->rocks * sediment_percent / 100;
				t->height += rocks;
				t->rocks -= rocks;
				//Apply any deferred erosion, terrain becomes loose rocks:
				t->height -= t->erosion;
				t->rocks += t->erosion;
				t->erosion = 0;
			}
		}
#ifdef DBG		
		printf("weather, round %i\n",i);
		printf("evaporation\n");
#endif
		//Terrain changed last round, recompute land/sea and sea level
		int seaheight = sealevel(tp, land, tile, weather);  //After this, tp is sorted on height.
		//Evaporate water from all tiles, deposit into air above
		for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
			tiletype *t = &tile[x][y];
			//Also reset steepness & waterflow for later steps;
			t->steepness = -1;
			//Fourth root of int will fit in a byte, and if
			//a>=b, then the same holds for the fourth roots.
			t->oldflow = sqrtf(sqrtf(t->waterflow));
			t->waterflow = 0;

			int abovesea = t->height - seaheight;
			if (abovesea < 0) abovesea = 0;
			int airix = 0;
			while (airheight[airix] < abovesea) ++airix;
			airboxtype * const ab = &air[x][y][airix];
			int cloudcap = cloudcapacity(abovesea, abovesea, t->temperature) - ab->water;
			if (cloudcap < 0) cloudcap = 0;
			//Found the cloud capacity over this tile. Sea and lake will evaporate to fill this capacity.
			//Land tiles loose no more than 1/3 of their water to evaporation.
			if (t->terrain == 'm') {
			 	if (t->wetness/3 < cloudcap) cloudcap = t->wetness/3;
			}
			//Evaporate
			ab->water += cloudcap;
			if (t->terrain == 'm') t->wetness -= cloudcap;
		}
#ifdef DBG
		printf("move clouds\n");
#endif
		//Move clouds using prevailing winds, random winds & sea breeze
		//A cloud hitting a mountain moves up as well
		for (int h=0; h < 9; ++h) for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {

			//skip airboxes that are underground:
			tiletype * const t = &tile[x][y];
			int abovesea = t->height - seaheight;
			if (abovesea < 0) abovesea = 0;
			if (airheight[h] < abovesea) continue;

			airboxtype * const ab = &air[x][y][h];

			//move a fraction up to the layers above
			if (h < 8) {
				int rising = ab->water/10;
				ab->water -= rising;
				air[x][y][h+1].water += rising;
			}

			neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];


			//sea breeze for lowest air layer, sea/lake tiles
			int amount = ab->water / 16;
			if ( (t->terrain != 'm') && ( h == 0 || airheight[h-1] < abovesea) ) {
				for (int n = 0; n < neighbours[topo]; ++n) {
					int nx = wrap(x + nb[n].dx, mapx);
					int ny = wrap(y + nb[n].dy, mapy);
					if (tile[nx][ny].terrain == 'm') {
						ab->water -= amount;
						pushcloud(h, nx, ny, amount, tile[nx][ny].height-seaheight, air);
					}
				}
			}

			//scatter some clouds in random directions
			//More if there are less prevailing winds.
			for (int reps = 3-weather[x][y].prevailing_strength; reps--;) {
				int way = random() % neighbours[topo];
				ab->water -= amount;
				int nx = wrap(x+nb[way].dx, mapx);
				int ny = wrap(y+nb[way].dy, mapy);
				pushcloud(h, nx, ny, amount, tile[nx][ny].height-seaheight, air);
			}

			//move most of the cloud on prevailing winds
			if (weather[x][y].prevailing_strength) {
				int nx1 = x, nx2 = x, ny1 = y, ny2 = y;
				int h1 = h, h2 = h, reps = weather[x][y].prevailing_strength;
				char way1 = weather[x][y].prevailing1;
				char way2 = weather[x][y].prevailing2;
				amount = ab->water / 3 / reps;
				while (reps--) {
					nb = (ny1 & 1) ? nodd[topo] : nevn[topo];
					ny1 = wrap(ny1+nb[way1].dy, mapy);
					nx1 = wrap(nx1+nb[way1].dx, mapx);
					nb = (ny2 & 1) ? nodd[topo] : nevn[topo];
					ny2 = wrap(ny2+nb[way2].dy, mapy);
					nx2 = wrap(nx2+nb[way2].dx, mapx);
					ab->water -= 2*amount;
					h1 = pushcloud(h1, nx1, ny1, amount, tile[nx1][ny1].height-seaheight, air);
					h2 = pushcloud(h2, nx2, ny2, amount, tile[nx2][ny2].height-seaheight, air);
				}
			}
		}
#ifdef DBG
		printf("add up clouds, let it rain\n");
#endif
		//Add moved water to cloudwater, then let the clouds rain, wetting the ground
		for (int h = 0; h<9; ++h) for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
			//Skip airboxes that are underground:
			tiletype * const t = &tile[x][y];
			int abovesea = t->height - seaheight;
			if (abovesea < 0) abovesea = 0;
			if (airheight[h] < abovesea) continue;

			airboxtype * const ab = &air[x][y][h];
			ab->water += ab->new;
			ab->new = 0;
			//Make a small amount of rain unconditionally
			int rain = ab->water / 25;
			ab->water -= rain;
			if (t->terrain != ':') t->wetness += rain;
			//If the cloud has more water than it can hold,
			//drop a large amount of it:
			int cloudcap = cloudcapacity(airheight[h], abovesea, t->temperature);
			if (cloudcap < ab->water) {
				int rain = (ab->water - cloudcap) / 3;
				ab->water -= rain;
				if (t->terrain != ':') t->wetness += rain;
				//Migrate som water to a lower cloud layer too, for better rain shadow effects
				if (h > 0 && airheight[h-1] > abovesea) {
					ab->water -= rain;
					air[x][y][h-1].water += rain;
				}
			}
		}
#ifdef DBG
		printf("run rivers\n");
#endif	
		run_rivers(seaheight, tile, tp);
		if (i < rounds) {
			mass_transport(tile, tp);
#ifdef DBG
			printf("erode terrain\n");
#endif
			//Use water flow and rock flow to erode the terrain
			for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
				tiletype *t = &tile[x][y];
				//More water moves more rocks. And more with more steepness
				//Water erodes along the river bottom, which is part of the
				//waterflow circumference. The circumference is proportional to
				//the square root of the flow area.
				if (t->terrain == 'm') {
					t->erosion = (sqrtf(t->waterflow) * t->steepness * 4.5) / rounds; //Erosion from waterflow  5 better than 4?
					t->erosion += 5*t->rockflow / 100; //Erosion from rocks dragged along river bottoms

					//printf("erosion: %5i  erosion*rounds:%5i, flow:%5i   steepness:%5i   rockflow:%5i\n",t->erosion,t->erosion*rounds,t->waterflow,t->steepness, t->rockflow);

				} else t->erosion = 0;
			}
		}
	}

	//print_platemap(tile); //dbg
	FILE *f = fopen("tergen.sav", "w");
	if (!tileset) {
		output0(f, land, hillmountain, tempered, wateronland, tile, tp, weather);
	} else {
		output1(f, land, hillmountain, tempered, wateronland, tile, tp, weather);
	}
	fclose(f);
}

#define MAXARGS 11


//Calculate some position offsets from hardcoded angles
void init_neighpos() {
	for (int i = 0; i < 4; ++i) {
		int n = neighbours[i];
		neighpostype *npos = nposition[i];
		while (n--) {
			neighpostype *np = npos+n;
			sincosf(M_PI*np->angle/180.0, &np->dy, &np->dx);
		}
	}
}

int main(int argc, char **argv) {
	mapx = 64; 
	mapy = 128;
	wrapmap = 2;
	nametxt = "Tergen";
	int land = 33, hillmountain = 30;
	int tempered = 50, wateronland = 50;
	topo = 3;
	tileset = 0;
	init_neighpos();
	if (argc > MAXARGS) fail("Too many arguments.");
	//tergen name topology xsize ysize randseed land% hill% tempered% water%
	switch (argc) {
		//Fall through all the way, no breaks
		case 11:
			wateronland = atoi(argv[10]); //wetter terrain, more swamps and rivers. Or more deserts, fewer rivers
			percentcheck(wateronland);
		case 10:
			tempered = atoi(argv[9]); //50 is normal. balance between polar/tropic
			percentcheck(tempered);
		case 9:
			hillmountain = atoi(argv[8]);
			percentcheck(hillmountain);
		case 8:
			land = atoi(argv[7]);
			percentcheck(land);
		case 7: 
			srandom(atoi(argv[6]));
		case 6:
			mapy = atoi(argv[5]);
			if (mapy < 16) fail("Bad map y size. >=16");
		case 5:
			mapx = atoi(argv[4]);
			if (mapx < 16) fail("Bad map x size. >=16");
		case 4:
			wrapmap = atoi(argv[3]);
			if (wrapmap < 0 || wrapmap > 2) fail("Bad map wrap. 0:no wrap, 1:x-wrap 2:xy-wrap");
		case 3:
			topo = atoi(argv[2]);
			if (topo >= 10) {
				topo -= 10;
				tileset = 1;
			}
			if (topo < 0 || topo > 3) fail("Bad topology, must be 0-3.\n");
		case 2:
			nametxt = argv[1];
		default:
			printf("Map named \"%s\"\n", nametxt);
			printf("Map size: %i × %i  Topology: %i (%s)\n", mapx, mapy, topo, topotxt[topo]);
			printf("%3i%% land\n%3i%% mountains/hills\n", land, hillmountain);
			printf("%3i%% tempered\n%3i%% water on land\n", tempered, wateronland);
	}

	if (argc == 1) {
		printf("\nFor a different world:\ntergen name topology wrap xsize ysize randomseed land%% hillmountain%% tempered%% wateronland%%\n");
		printf("specify as many parameters as needed\n\n");
		printf("name - appears in the freeciv scenario list\n\n");
		printf("topologies\n0 - squares\n1 - iso squares\n2 - hex\n3 - iso hex.\nAdd 10 for extended terrain features (requires a suitable tileset like toonhex+)\n\n");
		printf("wrap\n0 - no wrap, map has 4 edges\n1 - east/west wrap, top/bottom edges\n2 - wraparound in all directions, and round poles\n\n");
	 	printf("Change randomseed for a different map with the same parameters.\n\n");
		printf("xsize, ysize  size of the map, in tiles. ISO trades height for width\n\n");
		printf("land%%         How many percent of the map is land\n");
		printf("hillmountain%% How much of the land is hills or mountains\n");
		printf("tempered%%     100 no ice, 50 normal, 0 cold planet\n");
		printf("wateronland%%  0 dry world, 20–30 normal, ...\n");
		
	}

	//Keep the parameter list
	paramtxt[0] = 0;
	for (int  i = 0; i < argc ; ++i) {
		strcat(paramtxt, argv[i]);
		strcat(paramtxt, " ");
	}

	//The terrain:
	//tiletype tile[mapx][mapy]; //Stack allocation fails for [1000][2000]

	tiletype (*tile)[mapy];
	tile = malloc(sizeof(*tile) * mapx);
	//Sortable array of pointers to tiles:
	//tiletype *tp[mapx*mapy];
	tiletype **tp = malloc(mapx * mapy * sizeof(tiletype *));
	{
		int i = 0, x = mapx, y = mapy;
		while (x--) for (y=mapy; y--;) {
			tp[i++]=&(tile[x][y]);
		}
	}
	mkplanet(land, hillmountain, tempered, wateronland, tile, tp);
}
