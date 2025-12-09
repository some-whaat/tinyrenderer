#include <cmath>
#include <tuple>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"
#include <filesystem>

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

typedef vec4 Triangle[3];

// mat<4,4> perspective_matrix /*изменение с помощю этого thrustom'a*/;
// std::vector<double> zbuffer;


TGAColor my_cool_fancy_fragment_shader(const vec3 bar) {
    TGAColor a_color = {225, 0, 0, 225};
    TGAColor b_color = {111, 111, 0, 225};
    TGAColor c_color = {0, 111, 225, 225};

    TGAColor color;
    color[0] = static_cast<unsigned char>((a_color[0] * bar[0]) + (b_color[0] * bar[1]) + (c_color[0] * bar[2]));
    color[1] = static_cast<unsigned char>((a_color[1] * bar[0]) + (b_color[1] * bar[1]) + (c_color[1] * bar[2]));
    color[2] = static_cast<unsigned char>((a_color[2] * bar[0]) + (b_color[2] * bar[1]) + (c_color[2] * bar[2]));
    color[3] = 225;
    return color;
}

mat<4,4> make_model_view_matrix(float y_rot) { //(vec4 rot_quaternion) { /*типа вращение + affine transform, насклько понимаю*/
    mat<4,4> rot_matrix_y = mat<4,4>{{
        {cos(y_rot), 0, sin(y_rot), 0},
        {0, 1, 0, 0},
        {-sin(y_rot), 0, cos(y_rot), 0},
        {0, 0, 0, 1}
    }};

    return rot_matrix_y;
}

mat<4,4> make_viewport_matrix(const ivec2 &screen_sides, const float &z_depth) {
    // translate x and y from [-1, 1] to [0, width] and [0, height]
    // translate z from [-1, 1] to [0, z_depth] for the z-buffer
    
    mat<4,4> viewport_matrix = mat<4,4>{{
        {screen_sides.x / 2.0, 0, 0, screen_sides.x / 2.0},
        {0, screen_sides.y / 2.0, 0, screen_sides.y / 2.0},
        {0, 0, z_depth / 2.0, z_depth / 2.0},
        {0, 0, 0, 1}
    }};

    return viewport_matrix;
}



// https://en.wikipedia.org/wiki/Shoelace_formula
double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

void draw_line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool is_incrisses_in_y_more_than_in_x = std::abs(ax-bx) < std::abs(ay-by);
    if (is_incrisses_in_y_more_than_in_x) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    float y = ay;
    float slope = static_cast<float>(by - ay) / static_cast<float>(bx - ax);
    for (int x = ax; x <= bx; x++) {
        int y_coord = std::round(y);
        if (is_incrisses_in_y_more_than_in_x) // if transposed, de−transpose
            framebuffer.set(y_coord, x, color);
        else
            framebuffer.set(x, y_coord, color);
        
        y += slope;
    }
}


void draw_trig(ivec2 a, ivec2 b, ivec2 c, TGAImage &framebuffer) {
    draw_line(a.x, a.y, b.x, b.y, framebuffer, white);
    draw_line(b.x, b.y, c.x, c.y, framebuffer, white);
    draw_line(c.x, c.y, a.x, a.y, framebuffer, white);
}

void draw_filled_trig(ivec2 a, ivec2 b, ivec2 c, TGAImage &framebuffer, TGAColor color) {
    if (a.x > b.x) std::swap(a, b);
    if (a.x > c.x) std::swap(a, c);
    if (b.x > c.x) std::swap(b, c);
    
    if (a.x == c.x) return;
    

    float slope_ac = static_cast<float>(c.y - a.y) / (c.x - a.x);
    float slope_ab = static_cast<float>(b.y - a.y) / (b.x - a.x);
    
    float y_ac = a.y;
    float y_ab = a.y;
    
    for (int x = a.x; x <= b.x; x++) {
        draw_line(x, std::round(y_ac), x, std::round(y_ab), framebuffer, color);
        y_ac += slope_ac;
        y_ab += slope_ab;
    }
    
    y_ac = a.y + slope_ac * (b.x - a.x);
    float y_bc = b.y;
    float slope_bc = static_cast<float>(c.y - b.y) / (c.x - b.x);
    
    for (int x = b.x; x <= c.x; x++) {
        draw_line(x, std::round(y_ac), x, std::round(y_bc), framebuffer, color);
        y_ac += slope_ac;
        y_bc += slope_bc;
    }
}

void find_bound_box_points(int &x_min, int &x_max, int &y_min, int &y_max, const Triangle &trig) {
    //  UGLY :(

    y_max = std::max(std::max(trig[0].y, trig[1].y), trig[2].y);
    x_max = std::max(std::max(trig[0].x, trig[1].x), trig[2].x);

    y_min = std::min(std::min(trig[0].y, trig[1].y), trig[2].y);
    x_min = std::min(std::min(trig[0].x, trig[1].x), trig[2].x);
}

void draw_filled_trig_boundbox(const Triangle &trig, TGAImage &framebuffer, TGAImage &zbuffer) {

    int x_min;
    int x_max;
    int y_min;
    int y_max;

    find_bound_box_points(x_min, x_max, y_min, y_max, trig);

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {

            double all_trig_area = signed_triangle_area(trig[0].x, trig[0].y, trig[1].x, trig[1].y, trig[2].x, trig[2].y);
            double a_coord = signed_triangle_area(x, y, trig[1].x, trig[1].y, trig[2].x, trig[2].y) / all_trig_area;
            double b_coord = signed_triangle_area(trig[0].x, trig[0].y, x, y, trig[2].x, trig[2].y) / all_trig_area;
            double c_coord = signed_triangle_area(trig[0].x, trig[0].y, trig[1].x, trig[1].y, x, y) / all_trig_area;

            unsigned char depth = static_cast<unsigned char>(((trig[0].z * a_coord) + (trig[1].z * b_coord) + (trig[2].z * c_coord) + 1));

            if (a_coord < 0 || b_coord < 0 || c_coord < 0 || zbuffer.get(x, y)[0] > depth) { // можно разделить на 2 if'a для efficency
                continue;
            }

            TGAColor color = my_cool_fancy_fragment_shader(vec3{a_coord, b_coord, c_coord});

            zbuffer.set(x, y, {depth});
            framebuffer.set(x, y, color);
        }
    }
}

// ivec2 project_to_screen(vec3 pos, float screen_side) { // only squere screen
//     ivec2 proj = ivec2();
//
//     // i need to translate from [-1, 1] to [0, width]
//     proj.x = (pos.x/2. + 0.5)* screen_side; // now it's [0, 1]
//     // proj.x *= screen_side;
//     proj.y = (pos.y/2. + 0.5) * screen_side;
//
//     return proj;
// }

int main(int argc, char** argv) {

    constexpr int width  = 999;
    constexpr int height = 999;
    constexpr float z_depth = 255.0f;

    mat<4,4> model_view_matrix = make_model_view_matrix(111);
    mat<4,4> viewport_matrix = make_viewport_matrix(ivec2{width, height}, z_depth);

    mat<4, 4> the_one_holy_matrix = viewport_matrix * model_view_matrix;

    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    // const std::string model_path = (std::filesystem::current_path().string() + "\\" + "obj/boggie/body.obj");
    const std::string model_path = "/home/somewhat/projects/grathics_stuff/tinyrenderer/obj/african_head/african_head.obj";
    // const std::string model_path = "/home/somewhat/projects/grathics_stuff/tinyrenderer/obj/boggie/body.obj";

    if (!std::filesystem::exists(model_path)) {
        std::cerr << "Model file not found: " << model_path << std::endl;
        return 1;
    }

    Model model = Model(model_path);
    
    std::cout << model.nfaces() << std::endl;


    for (int i = 0; i < model.nfaces(); i++) {

        Triangle trig = { model.vert(i, 0), model.vert(i, 1), model.vert(i, 2)};
        
        trig[0] =  the_one_holy_matrix * trig[0];
        trig[1] =  the_one_holy_matrix * trig[1];
        trig[2] =  the_one_holy_matrix * trig[2];

        draw_filled_trig_boundbox(trig, framebuffer, zbuffer);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}