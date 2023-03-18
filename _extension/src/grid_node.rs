use super::*;
use crate::util::*;
use godot::{
    engine::{image::Format, node::InternalMode, Engine, Image, ImageTexture, Sprite2D},
    prelude::*,
};

#[derive(GodotClass)]
#[class(base=Node2D)]
struct Grid {
    draw_pos: Vector2,
    draw_size: Vector2,
    data: PackedByteArray,
    img: Gd<Image>,
    tex: Gd<ImageTexture>,
    sp: Gd<Sprite2D>,

    #[base]
    base: Base<Node2D>,
}
#[godot_api]
impl Grid {
    #[func]
    fn load_data(&self) {
        // TODO:
    }

    #[func]
    fn dbg_print_data(&mut self) {
        // log::info!("{:#?}", Data::data());
    }

    #[func]
    fn set_log_level(&mut self, level: u8) {
        // self.client_config.log_level = level;
        // log::set_max_level(log_level_from_int(level));
    }

    #[func]
    fn get_size(&self) -> Vector2i {
        unsafe { Vector2i::new(WIDTH as i32, HEIGHT as i32) }
    }

    #[func]
    fn new_empty_grid(&mut self, width: i64, height: i64) {
        unsafe {
            new_empty_grid(width, height);
        }
    }

    #[func]
    fn new_test_grid(&mut self) {
        unsafe {
            new_test_grid_with_materials();
        }
    }

    #[func]
    fn draw_rect(&mut self, origin: Vector2, size: Vector2) {
        if !is_grid_initialized() {
            godot_warn!("Grid not initialized");
            return;
        }

        let left = (origin.x / CELL_SIZE) as i64;
        let top = (origin.y / CELL_SIZE) as i64;
        let right = (left + (size.x / CELL_SIZE) as i64 + 1).min(WIDTH);
        let bot = (top + (size.y / CELL_SIZE) as i64 + 1).min(HEIGHT);


        let width = 1;
        let height = 1;
        let mut data = PackedByteArray::new();
        data.resize((width * height * 4) as usize);

        let img =
            Image::create_from_data(width, height, false, Format::FORMAT_RGBA8, data).unwrap();

        self.tex.update(img);
    }
}
#[godot_api]
impl GodotExt for Grid {
    fn init(mut base: Base<Node2D>) -> Self {
        // godot_logger::GodotLogger::init();

        // // TODO: Load configs from file.
        // let client_config: ClientConfig = Default::default();

        // // Apply configs.
        // log::set_max_level(log_level_from_int(client_config.log_level));

        let img = Image::new();
        let tex = ImageTexture::new();
        let mut sp = Sprite2D::new_alloc();
        sp.set_texture(tex.share().upcast());
        base.add_child(
            sp.share().upcast(),
            false,
            InternalMode::INTERNAL_MODE_DISABLED,
        );

        Self {
            base,
            draw_pos: Vector2::ZERO,
            draw_size: Vector2::ZERO,
            data: PackedByteArray::new(),
            img,
            tex,
            sp,
        }
    }

    fn process(&mut self, _delta: f64) {
        // TODO: Do not want to run in edit. Maybe this won't be needed later?
        if Engine::singleton().is_editor_hint() {
            return;
        }

        unsafe {
            for column_idx in 1..CHUNK_WIDTH - 2 {
                step_column(column_idx as usize);
            }
        }
    }
}

// fn log_level_from_int(level: u8) -> log::LevelFilter {
//     match level {
//         0 => log::LevelFilter::Off,
//         1 => log::LevelFilter::Error,
//         2 => log::LevelFilter::Warn,
//         3 => log::LevelFilter::Info,
//         4 => log::LevelFilter::Debug,
//         _ => log::LevelFilter::Trace,
//     }
// }
