# Game of life  with SDL3
Simple game of life, fully rendered on CPU, only using SDL as a way to render a pixel buffer.
Can render a rotated grid. Single threaded render and simulation (sharing the same thread).


## Usage
### Compilation
SDL3 needed to compile. C++20 compliant.

### Controls
+ Move around the grid with ZQSD (or WASD on qwerty)
+ Rotate the view +/- with H/L
+ Zoom in/out with J/K or mouse wheel
+ Play/pause the simulation with SPACE
+ Speed up/down the simulation with P/O
+ Create/delete a cell with Right/Left Click


## Benchmark
### With a 500x500 grid
+ Fully randomized grid (0.5 of probability of a living cell): O.32ms/tick, 2.5ms/render (full grid, 1080p)
+ Empty grid: O.32ms/tick, 1.5ms/render (full grid, 1080p)

### With a 1000x1000 grid
+ Fully randomized grid (0.5 of probability of a living cell): 1.25ms/tick, 17ms/render (full grid, 1080p)
+ Empty grid: 1.25/tick, 1.9ms/render (full grid, 1080p)
