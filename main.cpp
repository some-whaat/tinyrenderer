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

void find_bound_box_points(int &x_min, int &x_max, int &y_min, int &y_max, const ivec2 &a, const ivec2 &b, const ivec2 &c) {
    //  UGLY :(

    y_max = std::max(std::max(a.y, b.y), c.y);
    x_max = std::max(std::max(a.x, b.x), c.x);

    y_min = std::min(std::min(a.y, b.y), c.y);
    x_min = std::min(std::min(a.x, b.x), c.x);
}

// этот способ удобнее тк можно легко как в шейдере пербирать точки
void draw_filled_trig_boundbox_ver(const ivec2 &a, const float &az, const TGAColor &a_color, const ivec2 &b, const float &bz, const TGAColor &b_color, const ivec2 &c, const float &cz, const TGAColor &c_color, TGAImage &framebuffer, TGAImage &zbuffer) {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
    find_bound_box_points(x_min, x_max, y_min, y_max, a, b, c);

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {

            double all_trig_area = signed_triangle_area(a.x, a.y, b.x, b.y, c.x, c.y);
            double a_coord = signed_triangle_area(x, y, b.x, b.y, c.x, c.y) / all_trig_area;
            double b_coord = signed_triangle_area(a.x, a.y, x, y, c.x, c.y) / all_trig_area;
            double c_coord = signed_triangle_area(a.x, a.y, b.x, b.y, x, y) / all_trig_area;

            unsigned char depth = static_cast<unsigned char>(((az * a_coord) + (bz * b_coord) + (cz * c_coord) + 1) * 225./2);

            if (a_coord < 0 || b_coord < 0 || c_coord < 0 || zbuffer.get(x, y)[0] > depth) {
                continue;
            }


            // std::cout << (az * a_coord) + (bz * b_coord) + (cz * c_coord) << " ";

            TGAColor color;
            color[0] = (a_color[0] * a_coord) + (b_color[0] * b_coord) + (c_color[0] * c_coord);
            color[1] = (a_color[1] * a_coord) + (b_color[1] * b_coord) + (c_color[1] * c_coord);
            color[2] = (a_color[2] * a_coord) + (b_color[2] * b_coord) + (c_color[2] * c_coord);
            color[3] = 225;

            // std::cout << a_coord << "  " << b_coord << "  " << c_coord << "  ";
            zbuffer.set(x, y, {depth});
            framebuffer.set(x, y, color);
        }
    }  
}

ivec2 project_to_screen(vec3 pos, float screen_side) { // only squere screen
    ivec2 proj = ivec2();

    // i need to translate from [-1, 1] to [0, width]
    proj.x = (pos.x/2. + 0.5)* screen_side; // now it's [0, 1]
    // proj.x *= screen_side;
    proj.y = (pos.y/2. + 0.5) * screen_side;

    return proj;
}

int main(int argc, char** argv) {
    constexpr int width  = 1111;
    constexpr int height = 1111;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
    //vec3 camera_pos = vec3();

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
        
        ivec2 a = project_to_screen(model.vert(i, 0).xyz(), width);
        ivec2 b = project_to_screen(model.vert(i, 1).xyz(), width);
        ivec2 c = project_to_screen(model.vert(i, 2).xyz(), width);

        float az = model.vert(i, 0).z;
        float bz = model.vert(i, 1).z;
        float cz = model.vert(i, 2).z;

        // //std::cout << a << b << c << std::endl;
        // TGAColor rnd;
        // for (int c=0; c<3; c++) rnd[c] = std::rand()%255;

        TGAColor a_color = {225,0,0,225};
        TGAColor b_color = {0,225,0,225};
        TGAColor c_color = {0,0,225,225};

        draw_filled_trig_boundbox_ver(a, az, a_color, b, bz, b_color, c, cz, c_color, framebuffer, zbuffer);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}