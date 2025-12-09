#include <cmath>
#include <tuple>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"
#include "fake_gl.h"
#include <filesystem>

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

extern mat<4,4> model_view_matrix, perspective_matrix;
extern std::vector<double> zbuffer;

// struct MyFancyShader : IShader {
//     const Model &model;
//     TGAColor color = {};
//     vec3 tri[3];  // triangle in eye coordinates

//     MyFancyShader(const Model &m) : model(m) {
//     }

//     virtual vec4 vertex(const int face, const int vert) {
//         vec4 v = model.vert(face, vert);                          // current vertex in object coordinates
//         vec4 gl_Position = model_view_matrix * vec4{v.x, v.y, v.z, 1.};
//         tri[vert] = gl_Position.xyz();                            // in eye coordinates
//         return perspective_matrix * gl_Position;                         // in clip coordinates
//     }

//     virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
//         TGAColor a_color = {225,0,0,225};
//         TGAColor b_color = {0,225,0,225};
//         TGAColor c_color = {0,0,225,225};

//         TGAColor color;
//         color[0] = (a_color[0] * bar[0]) + (b_color[0] * bar[1]) + (c_color[0] * bar[2]);
//         color[1] = (a_color[1] * bar[0]) + (b_color[1] * bar[1]) + (c_color[1] * bar[2]);
//         color[2] = (a_color[2] * bar[0]) + (b_color[2] * bar[1]) + (c_color[2] * bar[2]);
//         color[3] = 225;

//         return {false, color};                                  // do not discard the pixel
//     }
// };

int main(int argc, char** argv) {

    ivec2 screen_sides;
    screen_sides.x = 888;
    screen_sides.y = 999;

    init_viewport_matrix(screen_sides, 255.); // IMPORTANT


    constexpr int width  = 1111;
    constexpr int height = 1111;
    TGAImage framebuffer(screen_sides.x, screen_sides.y, TGAImage::RGB);
    TGAImage zbuffer(screen_sides.x, screen_sides.y, TGAImage::GRAYSCALE);
    //vec3 camera_pos = vec3();

    // const std::string model_path = (std::filesystem::current_path().string() + "\\" + "obj/boggie/body.obj");
    // const std::string model_path = "/home/somewhat/projects/grathics_stuff/tinyrenderer/obj/african_head/african_head.obj";
    // const std::string model_path = "/home/somewhat/projects/grathics_stuff/tinyrenderer/obj/boggie/body.obj";
    const std::string model_path = "/home/somewhat/projects/grathics_stuff/tinyrenderer/obj/diablo3_pose/diablo3_pose.obj";
    // const std::string model_path = "/home/somewhat/projects/grathics_stuff/tinyrenderer/obj/my_fish.obj";

    if (!std::filesystem::exists(model_path)) {
        std::cerr << "Model file not found: " << model_path << std::endl;
        return 1;
    }

    Model model = Model(model_path);
    
    std::cout << model.nfaces() << std::endl;

    for (int i = 0; i < model.nfaces(); i++) {
        
        // ivec2 a = project_to_screen(model.vert(i, 0).xyz(), width);
        // ivec2 b = project_to_screen(model.vert(i, 1).xyz(), width);
        // ivec2 c = project_to_screen(model.vert(i, 2).xyz(), width);

        Triangle trig = { model.vert(i, 0), model.vert(i, 1), model.vert(i, 2)};

        rasterize(trig, framebuffer);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    // zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}