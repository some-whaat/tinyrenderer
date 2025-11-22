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

void draw_line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool is_incrisses_in_y_more_than_in_x = std::abs(ax-bx) < std::abs(ay-by);
    if (is_incrisses_in_y_more_than_in_x) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    float y = ay;
    float slope = static_cast<float>(by - ay) / static_cast<float>(bx - ax);
    for (int x=ax; x<=bx; x++) {
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

ivec2 project_to_screen(vec3 pos, float screen_side) { // only squere screen
    ivec2 proj = ivec2();

    // i need to translate from [-1, 1] to [0, width]
    proj.x = (pos.x/2. + 0.5)* screen_side; // now it's [0, 1]
    // proj.x *= screen_side;
    proj.y = (pos.y/2. + 0.5) * screen_side;

    return proj;
}

int main(int argc, char** argv) {
    constexpr int width  = 888;
    constexpr int height = 888;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    vec3 camera_pos = vec3();

    const std::string model_path = "D:\\CG\\tinyrenderer\\obj\\boggie\\body.obj";
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
        //std::cout << a << b << c << std::endl;
        TGAColor rnd;
        for (int c=0; c<3; c++) rnd[c] = std::rand()%255;
        
        draw_filled_trig(a, b, c, framebuffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
