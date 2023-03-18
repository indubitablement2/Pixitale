use super::*;

const MATERIAL_MASK: u32 = 0xFFF;

const UPDATED_SHIFT: u32 = 12;
const UPDATED_MASK: u32 = 1 << UPDATED_SHIFT;

const ACTIVE_SHIFT: u32 = 13;
const CELL_ACTIVE_MASK: u32 = 1 << ACTIVE_SHIFT;

const COLOR_SHIFT: u32 = 24;
const COLOR_MASK: u32 = 0xFF << COLOR_SHIFT;

/// - Material: 0-12
/// - Updated bit: 12
/// - Active: 13
/// - Unused: 14, 15
/// - Shade/velx: 16-24
/// - Color/vely: 24-32
pub trait CellTrait {
    fn material_idx(&self) -> u32;
    fn set_material_idx(&mut self, material: u32);
    fn material(&self) -> (u32, &'static Material) {
        let idx = self.material_idx();
        (idx, unsafe { MATERIALS.get_unchecked(idx as usize) })
    }

    fn is_updated(&self) -> bool;
    fn set_updated(&mut self);

    fn is_active(&self) -> bool;
    fn set_active(&mut self, active: bool);

    fn color(&self) -> u8;
    fn set_color(&mut self, color: u8);
}
impl CellTrait for u32 {
    fn material_idx(&self) -> u32 {
        (self & MATERIAL_MASK) as u32
    }

    fn set_material_idx(&mut self, material: u32) {
        *self = (*self & !MATERIAL_MASK) | material;
    }

    fn is_updated(&self) -> bool {
        unsafe { (self & UPDATED_MASK) == CELL_UPDATED_BIT }
    }

    fn set_updated(&mut self) {
        unsafe {
            *self = (*self & !UPDATED_MASK) | CELL_UPDATED_BIT;
        }
    }

    fn is_active(&self) -> bool {
        (self & CELL_ACTIVE_MASK) != 0
    }

    fn set_active(&mut self, active: bool) {
        if active {
            *self |= CELL_ACTIVE_MASK;
        } else {
            *self &= !CELL_ACTIVE_MASK;
        }
    }

    fn color(&self) -> u8 {
        ((self & COLOR_MASK) >> COLOR_SHIFT) as u8
    }

    fn set_color(&mut self, color: u8) {
        *self = (*self & !COLOR_MASK) | ((color as u32) << COLOR_SHIFT);
    }
}
