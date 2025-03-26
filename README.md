# tergen
Terrain generator for the open-souce game Freeciv, using simulated weather &amp; plate tectonics

The goal is realistic looking terrains for the freeciv game. The tergen program makes a scenario savefile, useable for starting a game. All freeciv topolgies are supported. (square tiles, hex tiles, iso or non-iso, no wrap, wrapx and wrapx+wrapy.)

## How tergen works
tergen initialize a height map, with some continents and islands. Many rounds of simulation follows. Each round moves some tectonic plates, and simulates weather and erosion. When the simulation finishes, tergen assigns freeciv terrain types for each tile.

## Realistic features from the simulations
The following features has been observed in generated terrains:
- Mountain ranges following some plate edge in any direction. Not limited to grid directions.
- When a mountain range runs into the sea, it may continue as a land stripe, then a series of islands, and finally a long stripe of shallow water. 
- Rain shadows, where mountains block clouds drifting in from the sea. Terrain beyond the mountains may be of a drier type.
- Rivers may wet the tiles they run through, causing a wetter terrain type.
![River wetting tiles](river-wet.png)
- Erosion changes landscapes. A river in the mountains, may erode them and create a valley. Near the coast, erosion may create a fjord.
![Fjord from erosion](river-fjord.png)

## Plate tectonics
Some plates are created, with random center coordinates and direction of movement. A bigger world has more plates, and the plates are bigger too. Each tile is assigned to the closest plate center, so the plates become convex polygons. After this, many rounds of fractional plate movement. Every plate will move a little in its assigned direction, but only a few plates actually moves each turn. Where plates move apart, rifts appear in the height map. Continents are sometimes torn apart. Where plates collide, mountain ranges are raised by adding up heightmap heights. Such ranges tend to follow plate boundaries. Occationally, an asteroid strikes leaving a crater.

## Weather simulation
For each turn of plate movement, weather is simulated. A temperature map is set up, where land and sea temperature depends on latitude and height. Neighbourhood averaging is used, so a cold land tile may freeze the sea around it close to the poles. Further from the poles, the sea may instead thaw a coast that would otherwise be frozen. 

A round of weather simulation starts with evaporation from sea and other wet tiles. Then the clouds are moved on prevailing winds, using an approximation of earth prevailing winds. Where land and sea meet, there is also a sea breeze moving air from the sea tile to land. Then rain falls. All clouds give some rain, a cloud that has been moved up holds less water and will rain much more. This results in realistic rain shadows. Rain wets the tiles, and runs off in rivers. Rivers generally runs to lower tiles, until the sea is reached. If there is no lower tile, a lake is formed and exit through a higher tile is permitted. The water flow also cause erosion, changing the height map. When there are several lower tiles, rivers prefer to merge with other rivers. This limits overly dense networks of rivers.

## Assigning freeciv terrain types
When the simulation is done, it is time to assign freeciv terrain types. The tiles are sorted on height first. If the user specified 66% sea, then the 66% lowest tiles become sea. The shallowest third will be shallow sea. The rest is land. The tallest tiles become mountains and hills, the rest will be low land. There are some exceptions, all sufficiently cold tiles freeze to arctic tiles. The low land tiles are sorted on wetness, so they can become desert, plains, grassland, forests, and swamps. The exact distribution of tile types depend on the parameters "temperate" and "wateronland" that the user may change. Forests are split into forest or jungle, depending on temperature. The tiles with the most waterflow become river tiles.

The end result is a somewhat realistic terrain. Much flat land, with occational mountain ranges that run independent of tile grid directions. Mountain ranges may happen both on land and in the sea. A sea range may become a long narrow land bar, possibly connecting continents the way north and south America connects. A mountain range running into the sea, is sometimes seen continuing as a string of islands and shallow water – much like the real world.  The weather simulations gives continents and islands that are wettest on the side where prevailing winds bring in clouds from the sea. One can occationally see rain shadows behind mountains. 

## Compiling
To compile, just run 'make'. 

If you get error messages about missing sincosf(), compile with this command instead:
gcc  -march=native -DINTERNAL_SINCOSF -O2 -o tergen tergen.c -lm

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

13 - hex tiles, isometric with extended terrain features.
Isometric seems to double the width and halve the height. Compensate by changing xsize and ysize

Extended terrain features. The terrain may then have hills of many types, such as desert hills, polar hills and so on. There are also volcanoes. You will need a tileset and a ruleset providing such graphics; this is avaliable from my [freecivstuff](https://github.com/Hafting/freecivstuff) repository. 

Currently, only 13 (isometric hex tiles) have tileset support. tergen supports 10, 11 and 12, but there is no tileset providing extended hills for the other topologies.  

### Wrap parameter
0 - no wrap, the game map has 4 edges. Mercator style map, where the north and south edges are polar terrain, and equator follows the middle of the map.

1 - wrap in the x direction, east-west. North and south are still polar edges. Equator at the middle of the map.

2 - wrap in x and y directions. Two round poles are created. Equator is the set of tiles furthest away from both poles. This equator won't be a straight line, but it will be where you find the Amazonas-like forests and swamps.

### Other parameters
xsize and ysize specify the map size in tiles

Specifying a different random seed gives a completely different map. The initial continents, as well as the tectonic plates, are all random.

The land percentage specify how much land and sea there will be. A very high percentage may not work well, as river simulation wants a sea to run into.

The hillmountain percentage specify how much of the land will be hills and mountains. The mountains will be a third of that.

The tempered parameter defaults to 50. Decrease to get more polar terrain, increase to get more deserts. Note that even a very cold planet will have deserts – a desert is dry land, it does not have to be hot. 

The wateronland parameter decides how wet the terrain will be. Increase to get more swamps, forests and rivers. Decrease to get fewer rivers and more desert.
## Use the produced map
The program produces the file tergen.sav, which is a scenario file. Move it into your scenario folder. On Linux, this is ~/.freeciv/scenarios/  Then, start a scenario from the game menu. The name you gave your scenario should be one of the alternatives.
To see all of a map without playing through the game first, use edit mode and become "global observer". 

## Some surprises
* There may be deep water touching the coast, blocking triremes.  This is intentional. Some places are too rough for a trireme, you wouldn't want to go around Cape Horn in one. There are still lots of shallow water. Sometimes, rivers lets you get around a blocked coast. Sometimes, you can place a city strategically, connecting pieces of shallow water. Triremes are still useful, but you cannot sail them everywhere around the world. With time, you get better ships.
* You may find ice away from the poles. These tiles are glaciers. Temperature falls with height, so ice may happen in high places some distance from the poles.
* A few rivers don't go all the way to the sea. Usually, such a river ends in a lake like the dead sea instead. This does not happen often on maps with enough sea, but it is very hard to avoid completely.
* The scenario file uses the civ2civ3 ruleset. If you want something else, open the file tergen.sav in a text editor (vim, notepad, ...) and change civ2civ3 to something else like civ1, civ2, multiplayer, sandbox or whatever you may have.
* A river goes up into the mountains, and down at the other side?  Nope. These are two rivers, starting in the mountains and running to different coasts. A river-capable boat may use them to cross the continent. Presumably, they drag the boat across the watershed like the vikings used to do with their longships.

## Limits
- The smallest world is 16×16.  
- Biggest is unknown. 400×400 takes about a minute to generate, and is enormous. Running time increase with the cube of the size, so expect an 800×800 world to need 10min or so.
- The simulation needs sea. Sea provides water for weather simulation, and termination for rivers. A world with very little sea may fail in strange ways. Earth is 29% land, 71% sea.
