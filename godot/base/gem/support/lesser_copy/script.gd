extends Object


func support(spell: Spell) -> void:
	spell.num_cast += 1
	spell.spread += 0.1
	spell.direct_cast_cooldown += 0.1
