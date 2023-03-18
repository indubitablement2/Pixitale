pub mod cell;
mod chunk;
pub mod material;

use self::cell::*;
use super::*;

pub struct Simulation {}

#[derive(Debug, Clone, Copy)]
pub struct Grid {
    /// Row major.
    cells: *mut u32,
    width: u64,
    height: u64,
    cell_alloc: u32,

    /// Column major.
    chunks: *mut u64,
    chunk_width: u64,
    chunk_alloc: u32,

    cell_update: CellUpdate,
}
impl Grid {
    fn new_empty(mut chunk_width: u64, mut chunk_height: u64) -> Self {
        chunk_width = chunk_width.max(3);
        chunk_height = chunk_height.max(3);
        // Make sure chunk height is a multiple of 8,
        // so that cache lines are not shared between threads.
        chunk_height = chunk_height.next_multiple_of(8);

        let width = chunk_width * 32;
        let height = chunk_height * 32;
        let (cells, _, cell_alloc) = vec![0u32; (width * height) as usize].into_raw_parts();

        let (chunks, _, chunk_alloc) =
            vec![u64::MAX; (chunk_width * chunk_height) as usize].into_raw_parts();

        Self {
            cells,
            width,
            height,
            cell_alloc: cell_alloc as u32,
            chunks,
            chunk_width,
            chunk_alloc: chunk_alloc as u32,
            cell_update: Default::default(),
        }
    }

    fn chunk_idx(&self, x: u64, y: u64) -> u64 {
        (y / 32) + (x / 32) * self.chunk_width
    }

    fn chunk(&mut self, x: u64, y: u64) -> &mut u64 {
        let idx = self.chunk_idx(x, y);
        unsafe { &mut *self.chunks.add(idx as usize) }
    }

    fn free(&mut self) {
        unsafe {
            Vec::from_raw_parts(self.cells, 0, self.cell_alloc as usize);
            Vec::from_raw_parts(self.chunks, 0, self.chunk_alloc as usize);
        }
    }
}
impl Default for Grid {
    fn default() -> Self {
        Self::new_empty(0, 0)
    }
}

trait CoordTrait {
    fn to_chunk_local(self) -> Self;
}
impl CoordTrait for u64 {
    fn to_chunk_local(self) -> Self {
        self % 32
    }
}