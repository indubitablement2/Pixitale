#![feature(drain_filter)]
#![feature(hash_drain_filter)]
#![feature(map_try_insert)]
#![feature(is_some_and)]
#![feature(variant_count)]
#![feature(option_get_or_insert_default)]
#![feature(int_roundings)]
#![feature(vec_into_raw_parts)]
#![feature(new_uninit)]
#![feature(test)]

// mod battlescape;
// mod client;
// mod client_battlescape;
// mod client_config;
// mod data;
// mod data_builder;
// mod godot_logger;
// mod metascape;
// mod player_inputs;
// mod simulation;
// mod time_manager;
mod cell;
mod chunk;
mod grid_node;
mod material;
mod phf_range;
mod util;

extern crate test;

use ahash::{AHashMap, AHashSet, RandomState};
use indexmap::IndexMap;
use na::{ComplexField, RealField};
use nalgebra as na;
use rand::prelude::*;
use std::f32::consts::{FRAC_PI_2, PI, TAU};

use self::cell::*;
use self::chunk::*;
use self::material::*;

/// 1 cell = 4 godot unit.
pub const CELL_SIZE: f32 = 4.0;

pub type SimRng = rand_xoshiro::Xoroshiro64StarStar;

/// Stored in row-major order.
pub static mut CELLS: *mut u32 = std::ptr::null_mut();
pub static mut WIDTH: i64 = 0;
pub static mut HEIGHT: i64 = 0;

/// Stored in column-major order.
pub static mut CHUNKS: *mut i64 = std::ptr::null_mut();
pub static mut CHUNK_WIDTH: i64 = 0;
pub static mut CHUNK_HEIGHT: i64 = 0;

pub static mut CELL_UPDATED_BIT: u32 = 0;

pub static mut MATERIALS: Vec<Material> = Vec::new();

static mut CELL_ALLOC: usize = 0;
static mut CHUNK_ALLOC: usize = 0;

pub unsafe fn free_grid() {
    if !CELLS.is_null() {
        Vec::from_raw_parts(CELLS, 0, CELL_ALLOC);
        Vec::from_raw_parts(CHUNKS, 0, CHUNK_ALLOC);
        CELLS = std::ptr::null_mut();
        CHUNKS = std::ptr::null_mut();
    }
}

pub fn is_grid_initialized() -> bool {
    unsafe { !CELLS.is_null() }
}

pub unsafe fn new_empty_grid(width: i64, height: i64) {
    free_grid();

    CHUNK_WIDTH = (width / 32).max(3);
    // Make sure chunk height is a multiple of 8,
    // so that cache lines are not shared between threads.
    CHUNK_HEIGHT = (height / 32).max(3).next_multiple_of(8);
    let (chunks, _, chunk_alloc) = vec![0; (CHUNK_WIDTH * CHUNK_HEIGHT) as usize].into_raw_parts();
    CHUNKS = chunks;
    CHUNK_ALLOC = chunk_alloc;

    WIDTH = CHUNK_WIDTH * 32;
    HEIGHT = CHUNK_HEIGHT * 32;
    let (cells, _, cell_alloc) = vec![0; (WIDTH * HEIGHT) as usize].into_raw_parts();
    CELLS = cells;
    CELL_ALLOC = cell_alloc;
}

// TODO: Take material from engine.
#[deprecated]
pub unsafe fn new_test_grid_with_materials() {
    new_empty_grid(0, 0);

    MATERIALS = vec![
        Material {
            display_name: "air".into(),
            color: Default::default(),
            movement: Default::default(),
            density: Default::default(),
            durability: Default::default(),
            collision: Default::default(),
            friction: Default::default(),
            bounciness: Default::default(),
            on_destroyed: Default::default(),
            reactions_range: vec![0, 1 << 32],
            reactions: vec![Reaction {
                probability: 0.1,
                material_out1: None,
                material_out2: Some(0),
            }],
        },
        Material {
            display_name: "react".into(),
            color: Default::default(),
            movement: Default::default(),
            density: Default::default(),
            durability: Default::default(),
            collision: Default::default(),
            friction: Default::default(),
            bounciness: Default::default(),
            on_destroyed: Default::default(),
            reactions_range: Default::default(),
            reactions: Default::default(),
        },
    ];

    for x in 0..WIDTH {
        for y in 0..HEIGHT {
            let mut cell = 0;
            cell.set_material_idx(1);
            set_cell_active(x, y, cell);
        }
    }
    set_cell_active(WIDTH / 2, HEIGHT / 2, 0);
}

pub unsafe fn get_cell(x: i64, y: i64) -> u32 {
    CELLS.add((y * WIDTH + x) as usize).read()
}

unsafe fn set_cell(x: i64, y: i64, cell: u32) {
    CELLS.add((y * WIDTH + x) as usize).write(cell);
}

/// Set cell and also set chunk/cell to active.
pub unsafe fn set_cell_active(x: i64, y: i64, mut cell: u32) {
    cell.set_active(true);
    set_cell(x, y, cell);
    chunk_set_active(x, y);
}

/// Set cell and also set neighbors to active.
pub unsafe fn set_cell_changed(x: i64, y: i64, cell: u32) {
    set_cell_active(x, y, cell);
    set_cell_neightbor_active(x, y);
}

/// Set a cell to active without changing anything else.
pub unsafe fn force_active(x: i64, y: i64) {
    let cell = get_cell(x, y);
    set_cell_active(x, y, cell);
}

/// Set cell's neighbors to active.
pub unsafe fn set_cell_neightbor_active(x: i64, y: i64) {
    force_active(x - 1, y);
    force_active(x + 1, y);
    force_active(x - 1, y - 1);
    force_active(x, y - 1);
    force_active(x + 1, y - 1);
    force_active(x - 1, y + 1);
    force_active(x, y + 1);
    force_active(x + 1, y + 1);
}

unsafe fn step_column(column_idx: usize) {
    let mut rng = SimRng::seed_from_u64(column_idx as u64);

    let chunk_end = column_idx * CHUNK_HEIGHT as usize;
    let chunk_start = chunk_end + CHUNK_HEIGHT as usize - 1;

    let mut next_chunk = CHUNKS.add(chunk_start).read();
    CHUNKS.add(chunk_start).write(0);
    for chunk_idx in chunk_start..chunk_end {
        let chunk = next_chunk;
        next_chunk = CHUNKS.add(chunk_idx + 1).read();
        CHUNKS.add(chunk_idx + 1).write(0);

        if chunk == 0 {
            continue;
        }

        let chunk_y_offset = (chunk_idx % CHUNK_HEIGHT as usize) as i64 * 32;
        let chunk_x_offset = (chunk_idx / CHUNK_HEIGHT as usize) as i64 * 32;
        let (rows, columns) = chunk_active_rect(chunk);
        for local_y in rows.rev() {
            if chunk_is_row_inactive(chunk, local_y) {
                continue;
            }

            let y = chunk_y_offset + local_y;
            for local_x in columns.clone() {
                let x = chunk_x_offset + local_x;
                update_cell(x, y, &mut rng);
            }
        }
    }
}

unsafe fn update_cell(x: i64, y: i64, rng: &mut SimRng) {
    let mut cell = get_cell(x, y);
    let mut active = cell.is_active();

    if !active {
        return;
    }

    if cell.is_updated() {
        if active {
            chunk_set_active(x, y);
        }
        return;
    }

    active = false;
    let mut changed = false;

    // Reactions
    let (mut mat_idx1, mut mat1) = cell.material();
    // x x x
    // . o x
    // . . .
    let reaction_neighbors = [(1, 0), (-1, -1), (0, -1), (1, -1)];
    for neightbor_offset in reaction_neighbors {
        let nx = x + neightbor_offset.0;
        let ny = y + neightbor_offset.1;
        let mut n_cell = get_cell(nx, ny);
        let (mat_idx2, mat2) = n_cell.material();
        match reactions_between(mat_idx1, mat_idx2, mat1, mat2, rng) {
            ReactionResult::None => {}
            ReactionResult::CouldReact => {
                active = true;
            }
            ReactionResult::Reacted {
                material_out1,
                material_out2,
            } => {
                if let Some(out2) = material_out2 {
                    n_cell.set_material_idx(out2);
                    set_cell_changed(nx, ny, n_cell);
                }

                if let Some(out1) = material_out1 {
                    mat_idx1 = out1;
                    mat1 = &MATERIALS[mat_idx1 as usize];
                    changed = true;
                }

                active = true;
            }
        }
    }
    cell.set_material_idx(mat_idx1);

    // TODO: Movement

    cell.set_updated();
    if changed {
        set_cell_changed(x, y, cell);
    } else if active {
        set_cell_active(x, y, cell)
    } else {
        set_cell(x, y, cell);
    }
}

mod ext {
    use godot::prelude::*;

    struct Client {}
    #[gdextension]
    unsafe impl ExtensionLibrary for Client {}
}
