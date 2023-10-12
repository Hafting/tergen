/*
    tergen - terrain generator for freeciv (2.6)
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
(0,0) to (1,1) is 45 degrees for geometrical purposes. But it is a down direction, because y increase in the 90 degree direction, which is down.


abc   "a" is the top corner in the rendering, "c" is right corner.
def
ghi
jkl
mno
pqr

h neighbours: egik (with diagonals: defgijkl)
Directions: 0,90,180,270
Diagonals: 45,135,225,315 

Tile neighbour offsets for odd & even lines:
(-1,0) (1,0) (0,-1) (0,1)
May also use diagonal neigbours:
(-1,-1) (-1, 1) (1, -1) (1, 1)


square iso topo 1
==========
a   b   c       Each tile a diamond, closest neighbours diagonally
  d   e   f
g   h   i
  j   k   l
m   n   o
  p   q   r

h neighbours: dejk (with diagonals: bdegijkn)
k neighbours: hino (with diagonals: ehijlnoq)
Directions: 45,135,225,315
Diagonals: 0,90,180,270

tile neighbours  odd lines: ( 0,-1) (1,-1) (0, 1) (1, 1)
tile neighbours even lines: (-1,-1) (0,-1) (-1,1) (0, 1)

odd & even diagonals: (0,-2) (-1,0) (1,0) (0,2)

hex nonISO topo 2
==========
The hexagons are flat (connecting) on the sides, points up and down.
a b c
 d e f
g h i
 j k l
m n o
 p q r

h neighbours: degijk
k neighbours: hijlno
Directions: 0,60,120,180,240,300

Tile neighbours  odd lines: (0,-1)  (1,-1) (-1,0) (1,0) (0,1)  (1,1)
Tile neighbours even lines: (-1,-1) (0,-1) (-1,0) (1,0) (-1,1) (0,1)

hex ISO topo 3
=======
hexagons are flat (connecting) above and below, points to the sides.
a   b   c
  d   e   f
g   h   i
  j   k   l
m   n   o
  p   q   r

h neighbours: bdejkn
k neighbours: ehinoq
Directions: 30, 90, 150, 210, 270, 330

Tile neighbours  odd lines: (0,-2) (0,-1)  (1,-1) (0,1)  (1,1) (0,2)
Tile neighbours even lines: (0,-2) (-1,-1) (0,-1) (-1,1) (0,1) (0,2)

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
	short height; //Meters above lowest sea bottom. Max 10000
	char terrain; //Freeciv terrain letter
	char plate;   //id of tectonic plate this tile belongs to
	char mark; //for depth-first search.
	char river; //for river assignment during map output
	char lowestneigh; //direction of lowest neighbour
	char steepness; //1+log2 of height difference to lowest neighbour. -1 if unset
	char temperature; //in celsius
	int wetness; //rainfall adds, evaporation subtracts
	int waterflow; //amount in rivers. From rain, plus flow from other tiles
	char oldflow; //log2 of prev. flow. Used for re-routing rivers
	short rocks; //erosion, rocks following the rivers
} tiletype;


typedef struct {
	signed char sea_temp;  //in celsius
	signed char land_temp; //At 0m above sea
	char prevailing1; //index into neighbourtype array. Direction of prevaling wind
	char prevailing2; //second prevailing wind direction
	char prevailing_strength; //1, 2, 3
} weatherdata;


typedef struct {
	char temperature;
	int water;
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

//Globals
int mapx, mapy; //Map dimensions
int topo; //map topology 0:square, 1:square iso, 2:hex, 3:hex iso
int wrapmap; //0:no map wrap, 1: x-wrap 2:xy-wrap

char *wraptxt[3] = {"", "WRAPX", "WRAPX|WRAPY"};
char *topotxt[4] = {"", "ISO", "HEX", "ISO|HEX"};

int airheight[9] = {50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};

/*  odd & even neighbour arrays for the topologies  */
neighbourtype n0o[] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
neighbourtype n0e[] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
neighbourtype n1o[] = {{ 0,-1},{1,-1},{ 0,1},{1, 1},{0,-2},{-1,0},{0,2},{0,1}};
neighbourtype n1e[] = {{-1,-1},{0,-1},{-1,1},{0, 1},{0,-2},{-1,0},{0,2},{0,1}};
neighbourtype n2o[] = {{ 0,-1},{1,-1},{-1,0},{1,0},{ 0,1},{1,1}};
neighbourtype n2e[] = {{-1,-1},{0,-1},{-1,0},{1,0},{-1,1},{0,1}};
neighbourtype n3o[] = {{0,-2},{ 0,-1},{1,-1},{ 0,1},{1,1},{0,2}};
neighbourtype n3e[] = {{0,-2},{-1,-1},{0,-1},{-1,1},{0,1},{0,2}};

neighbourtype *nodd[4] = {n0o, n1o, n2o, n3o};
neighbourtype *nevn[4] = {n0e, n1e, n2e, n3e};

/*  geometric position of neighbour tiles for the topologies  */
neighpostype np0[8] = {{180},{90},{270},{0},{225},{135},{315},{45}};
neighpostype np1[8] = {{225},{315},{135},{45},{270},{180},{90},{0}};
neighpostype np2[6] = {{240},{300},{180},{0},{120},{60}};
neighpostype np3[6] = {{270},{210},{330},{150},{30},{90}};

neighpostype *nposition[4] = {np0, np1, np2, np3};

/*  This many tile neighbours in each topology */
int neighbours[4] = {8,4,6,6};

void fail(char *s) {
	printf("%s\n", s);
	exit(1);
}

void percentcheck(int const x) {
	if (x < 0 || x > 100) fail("Percentages must be in the 0-100 range.");
}

//Finds square of distance between two points.
//Shortest distance, using wrapx, wrapy, both or none.
//Uses ints for speed, ok even if the coordinates may be floats.
int sqdist(int const x1, int const y1, int const x2, int const y2) {
	int dx, dx2, dy, dy2;

  dx = x1-x2; 
	if (dx < 0) dx = -dx;
	dx2 = mapx - dx;
 	if (dx2 < dx) dx = dx2;

	dy = y1-y2;
	if (dy < 0) dy = -dy;
	dy2 = mapy - dy;
	if (dy2 < dy) dy = dy2;
	return dx*dx + dy*dy;
}

/* Random number in a range (inclusive) */
float frand(float min, float max) {
	float range = max - min;
	return min + random() * range / RAND_MAX;
}

/*
Weather effects moves one tile per round. 
To get clouds from the sea to the center of a continent,
we need as many rounds as the radius of the largest continent.
Otherwise, the interior will necessarily be deserts.

Each round also erodes the terrain. Too many, and it may all be flat.
100 rounds with 100 erosion can remove a mountain of height 10000
*/
#define ROUNDS 100


/* Make a tectonic plate, not too close to other plates. */
int mkplate(int const plates, int const ix, platetype plate[plates]) {
	//Up to 5 tries, or give up. Fewer plates is not a problem
	int tries = 5+1;
	while (--tries) {
		float x = frand(0, mapx); 
		float y = frand(0, mapy); 
		//Distance test
		for (int i = 0; i < ix; ++i) if (sqdist(x,y,plate[i].cx,plate[i].cy) < 64) goto tryagain;
		plate[ix].cx = plate[ix].ocx = x;
		plate[ix].cy = plate[ix].ocy = y;
		plate[ix].ix = ix + 1;
		plate[ix].vx = frand(-5.0/ROUNDS,5.0/ROUNDS);
		plate[ix].vy = frand(-5.0/ROUNDS,5.0/ROUNDS);
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


bool is_sea(char c) {
	return c == ' ' || c == ':';
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
		 - single-tile islands become shallow sea
		 - deep sea with more than one land neighbours become shallow 
		 - lake connected to sea become shallow sea
		 - river in the ice removed, unless there is a non-arctic neighbour (ice edge may have river)
	 */
void terrain_fixups(tiletype tile[mapx][mapy]) {
	//Get rid of single-tile islands, except arctic.
	int const neighcount = neighbours[topo];
	int n;

	for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		if (!is_sea(tile[x][y].terrain) && tile[x][y].terrain != 'a') {
			for (n = 0; n < neighcount; ++n) if (!is_sea(tile[wrap(x+nb[n].dx, mapx)][wrap(y+nb[n].dy, mapy)].terrain)) break;
			if (n == neighcount) {
				//Double the island, or drown it. Either way avoids single tile islands
				int num = random() % (neighcount*2);
				if (num >= neighcount) {
					tile[x][y].terrain = ' '; //Drown the island
					tile[x][y].river = 0;
				} else {
					int nx = wrap(x+nb[num].dx, mapx);
					int ny = wrap(y+nb[num].dy, mapy);
					tile[nx][ny].terrain = tile[x][y].terrain; //Raise some neighbour
				}
			}
		}
	}

  for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
		if (is_sea(tile[x][y].terrain)) {
			//Find deep sea next to land, and small lakes
			int seacnt = seacount(x, y, tile);
			int landcnt = neighcount - seacnt;
			if (seacnt == 0) tile[x][y].terrain = '+'; //Single-tile sea->lake
			else if (landcnt >= 2) {
				tile[x][y].terrain = ' '; //Make it shallow
				//Convert to lake, if a small piece of sea is trapped:
				start_dfs_lake(x, y, tile);
			}
		} else if (tile[x][y].terrain == '+') {
			//Find lakes connecting to the sea, change to shallow sea
			//Occur because of land sinking or eroding
			if (seacount(x, y, tile)) lake_to_sea(x, y, tile);
		} else if (tile[x][y].river) {
			//Remove rivers from the ice, unless they are on the ice edge
			//Also remove rivers completely surrounded by other rivers
			int icecnt = 0;
			int rivercnt = 0;
			for (int n = 0; n < neighbours[topo]; ++n) {
				tiletype *nbtile = &tile[wrap(x+nb[n].dx,mapx)][wrap(y+nb[n].dy,mapy)];
				if (nbtile->terrain == 'a') ++icecnt;
				rivercnt += nbtile->river;
			}
			if (icecnt == neighbours[topo] || rivercnt == neighbours[topo]) tile[x][y].river = 0;
		}
  }
}


//Sorts the tiles on height, determining the sea level because
//x% of the tiles are sea, so the last sea tile gives the sea height.
//Also determine tile temperatures based on being sea or land
//Done every round, as erotion & tectonics change tile heights & sea level
int sealevel(tiletype *tp[mapx*mapy], int land, tiletype tile[mapx][mapy], weatherdata weather[mapx][mapy]) {
	//Find the sea level by sorting on height. "land" is the percentage of land tiles
	int tilecnt = mapx*mapy;
	qsort(tp, tilecnt, sizeof(tiletype *), &q_compare_height);
	int landtiles = land * tilecnt / 100;
	int seatiles = tilecnt - landtiles;
	int level = tp[seatiles-1]->height;

	//Update the temperature array, based on sea or land status
	//assign sea/land status
	for (int i = 0; i < seatiles-1; ++i) tp[i]->terrain = ':';
	for (int i = seatiles; i < tilecnt; ++i) {
		if (tp[i]->terrain == ':') tp[i]->terrain = 'm';
	}
	//temperatures. Land temperatures fall with elevation
  for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		if (tile[x][y].terrain == ':') tile[x][y].temperature = weather[x][y].sea_temp;
		else tile[x][y].temperature = weather[x][y].land_temp - (tile[x][y].height-level)/111;
	}
	//two rounds of averaging. Sea may thaw frozen land, land may freeze some sea.
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

#define T_SEAICE -1
#define T_GLACIER -6
#define T_TUNDRA 2

//Assign freeciv terrain types, and output a freeciv map
/*
Terrain assignment:
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
 
   Missing tiletypes: 
	 snowy mountain       Looks better in the arctic, otherwise like normal mountains
	 desert mountain      Looks better (desert colors), otherwise like other mountains
   snowy hill           Better for the arctic. Mineable like other hills
	 desert hill          Rougher terrain in deserts, mineable.
	 forest/jungle hill   Rougher terrain. Convertible to plain hill by removing trees
   tundra hill          Rougher terrain in the tundra areas, mineable


Handling parameters "temperate" and "wateronland":

"temperate" influence the temperature map, and therefore how much ice we get.
It also affect the amount of desert and the forest/jungle allocation. 50 is normal

"wateronland" gives twice the percentage of river tiles. Actual number will be lower, because rivers merge to prevent ugly "river on every tile in the grid". Wateronland also affect the desert/swamp balance, and p/g allocation. 50 is normal
*/
void output(FILE *f, int land, int hillmountain, int tempered, int wateronland, tiletype tile[mapx][mapy], tiletype *tp[mapx*mapy], weatherdata weather[mapx][mapy]) {
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
	float d_part = 80.0 * tempered/100.0 * (100.0-wateronland)/100.0; // 0–80, 20 normal
	float pg_part = 40.0;
	
	float fj_part = 20.0;
	float s_part = 40.0 * wateronland/100.0; //0–40, 20 normal

	float f_part = fj_part * (100.0-tempered) / 100.0;
	float j_part = fj_part - f_part;
	float p_part = pg_part * (100.0-wateronland) / 100.0;
	float g_part = pg_part * wateronland / 100.0;
	float partsum = d_part + p_part + g_part + f_part + j_part + s_part;
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
	//int fifth = (firsthill-firsttempered) / 5;
	limit = firsttempered + (d_part/partsum)*total;
	for (; j < limit; ++j) set(&tp[j]->terrain, 'd');
	for (limit += (p_part/partsum)*total; j < limit; ++j) set(&tp[j]->terrain, 'p');
	for (limit += (g_part/partsum)*total; j < limit; ++j) set(&tp[j]->terrain, 'g');
	//Split the forests on temperature. The warmer part is jungle
	int forest = fj_part/partsum * total;
	qsort(tp + limit, forest, sizeof(tiletype *), &q_compare_temperature);
	int firstswamp = limit + forest;
	for (limit += f_part/partsum*total; j < limit; ++j) set(&tp[j]->terrain, 'f');
	for (limit = firstswamp; j < limit; ++j) set(&tp[j]->terrain, 'j');
	for (limit = firsthill; j < limit; ++j) set(&tp[j]->terrain, 's');	
	//Assign the rivers
	qsort(tp + seatiles, landtiles, sizeof(tiletype *), &q_compare_waterflow);
	j = 0;
	int rivertiles = landtiles * wateronland / 200; //More than 50% river tiles is useless anyway
	int nonrivers = landtiles - rivertiles;
	for (limit = seatiles + nonrivers; j < limit; ++j) {
		tp[j]->river = 0;
	}
	for (limit += rivertiles; j < limit; ++j) {
		tp[j]->river = (tp[j]->terrain != '+' && tp[j]->waterflow > 0) ? 1 : 0; 
	}
  terrain_fixups(tile);
	

	//pre-terrain stuff
	fprintf(f, "[scenario]\n");
	fprintf(f, "game_version=3000100\n");
	fprintf(f, "is_scenario=TRUE\n");
	fprintf(f, "name=\"Tergen\"\n");
	fprintf(f, "description=\"Testing the tergen terrain generator\"\n");
	fprintf(f, "save_random=FALSE\n");
	fprintf(f, "players=FALSE\n");
	fprintf(f, "startpos_nations=FALSE\n");
	fprintf(f, "lake_flooding=TRUE\n");
	fprintf(f, "ruleset_locked=TRUE\n");
	fprintf(f, "ruleset_caps=\"+std-terrains\"\n");

  fprintf(f, "[savefile]\n");
	fprintf(f, "options=\" +version3\"\n");
	fprintf(f, "rulesetdir=\"civ2civ3\"\n");
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
	fprintf(f, "terrident={\"name\",\"identifier\"\n\"Inaccessible\",\"i\"\n\"Lake\",\"+\"\n\"Ocean\",\" \"\n\"Deep Ocean\",\":\"\n\"Glacier\",\"a\"\n\"Desert\",\"d\"\n\"Forest\",\"f\"\n\"Grassland\",\"g\"\n\"Hills\",\"h\"\n\"Jungle\",\"j\"\n\"Mountains\",\"m\"\n\"Plains\",\"p\"\n\"Swamp\",\"s\"\n\"Tundra\",\"t\"\n}\n");
	fprintf(f, "\n");

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

//Ensures recursively that mountains stay below 10000m when plates collide.
//Spill the excess onto neighbours, then check them too.
void mountaincheck(int x, int y, tiletype tile[mapx][mapy]) {
	tiletype *this = &tile[x][y];
	if (this->height > 10000) {
		short excess = this->height - 9000 + (random() & 1023);
		this->height -= excess;
		excess /= neighbours[topo];
		neighbourtype *neigh = (y & 1) ? nodd[topo] : nevn[topo];
		for (int i = neighbours[topo]; i-- > 0;) {
			int nx = wrap(x+neigh[i].dx, mapx);
			int ny = wrap(y+neigh[i].dy, mapy);
			tile[nx][ny].height += excess;
			mountaincheck(nx, ny, tile);
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
	int x = startx;
	do {
		int y = starty;
		do {
			int nxy,nxx;
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
					mountaincheck(nxx, nxy, tile);
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
				} else *next = *this;

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
void init_weather(tiletype tile[mapx][mapy], airboxtype air[9][mapx][mapy], weatherdata weather[mapx][mapy], int tempered) {
	memset(air, 0, 9*mapx*mapy*sizeof(airboxtype));
	for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
		tiletype *t = &tile[x][y];
		t->wetness = t->waterflow = 0;
		t->rocks = 0;
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

				float r1 = sqrtf(dx1*dx1+dy1*dy1);
				float r2 = sqrtf(dx2*dx2+dy2*dy2);

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
	//Capacity at 50 celsius is arbitrarily set to 3000 "units" of water
	return 3000 * powf(1.08, atemp-50);
}

/*
	Push a cloud somewhere. Go up, if the airbox is underground
	Return whatever layer the cloud went to.
	 */
int pushcloud(int h, int x, int y, int amount, int abovesea, airboxtype air[9][mapx][mapy]) {
	while (airheight[h] < abovesea) ++h;
	air[h][x][y].water += amount;
	return h;

}

/*
	Find next tile in a river flow. If there is nothing else, use the lowest neighbour.
	If there is a neighbour river with more flow, merge into that instead, as long is it is on a lower tile.  Such coalescing improves the river systems a lot
	Also calculate steepness, based on the height difference between this tile and the outflow tile. Steepness is used for erosion, and for deciding how much of the wetness that leaves in the river.
*/
void find_next_rivertile(int x, int y, tiletype tile[mapx][mapy], short seaheight) {
	neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];
	char lownb = 0;
	char flowlownb = -127;
	int maxflow = 1;
	short lowheight = 32767; //Higher than highest, some neighbour will be chosen
	tiletype *t = &tile[x][y];
	tiletype *neigh;
	short flowlowheight = lowheight;
	for (int n = 0; n < neighbours[topo]; ++n) {
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
	if (flowlownb == -127) { //No lower tile with riverflow
		flowlownb = lownb; //use lowest neighbour instead
		flowlowheight = lowheight;
	}
	tile[x][y].lowestneigh = flowlownb;

	/* Don't use height difference underwater */
	if (flowlowheight < seaheight) flowlowheight = seaheight; 

	short heightdiff = tile[x][y].height - flowlowheight;
	tile[x][y].steepness = (heightdiff <= 0) ? 0 : 1+log2(heightdiff);
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
	
	float halfx = mapx / 2.0 - 0.5, halfy = mapy / 2.0 - 0.5;

  for (int x = mapx; x--;) for (int y = mapy; y--;) {
		float fx = 2*x*M_PI / mapx2, fy = 2*y*M_PI / mapy2;

		float fxb = (x-halfx)*2*M_PI/mapx;
		float fyb = (y-halfy)*2*M_PI/mapy;

		//Random fuzziness
		float frndx = frand(-.5, .5);
		float frndy = frand(-.5, .5);

		//Regular pattern, yields 8 round continents on quadratic grid
		//errors on hex grid hidden in fuzz.
		//with fussy edges
		float h1 =  sinf(fx*2+frndx+xphase)*cosf(fy*2+frndy+yphase);

		//Nice irregular pattern, breaks up repetition:
		float h2 =  cosf(fxb*1.8*(1.0+sinf(fyb+yphase)/2))*cosf(fyb*1.8*(1.0+sinf(fxb+xphase)/2));

		//High frequency noise, more variation:
		float h3 =  sinf(fxb*13+frndy)*sinf(fyb*13+frndx);
		tile[x][y].height = 4000 + 1000*h1 + 1400*h2 + 1000*h3*h2; // 600..7400
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




	//Phase 2: plate tectonics, weather & erosion
	//Make the tectonic plates
	int plates = 3 * (mapx+mapy) / 32; //3, for the smallest (16×16) map
	if (plates > 255) plates = 255;

	printf("Plate tectonics, trying %i plates\n", plates);
	platetype plate[plates];
	int done = 0;
	while (!done) {
		int i = 0;
  	for (i = 0; i < plates; ++i) {
			if (!mkplate(plates, i, plate)) break;
		} 
		if (i >= 3) {
			plates = i;
			done = 1;
		}	
	}

	//Assign each tile to the nearest plate:
	//correct only for square topology, probably "close enough" anyway
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

	airboxtype air[9][mapx][mapy];
	weatherdata weather[mapx][mapy];
	init_weather(tile, air, weather, tempered);

	//print_platemap(tile); //dbg

	//Terrain BEFORE plate tectonics (debug):
	//output(stdout, land, hillmountain, tempered, wateronland, tile, tp, weather);

	
	neighpostype *np = nposition[topo];
	/* Move plates */
	printf("Plate tectonics with %i plates\n", plates);
	for (int i = 1; i <= ROUNDS; ++i) {
		//Move the plates
		for (int p = 0; p < plates; ++p) {
			platetype *pl = &plate[p];
			pl->cx += pl->vx;
			pl->cy += pl->vy;	
			//Is the plate now closer to some neighbour, than the old center?
			float dx = pl->ocx - pl->cx;
			float dy = pl->ocy - pl->cy;
			float sqdisto = dx*dx+dy*dy;
			for (int n = neighbours[topo]; n--;) {
				neighpostype *neigh = np+n;
				dx = pl->ocx + neigh->dx - pl->cx;
				dy = pl->ocy + neigh->dy - pl->cy;
				float sqdistn = dx*dx+dy*dy;
				if (sqdistn < sqdisto) {
					//Move the plate in direction of that neighbour
					moveplate(pl, n, tile);
					pl->ocx += neigh->dx;
					pl->ocy += neigh->dy;
				} 
			}
		}
		/* Run weather & erosion */
//printf("weather, round %i\n",i);
		//Terrain changed last round, recompute land/sea and sea level
		int seaheight = sealevel(tp, land, tile, weather);
		//Evaporate water from all tiles, deposit into air above
		for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
			tiletype *t = &tile[x][y];
			//Also reset steepness & waterflow for later steps;
			t->steepness = -1;
			t->oldflow = log2(t->waterflow);
			t->waterflow = 0;

			int abovesea = t->height - seaheight;
			if (abovesea < 0) abovesea = 0;
			int airix = 0;
			while (airheight[airix] < abovesea) ++airix;
			airboxtype *ab = &air[airix][x][y];
			int cloudcap = cloudcapacity(abovesea, abovesea, t->temperature) - ab->water;
			if (cloudcap < 0) cloudcap = 0;
			//found the cloud capacity over this tile. But do not evaporate over half of a land tile:
			if (t->terrain == 'm') {
			 	if (t->wetness/2 < cloudcap) cloudcap = t->wetness/2;
			}
			//Evaporate
			ab->water += cloudcap;
			if (t->terrain == 'm') t->wetness -= cloudcap;
		}

		//Move clouds using prevailing winds, random winds & sea breeze
		//A cloud hitting a mountain moves up instead - it will then move
		//when the layer above moves.
		for (int h=0; h < 9; ++h) for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {

			//skip airboxes that are underground:
			tiletype *t = &tile[x][y];
			int abovesea = t->height - seaheight;
			if (abovesea < 0) abovesea = 0;
			if (airheight[h] < abovesea) continue;

			airboxtype *ab = &air[h][x][y];

			//move a fraction up to the layer above
			if (h < 8) {
				int rising = ab->water/10;
				ab->water -= rising;
				air[h+1][x][y].water += rising;
			}

			neighbourtype *nb = (y & 1) ? nodd[topo] : nevn[topo];


			//sea breeze for air layer 0, sea/lake tiles
			int amount = ab->water / 16;
			if (h == 0 && t->terrain != 'm') {
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
			//More if there is less prevailing winds.
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

		//Let the clouds rain, wetting the ground
		for (int h = 0; h<9; ++h) for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
			//Skip airboxes that are underground:
			tiletype *t = &tile[x][y];
			int abovesea = t->height - seaheight;
			if (abovesea < 0) abovesea = 0;
			if (airheight[h] < abovesea) continue;

			airboxtype *ab = &air[h][x][y];
			//Make a small amount of rain unconditionally
			int rain = ab->water / 25;
			ab->water -= rain;
			if (t->terrain == 'm') t->wetness += rain; 
			//If the cloud has more water than it can hold,
			//drop a large amount of it:
			int cloudcap = cloudcapacity(airheight[h], abovesea, t->temperature);
			if (cloudcap < ab->water) {
				int rain = (ab->water - cloudcap) / 3;
				ab->water -= rain;
				t->wetness += rain;
				//Migrate som water to a lower cloud layer too, for better rain shadow effects
				if (h > 0 && airheight[h-1] > abovesea) {
					rain /= 2;
					ab->water -= rain;
					air[h-1][x][y].water += rain;
				}
			}
		}

		//Let some ground wetness run off in rivers. A steep tile
		//drains faster than one in flat terrain. (height difference to lowest neighbour)
		for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
			//Find the lowest neighbour, which is where the water will go.
			//Also calculate the steepness as the height difference.
			//More wetness run off from steep terrain
			tiletype *t = &tile[x][y];
			//skip sea tiles
			if (t->terrain == ':') continue;
			//Find lowest neighbour & steepness.  
			find_next_rivertile(x, y, tile, seaheight); //steepness 0–12
			/*Less runoff from flat land, more from steeper, most from mountains. But not all.
				Steepness from 0 to 14. 3/(7-steepness) yields 3/8, 3/7, 3/6, 3/5, 3/4
				*/
			int waterflow = 3*t->wetness / (7 - t->steepness/3);
			t->wetness -= waterflow;

			//More emphasis to rivers composed from several tiles, than a single large mountain
			//looks better, and more rivers in cold regions
			if (waterflow)  waterflow = 1 + log2(waterflow);

			int prev_x = x, prev_y = y;
			int pprevx = -1, pprevy = -1;
			int rocks = 0;

			//Now send this to the ocean, accumulating flow along the way:
			if (waterflow) do {
				/* Move water to the lowest neighbour.
					 if the lowest neighbour is higher up, this becomes a lake.
					 If the lake's lowest neighbour is where the water came in, 
					 give up and create the dead sea.
				*/
				neighbourtype *nb = (prev_y & 1) ? nodd[topo] : nevn[topo];
				t->waterflow += waterflow;
				rocks += t->rocks;
				t->rocks = 0;
				int nx = wrap(prev_x + nb[t->lowestneigh].dx, mapx);
				int ny = wrap(prev_y + nb[t->lowestneigh].dy, mapy);
				if (tile[nx][ny].height >= t->height) {
					t->terrain = '+'; //This is a hole, make a lake
				}
				//If we hit a lake, drop the rocks
				if (t->terrain == '+') {
					t->height += rocks;
					rocks = 0;
					//Did we fill this lake?
					if (tile[nx][ny].height < t->height) {
						//Let some rocks go downstream
						int excess = (t->height - tile[nx][ny].height);
						rocks = excess/3;
						t->height -= rocks;
						t->terrain = 'm';
					} else {
						//Drop some rocks, depending on steepness
						int drop = rocks / (t->steepness/4 + 2);
						rocks -= drop;
						t->height += drop;
					}
				}
				if (nx == pprevx && ny == pprevy) {
					break; //Give up reaching the sea, this lake must be the dead sea
				}
				pprevx = prev_x; pprevy = prev_y; prev_x = nx; prev_y = ny; 
				t = &tile[prev_x][prev_y];
				find_next_rivertile(prev_x, prev_y, tile, seaheight);
			} while (t->terrain != ':'); //Stop river upon reaching the sea
			t->waterflow += waterflow;
			t->height += rocks; 
		} //end of rivers

		//Use water flow to erode the terrain
		for (int x = 0; x < mapx; ++x) for (int y = 0; y < mapy; ++y) {
			tiletype *t = &tile[x][y];
			//More water moves more rocks. And more with more steepness
			if (t->terrain == 'm') {
				t->rocks = sqrt(t->waterflow * t->steepness);
				t->height -= t->rocks;
			}
		}

	}

	//print_platemap(tile); //dbg
	//output(stdout, land, hillmountain, tempered, wateronland, tile, tp, weather); //screen dbg
	FILE *f = fopen("tergen.sav", "w");
	output(f, land, hillmountain, tempered, wateronland, tile, tp, weather);
	fclose(f);
}

#define MAXARGS 10


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
	int land = 33, hillmountain = 30;
	int tempered = 50, wateronland = 50;
	topo = 3;
	init_neighpos();
	if (argc > MAXARGS) fail("Too many arguments.");
	//tergen topology xsize ysize randseed land% hill% tempered% water%
	switch (argc) {
		//Fall through all the way, no breaks
		case 10:
			wateronland = atoi(argv[9]); //wetter terrain, more swamps and rivers. Or more deserts, fewer rivers
			percentcheck(wateronland);
		case 9:
			tempered = atoi(argv[8]); //50 is normal. balance between polar/tropic
			percentcheck(tempered);
		case 8:
			hillmountain = atoi(argv[7]);
			percentcheck(hillmountain);
		case 7:
			land = atoi(argv[6]);
			percentcheck(land);
		case 6: 
			srandom(atoi(argv[5]));
		case 5:
			mapy = atoi(argv[4]);
			if (mapy < 16) fail("Bad map y size. >=16");
		case 4:
			mapx = atoi(argv[3]);
			if (mapx < 16) fail("Bad map x size. >=16");
		case 3:
			wrapmap = atoi(argv[2]);
			if (wrapmap < 0 || wrapmap > 2) fail("Bad map wrap. 0:no wrap, 1:x-wrap 2:xy-wrap");
		case 2:
			topo = atoi(argv[1]);
			if (topo < 0 || topo > 3) fail("Bad topology, must be 0-3.\n");
		default:
			printf("Map size: %i × %i  Topology: %i (%s)\n", mapx, mapy, topo, topotxt[topo]);
			printf("%3i%% land\n%3i%% mountains/hills\n", land, hillmountain);
			printf("%3i%% tempered\n%3i%% water on land\n", tempered, wateronland);
	}

	if (argc == 1) {
		printf("\nFor a different world:\ntergen topology wrap xsize ysize randomseed land%% hillmountain%% tempered%% wateronland%%\n");
		printf("specify as many parameters as needed\n\n");
		printf("topologies\n0 - squares\n1 - iso squares\n2 - hex\n3 - iso hex\n\n");
		printf("wrap\n0 - no wrap, map has 4 edges\n1 - east/west wrap, top/bottom edges\n2 - wraparound in all directions, and round poles\n\n");
	 	printf("Change randomseed for a different map with the same parameters.\n\n");
		printf("xsize, ysize  size of the map, in tiles. ISO trades height for width\n\n");
		printf("land%%         How many percent of the map is land\n");
		printf("hillmountain%% How much of the land is hills or mountains\n");
		printf("tempered%%     100 no ice, 50 normal, 0 cold planet\n");
		printf("wateronland%%  0 dry world, 20–30 normal, ...\n");
		
	}


	//The terrain:
	tiletype tile[mapx][mapy];

	//Sortable array of pointers to tiles:
	tiletype *tp[mapx*mapy];
	{
		int i = 0, x = mapx, y = mapy;
		while (x--) for (y=mapy; y--;) tp[i++]=&(tile[x][y]);
	}
	mkplanet(land, hillmountain, tempered, wateronland, tile, tp);
}
