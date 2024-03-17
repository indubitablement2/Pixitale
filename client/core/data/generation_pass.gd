extends Node
class_name GenerationPass

## Generation
##
## This should be deterministic.
## Seed noise at _ready using Grid.get_seed().

## Generate data for a (large) slice of the Grid.
##
## data may then be used by _generate_chunk to speed up generation
## or to create complex structure.
##
## While this is called on a separate thread,
## unlike _generate_chunk, it has exclusive read/write access to Grid.
##
## Use rand methods from Grid. It is seeded to be independent of time.
##
## For custom data, use data's metadata.
##
## data is never saved or sent over the network to connected peer. 
## Instead it is (re)generated as needed in any order.
func _generate_slice(data: GenerationData) -> void:
	pass

## Generate a chunk.
##
## This is called on a separate thread.
## self may be generating mutiple chunks simultaneously.
## Nothing but iter can be modified. This include %%% and self.
##
## This should return as early as possible when it has nothing to do.
## Eg. When generating surface terrain, check if anywhere near the surface 
## before iterating over each cells or doing expensive operations.
## Generating noise is especially expensive.
##
## To generate independently of time, 
## use rand methods from iter which will always output the same value 
## given the same chunk coordinate and uses from previous generation passes.
##
## To share data between passes, add it as metadata to iter. 
## The same iter is used throughout the chunk generation. 
## It is just reset before each pass.
func _generate_chunk(iter: GridChunkIter, data: GenerationData) -> void:
	pass
