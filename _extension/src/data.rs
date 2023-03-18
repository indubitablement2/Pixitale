use super::*;
use std::hash::Hash;

/// An entity that can be spawned in a battlescape.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize, Default)]
pub struct EntityDataId(u32);
impl EntityDataId {
    pub fn data(self) -> &'static () {
        &Data::data().entities[self.0 as usize]
    }

    pub fn render_data(self) -> &'static () {
        &Data::data().entities_render[self.0 as usize]
    }
}

#[derive(Debug)]
pub struct Data {
    pub entities_path: AHashMap<String, EntityDataId>,
    pub entities: Vec<()>,
    pub entities_render: Vec<()>,
}
impl Data {
    /// Free all resources.
    /// ## Safety:
    /// Data should not be in use.
    pub fn clear() {
        unsafe {
            DATA = None;
        }
    }

    pub fn add_entity(path: String, entity_data: (), entity_render_data: ()) -> EntityDataId {
        let data = Self::data_mut();
        let id = EntityDataId(data.entities.len() as u32);
        data.entities_path.insert(path, id);
        data.entities.push(entity_data);
        data.entities_render.push(entity_render_data);
        id
    }

    pub fn data() -> &'static Data {
        unsafe { DATA.get_or_insert_default() }
    }

    fn data_mut() -> &'static mut Data {
        unsafe { DATA.get_or_insert_default() }
    }
}
impl Default for Data {
    fn default() -> Self {
        Self {
            entities_path: Default::default(),
            entities: Default::default(),
            entities_render: Default::default(),
        }
    }
}

static mut DATA: Option<Data> = None;
