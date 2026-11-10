# Seam Carving

Content-aware image shrinking: repeatedly removing the least noticeable path of pixels so an image gets narrower or shorter without squashing its important content.

## Language

### Seams

**Seam**:
A connected path of pixels, one per row (vertical) or one per column (horizontal), whose neighbouring members differ by at most one position. Removing it shrinks the image by one pixel in one dimension.
_Avoid_: Path, line, cut (a cut is the graph-cut formulation of a seam, not the seam itself)

**Vertical seam**:
A seam with one pixel in every row; removing it makes the image one column narrower.
_Avoid_: Column seam

**Horizontal seam**:
A seam with one pixel in every column; removing it makes the image one row shorter. Found by running vertical-seam logic on the transposed image.
_Avoid_: Row seam

**Seam cost**:
The sum of the energy of every pixel on a seam. The quantity every algorithm tries to minimise.
_Avoid_: Seam energy, seam weight

**Optimal seam**:
A seam with the lowest possible seam cost in the current image. Ties go to the leftmost (or topmost) candidate so results are deterministic.
_Avoid_: Best seam, shortest path

### Energy

**Energy**:
A per-pixel measure of how visually disruptive removing that pixel would be; low means similar to its neighbours.
_Avoid_: Disruption measure, importance, gradient (gradient is an input to energy, not energy)

**Energy map**:
The grid of energy values for the whole image, recomputed after every seam removal.

**Backward energy**:
Energy computed from the current image alone, as the Sobel gradient magnitude of its grayscale form. The default.
_Avoid_: Standard energy, plain energy

**Forward energy**:
Energy that measures the new neighbour differences created by removing a pixel, rather than the pixel's own gradient. An optional alternative to backward energy.

### Algorithms

**DP seam finder**:
Finds the optimal seam by filling a cumulative-cost table row by row, then backtracking from the cheapest bottom cell.
_Avoid_: Dynamic seam, baseline

**Greedy seam finder**:
Builds a seam by picking the cheapest reachable pixel in each next row, starting from the cheapest top-row pixel, with no look-ahead. Fast but not guaranteed optimal.

**Graph-cut seam finder**:
Finds the optimal seam by modelling the image as a flow network and taking its minimum s–t cut with a max-flow algorithm.
_Avoid_: Min-cut seam, flow seam

**Resize**:
Reaching a target width and height by removing seams. Mixed targets alternate one vertical seam, then one horizontal seam, until both targets are met.
_Avoid_: Crop, scale (both are different operations the brief contrasts against)

### Evaluation

**Seam agreement**:
Two seam finders agree on an image when they return seams with equal seam cost. DP and graph cut must always agree; greedy is expected to cost the same or more.

**Step frame**:
The image captured after one seam removal, with the removed seam drawn on the pre-removal image. The unit of the step-by-step visualisation.
