#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/classes/node2d.hpp>

using namespace godot;

class GridCharacterBody : public Node2D {
    GDCLASS(GridCharacterBody, Node2D);

private:
    Vector2 step_offset = Vector2(0.0f, 0.0f);

protected:
    static void _bind_methods();

public:
    Vector2 size = Vector2(10.0f, 10.0f);
    void set_size(Vector2 value);
    Vector2 get_size() const;

    Vector2 velocity = Vector2(0.0f, 0.0f);
    void set_velocity(Vector2 value);
    Vector2 get_velocity() const;

    int max_steps_height = 3;
    void set_max_steps_height(int value);
    int get_max_steps_height() const;

    int min_ledge_width = 2;
    // TODO: set/get

    bool stick_to_floor = true;

    bool collision = true;

    Rect2 get_collision_rect() const;
    bool move();

    virtual void _draw() override;
};
