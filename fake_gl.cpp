#include <limits>
#include <algorithm>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"
#include "fake_gl.h"

mat<4,4> model_view_matrix /*типа вращение + affine transform, насклько понимаю*/, viewport_matrix /*сопсна проекция на экран + z*/, perspective_matrix /*изменение с помощю этого thrustom'a*/;
std::vector<double> zbuffer;


TGAColor my_cool_fancy_fragment_shader(const vec3 bar) {
    
    TGAColor a_color = {225,0,0,225};
    TGAColor b_color = {0,225,0,225};
    TGAColor c_color = {0,0,225,225};

    TGAColor color;
    color[0] = (a_color[0] * bar[0]) + (b_color[0] * bar[1]) + (c_color[0] * bar[2]);
    color[1] = (a_color[1] * bar[0]) + (b_color[1] * bar[1]) + (c_color[1] * bar[2]);
    color[2] = (a_color[2] * bar[0]) + (b_color[2] * bar[1]) + (c_color[2] * bar[2]);
    color[3] = 225;

    return color;
}


void init_viewport_matrix(const ivec2 &screen_sides, const float &z_depth) {
    // i need to translate the x and y from [-1, 1] to [0, width] and [0, hight]
    // and z from [-1, 1] to [0, 255] for the zbuffer (as 255 is owr color range)

    // proj.x = (pos.x/2. + 0.5) * screen_sides.x;
    // proj.y = (pos.y/2. + 0.5) * screen_sides.y;
    // proj.z = (pos.z/2. + 0.5) * 255.;

    viewport_matrix = {
        {screen_sides.x/2., 0, 0, screen_sides.x/2.},
        {0, screen_sides.y/2., 0, screen_sides.y/2.},
        {0, 0, z_depth/2., z_depth/2.}, // for the depth buffer
        {0, 0, 0, 1}
    };
}

// https://en.wikipedia.org/wiki/Shoelace_formula
double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

void find_bound_box_points(int &x_min, int &x_max, int &y_min, int &y_max, const Triangle &trig) {
    //  UGLY :(

    y_max = std::max(std::max(trig[0].y, trig[1].y), trig[2].y);
    x_max = std::max(std::max(trig[0].x, trig[1].x), trig[2].x);

    y_min = std::min(std::min(trig[0].y, trig[1].y), trig[2].y);
    x_min = std::min(std::min(trig[0].y, trig[1].y), trig[2].y);
}

// этот способ удобнее тк можно легко как в шейдере пербирать точки
void draw_filled_trig_boundbox(const Triangle &trig, TGAImage &framebuffer) {

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

            unsigned int depth = (((trig[0].z * a_coord) + (trig[1].z * b_coord) + (trig[2].z * c_coord) + 1) * 225./2);

            if (a_coord < 0 || b_coord < 0 || c_coord < 0 || zbuffer[x + y * framebuffer.width()] > depth) { // можно разделить на 2 if'a для efficency
                continue;
            }


            // std::cout << (az * a_coord) + (bz * b_coord) + (cz * c_coord) << " ";

            TGAColor color = my_cool_fancy_fragment_shader(vec3(a_coord, b_coord, c_coord));

            // std::cout << a_coord << "  " << b_coord << "  " << c_coord << "  ";
            zbuffer[x + y * framebuffer.width()] = depth;
            framebuffer.set(x, y, color);
        }
    }
}

void rasterize(Triangle &trig, TGAImage &framebuffer) {

    trig[0] = trig[0] * viewport_matrix;
    trig[1] = trig[1] * viewport_matrix;
    trig[2] = trig[2] * viewport_matrix;

    // //std::cout << a << b << c << std::endl;
    // TGAColor rnd;
    // for (int c=0; c<3; c++) rnd[c] = std::rand()%255;

    TGAColor a_color = {225,0,0,225};
    TGAColor b_color = {0,225,0,225};
    TGAColor c_color = {0,0,225,225};

    draw_filled_trig_boundbox(trig, framebuffer);

    // framebuffer.write_tga_file("framebuffer.tga");
    
}

