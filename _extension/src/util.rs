use super::*;

pub trait Lerp {
    fn lerp(self, to: Self, t: f32) -> Self;
    fn slerp(self, to: Self, t: f32) -> Self;
}
impl Lerp for f32 {
    fn lerp(self, to: Self, t: f32) -> Self {
        t.mul_add(to - self, self)
    }

    fn slerp(self, to: Self, t: f32) -> Self {
        let delta = ((to - self + TAU + PI) % TAU) - PI;
        t.mul_add(delta, self + TAU) % TAU
    }
}

pub trait AproxZero {
    fn aprox_zero(self) -> bool;
}
impl AproxZero for f32 {
    fn aprox_zero(self) -> bool {
        ComplexField::abs(self) < 0.001
    }
}
impl AproxZero for na::Vector2<f32> {
    fn aprox_zero(self) -> bool {
        self.x.abs() + self.y.abs() < 0.001
    }
}
