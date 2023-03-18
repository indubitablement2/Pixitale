use super::*;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum CellMovement {
    #[default]
    Solid,
    Powder,
    Liquid,
    Gas,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum CellCollision {
    #[default]
    Solid,
    Platform,
    Liquid,
    None,
}

#[derive(Debug, Clone)]
pub struct Material {
    pub display_name: String,
    pub color: (),
    pub movement: CellMovement,
    pub density: f32,

    pub durability: f32,

    pub collision: CellCollision,
    pub friction: f32,
    pub bounciness: f32,

    pub on_destroyed: (),

    /// Has all reactions with material that have idx >= this material's idx.
    pub reactions_range: Vec<u64>,
    pub reactions: Vec<Reaction>,
}

#[derive(Debug, Clone)]
pub struct Reaction {
    pub probability: f64,
    pub material_out1: Option<u32>,
    pub material_out2: Option<u32>,
}

// 0) a -> 0) a, 1) b, 2) c, 3) d
// 1) b -> 0) b, 1) c, 2) d
// 2) c -> 0) c, 1) d
// 3) d -> 0) d
pub fn reactions_between<'a>(
    mat_idx1: u32,
    mat_idx2: u32,
    mat1: &'static Material,
    mat2: &'static Material,
    rng: &'a mut SimRng,
) -> ReactionResult {
    let (mat, idx, switch) = if mat_idx1 <= mat_idx2 {
        (mat1, mat_idx2 - mat_idx1, false)
    } else {
        (mat2, mat_idx1 - mat_idx2, true)
    };

    let reactions = if let Some(range) = mat.reactions_range.get(idx as usize) {
        let start = (range & 0xFFFF_FFFF) as usize;
        let end = (range >> 32) as usize;
        &mat1.reactions[start..end]
    } else {
        return ReactionResult::None;
    };

    if reactions.is_empty() {
        return ReactionResult::None;
    }

    for reaction in reactions {
        if rng.gen_bool(reaction.probability) {
            if switch {
                return ReactionResult::Reacted {
                    material_out1: reaction.material_out2,
                    material_out2: reaction.material_out1,
                };
            } else {
                return ReactionResult::Reacted {
                    material_out1: reaction.material_out1,
                    material_out2: reaction.material_out2,
                };
            }
        }
    }

    ReactionResult::CouldReact
}

pub enum ReactionResult {
    None,
    CouldReact,
    Reacted {
        material_out1: Option<u32>,
        material_out2: Option<u32>,
    },
}
