extends Node2D

## Mouse global position.
var mouse_position := Vector2.ZERO
## Mouse grid coord. 
## Very similar to mouse_position, because 1 godot unit = 1 grid unit.
var mouse_coord := Vector2i.ZERO

## Time since engine stated. Updated once per frame.
var time := 0.0

func _process(delta: float) -> void:
	mouse_position = get_global_mouse_position()
	mouse_coord = Vector2i(mouse_position.floor())
	time += delta

const PIXEL := preload("res://core/texture/util/pixel.png")

const COLOR_PALETTE : Array[Color] = [
	Color(0.5216, 0.5216, 0.5216),
	Color(0.6353, 0.6353, 0.6353),
	Color(0.7529, 0.7529, 0.7529),
	Color(0.8745, 0.8745, 0.8745),
	Color(1, 1, 1),
	Color(0, 0, 0),
	Color(0.0941, 0.0941, 0.0941),
	Color(0.2, 0.2, 0.2),
	Color(0.302, 0.302, 0.302),
	Color(0.4078, 0.4078, 0.4078),
	Color(1, 0, 0),
	Color(1, 1, 0),
	Color(0, 1, 0),
	Color(0, 1, 1),
	Color(0, 0, 1),
	Color(1, 0, 1),
	Color(0.1882, 0, 0.0784),
	Color(0.3529, 0, 0.1333),
	Color(0.5216, 0, 0.1569),
	Color(0.6941, 0, 0.1333),
	Color(0.8627, 0.098, 0),
	Color(0.9647, 0.3098, 0),
	Color(1, 0.5176, 0.2667),
	Color(1, 0.698, 0.5059),
	Color(0.1412, 0.0627, 0),
	Color(0.2627, 0.149, 0),
	Color(0.3686, 0.2431, 0),
	Color(0.4667, 0.3451, 0),
	Color(0.5608, 0.451, 0),
	Color(0.6471, 0.5686, 0),
	Color(0.7176, 0.6941, 0),
	Color(0.7686, 0.8275, 0),
	Color(0.0706, 0.098, 0),
	Color(0.1216, 0.2078, 0),
	Color(0.1176, 0.3255, 0),
	Color(0, 0.4471, 0.1176),
	Color(0, 0.5569, 0.2941),
	Color(0, 0.6706, 0.4471),
	Color(0, 0.7843, 0.6039),
	Color(0, 0.902, 0.7647),
	Color(0, 0.1059, 0.0902),
	Color(0, 0.2118, 0.2039),
	Color(0, 0.3137, 0.3255),
	Color(0, 0.4157, 0.4588),
	Color(0, 0.5216, 0.6078),
	Color(0, 0.6275, 0.7725),
	Color(0, 0.7333, 0.9608),
	Color(0.5294, 0.8157, 1),
	Color(0, 0.0902, 0.1686),
	Color(0, 0.1804, 0.3529),
	Color(0, 0.2353, 0.6078),
	Color(0.1529, 0.1529, 0.9961),
	Color(0.349, 0.3451, 1),
	Color(0.5216, 0.4784, 1),
	Color(0.6745, 0.6, 1),
	Color(0.8078, 0.7216, 1),
	Color(0.1333, 0, 0.2235),
	Color(0.2784, 0, 0.3765),
	Color(0.4353, 0, 0.498),
	Color(0.6039, 0, 0.5922),
	Color(0.7843, 0, 0.6667),
	Color(0.9765, 0, 0.7176),
	Color(1, 0.451, 0.7137),
	Color(1, 0.6667, 0.7647)
] 
