# tergen
Terrain generator for the game Freeciv, using simulated weather &amp; plate tectonics

The goal is realistic looking terrains for the freeciv game. The tergen program makes a scenario savefile, useable for starting a game. Most freeciv topolgies are supported. (square tiles, hex tiles, iso or non-iso, no wrap, wrapx and wrapx+wrapy.)

## How tergen works
tergen initialize a height map, with some continents and islands. A couple of height averaging steps smooths this out, avoiding problems later on.

Then plate tectonics is simulated. Some plates are created, with random center coordinates and direction of movement. Each tile is assigned to the closest plate, so the plates become convex polygons. After this, a 100 rounds of plate movement, where every plate may move up to 5 tiles in any direction. Where paltes move apart, rifts appear in the height map. Continents are sometimes torn apart. Where plates collide, mountain ranges are raised by adding up heightmap heights. Such ranges tend to follow plate boundaries.

## Weather simulation
For each of the 100 turns of plate movement, weather is simulated. A temperature map is set up, where land and sea temperature depends on latitude and height. Neighbourhood averaging is used, so a cold land tile may freeze the sea around it close to the poles. Further from the poles, the sea may instead thaw a coast that would otherwise be frozen. 

A round of weather simulation starts with evaporation from sea and other wet tiles. Then the clouds are moved on prevailing winds, using an approximation of earth prevailing winds. Where land and sea meet, there is also a sea breeze moving air from the sea tile to land. Then rain falls. All clouds give some rain, a cloud that has been moved up holds less water and will rain much more. This results in realistic rain shadows. Rain wets the tiles, and runs off in rivers. Rivers generally runs to lower tiles, until the sea is reached. If there is no lower tile, a lake is formed and exit through a higher tile is permitted. The water flow also cause erosion, changing the height map. When there are several lower tiles, rivers prefer to merge with other rivers. This avoids overly dense networks of rivers.

## Assigning freeciv terrain types
When the simulation is done, it is time to assign freeciv terrain types. The tiles are sorted on height first. If the user specified 33% sea, then the 33% lowest tiles become sea. One third will be shallow sea. The rest is land. The talles tiles become mountains and hills, the rest will be low land. There are some exceptions, all sufficiently cold tiles freeze to arctic tiles. The low land tiles are sorted on wetness, so they can become desert, plains, grassland, forests and swamps. The exact distribution of tile types depend on the parameters "temperate" and "wateronland" that the user may change. Forests are split into forest or jungle, depending on temperature. The tiles with the most waterflow become river tiles.

The end result is a somewhat realistic terrain. Much flat land, with occational mountain ranges that run independent of tile grid directions. Mountain ranges may happen both on land and in the sea. A sea range may become a long narrow land bar, possibly connecting continents the way north and south America connects. The weather simulations gives continents and islands that are wettest on the side where prevailing winds bring in clouds from the sea. One can sometimes see rain shadows behind mountains. 

## Compiling
To compile, just run 'make'. 

If you get error messages about missing sincosf(), compile with this command instead:
gcc  -march=native -DINTERNAL_SINCOSF -O2 -lm -o tergen tergen.c

## Program usage
./tergen name topology wrapping xsize ysize randomseed land% hillmountain% tempered% wateronland%

All parameters has defaults and may be omitted.
### Name
The name is stored in the generated file (tergen.sav), and will appear in the scenario list in the freeciv GUI.

### Topologies
0 - square tiles

1 - square tiles, isometric

2 - hex tiles

3 - hex tiles, isometric

Isometric seems to double the with and halve the height. Compensate by changing xsize and ysize
### Wrap parameter
0 - no wrap, the game map has 4 edges. Mercator style map, where the north and south edges are polar terrain, and equator follows the middle of the map.

1 - wrap in the x direction, east-west. North and south are still polar edges.


2 - wrap in x and y directions. Two round poles are created. Equator is the set of tiles furthest away from both poles.
### Other parameters
xsize and ysize specify the map size in tiles

Specifying a different random seed gives a completely different map. The initial continents, as well as the tectonic plates, are all random.

The land percentage specify how much land and sea there will be.

The hillmountain percentage specify how much of the land will be hills and mountains. The mountains will be a third of that.

The tempered parameter defaults to 50. Decrease to get more polar terrain, increase to get more deserts. Note that even a very cold planet will have deserts – a desert is dry land, it does not have to be hot. 

The wateronland parameter decides how wet the terrain will be. Increase to get more swamps, forests and rivers. Decrease to get fewer rivers and more desert.
## Use the produced map
The program produces the file tergen.sav, which is a scenario file. Move it into your scenario folder. On Linux, this is ~/.freeciv/scenarios/  Then, start a scenario from the game menu. "Tergen" should be one of the alternatives.
To see all of a map without playing through the game first, use edit mode and become "global observer". 

## Some surprises
* There may be deep water touching the coast, blocking triremes.  This is intentional. Some places are too rough for a trireme, you wouldn't want to go around Cape Horn in one. There are still lots of shallow water. Sometimes, rivers lets you get around a blocked coast. Sometimes, you can place a city strategically, connecting pieces of shallow water. Triremes are still useful, but you cannot sail them everywhere around the world. With time, you get better ships.
* You may find ice away from the poles. These tiles are glaciers. Temperature falls with height, so ice may happen in high places on islands some distance from the poles.
* A few rivers don't go all the way to the sea. Usually, such a river ends in a lake like the dead sea instead. This does not happen often, but it is very hard to avoid.
* The scenario file uses the civ2civ3 ruleset. If you want something else, open the file tergen.sav in a text editor (vim, notepad, ...) and change civ2civ3 to something else like civ1, civ2, multiplayer, sandbox or whatever you may have.
