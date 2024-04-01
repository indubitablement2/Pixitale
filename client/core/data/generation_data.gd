extends Object
class_name GenerationData

## Data which generation passes may reference to generate chunks.
##
## Generate one for each Grid.GENERATION_SLICE_CHUNK_SIZE 
## slice of chunks, before generating any chunk in that slice.

const GENERATION_SLICE_CHUNK_SIZE := 1024
const GENERATION_SLICE_CHUNK_SHIFT := GENERATION_SLICE_CHUNK_SIZE / 2

## Slice are shifted so that chunks [-512, 512) is slice_idx 0.
var slice_idx : int
## Slice contains cell with a chunk_coord.x in the range:
## [chunk_left, chunk_left + Grid.GENERATION_SLICE_CHUNK_SIZE)
var chunk_left: int

static func compute_slice_idx(chunk_coord_x: int) -> int:
	return Grid.div_floor(
		chunk_coord_x + GENERATION_SLICE_CHUNK_SHIFT,
		GENERATION_SLICE_CHUNK_SIZE)

func _init(idx: int) -> void:
	slice_idx = idx
	chunk_left = idx * Grid.GENERATION_SLICE_CHUNK_SIZE - GENERATION_SLICE_CHUNK_SHIFT

## Exclusive
func chunk_right() -> int:
	return chunk_left + Grid.GENERATION_SLICE_CHUNK_SIZE
func chunk_mid() -> int:
	return chunk_left + GENERATION_SLICE_CHUNK_SHIFT
