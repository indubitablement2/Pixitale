extends RefCounted


func support(spell: Spell) -> void:
	if spell is SpellProjectile:
		spell.num_projectile += 2
