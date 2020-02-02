#include <SDL2/SDL.h>

#include <iostream>
#include <cmath>

namespace {
    void DrawCircle(SDL_Renderer* renderer, int centreX, int centreY, int radius) noexcept {
       const int diameter = radius * 2;
       int x = radius - 1;
       int y = 0;
       int tx = 1;
       int ty = 1;
       int error = tx - diameter;
       while (x >= y) {
           const SDL_Point points[]{{centreX + x, centreY - y},
                                    {centreX + x, centreY + y},
                                    {centreX - x, centreY - y},
                                    {centreX - x, centreY + y},
                                    {centreX + y, centreY - x},
                                    {centreX + y, centreY + x},
                                    {centreX - y, centreY - x},
                                    {centreX - y, centreY + x}};
          ::SDL_RenderDrawPoints(renderer, points, 8);
          if (error <= 0) {
             ++y;
             error += ty;
             ty += 2;
          }
          if (error > 0) {
             --x;
             tx += 2;
             error += tx - diameter;
          }
       }
    }

    struct Scene {
        struct {
            float a;
            float b;
            float angle;
            float angular_velocity;
        } rect_pend{5, 14, 80.0 * M_PI / 180.0, 0.0};

        struct {
            float radius;
            float angle;
            float angular_velocity;
        } circle_pend{4, 140 * M_PI / 180.0, 0.0};

        float gravity = 9.8;

        void update(int dt) noexcept {
            struct Vec {
                float x1, x2, x3, x4;
                Vec& operator+=(const Vec& vec) noexcept {
                    x1 += vec.x1;
                    x2 += vec.x2;
                    x3 += vec.x3;
                    x4 += vec.x4;
                    return *this;
                }
                Vec operator+(const Vec& vec) const noexcept {return {x1 + vec.x1, x2 + vec.x2, x3 + vec.x3, x4 + vec.x4};}
                Vec operator*(float scalar) const noexcept {return {x1 * scalar, x2 * scalar, x3 * scalar, x4 * scalar};}
                Vec operator/(float scalar) const noexcept {const float inv_scalar = 1.0 / scalar; return *this * inv_scalar;}
            };

            auto f = [this](const Vec& vec) noexcept -> Vec {
                const float sin1 = std::sin(vec.x1), sin2 = std::sin(vec.x3), sinV = std::sin(vec.x1 - vec.x3), cosV = std::cos(vec.x1 - vec.x3);
                const float& a = rect_pend.a, &b = rect_pend.b, &r = circle_pend.radius;
                const float sqrt_ab = std::sqrt(a * a + b * b);
                const float f2 = -(3.0 * a * b * gravity * sin1 / 2.0 + 2 * M_PI * r * r * cosV * (vec.x2 * vec.x2 * sqrt_ab * sinV - gravity * sin2) + 3.0 * vec.x4 * vec.x4 * M_PI * r * r * r * sinV + 3.0 * gravity * M_PI * r * r * sin1) / (sqrt_ab * (a * b + 2.0 * M_PI * r * r * sinV * sinV + M_PI * r * r));
                const float f4 = ((2.0 * a * b / 3.0 + 2.0 * M_PI * r * r) * (vec.x2 * vec.x2 * sqrt_ab * sinV - gravity * sin2) + cosV * (a * b * gravity * sin1 + 2.0 * vec.x4 * vec.x4 * M_PI * r * r * r * sinV + 2.0 * gravity * M_PI * r * r * sin1)) / (r * (a * b + 2.0 * M_PI * r * r * sinV * sinV + M_PI * r * r));
                return {vec.x2, f2, vec.x4, f4};
            };

            static constexpr float h = 0.025;

            static constexpr float ms_per_step = 1000.0 / 80.0;
            static int t = 0;

            for (t += dt; t >= ms_per_step; t -= ms_per_step) {
                const Vec y0{rect_pend.angle, rect_pend.angular_velocity, circle_pend.angle, circle_pend.angular_velocity};

                const Vec k1 = f(y0);
                const Vec k2 = f(y0 + k1 * h * .5);
                const Vec k3 = f(y0 + k2 * h * .5);
                const Vec k4 = f(y0 + k3 * h);

                const Vec y1 = y0 + (k1 + k2 * 2.0 + k3 * 2.0 + k4) * h / 6.0;

                rect_pend.angle            = y1.x1;
                rect_pend.angular_velocity = y1.x2;
                circle_pend.angle            = y1.x3;
                circle_pend.angular_velocity = y1.x4;
            }
        }

        void render(SDL_Renderer* renderer, int x, int y, int scale) const noexcept {

            const int proj_a_x = -rect_pend.a * std::cos(rect_pend.angle) * scale;
            const int proj_a_y =  rect_pend.a * std::sin(rect_pend.angle) * scale;

            const int proj_b_x = rect_pend.b * std::sin(rect_pend.angle) * scale;
            const int proj_b_y = rect_pend.b * std::cos(rect_pend.angle) * scale;

            ::SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

            const SDL_Point line_poinst[]{{x, y}, {x + proj_a_x, y + proj_a_y}, {x + proj_a_x + proj_b_x, y + proj_a_y + proj_b_y}, {x + proj_b_x, y + proj_b_y}, {x, y}, 
                                                                                {x + proj_a_x + proj_b_x, y + proj_a_y + proj_b_y}};
            ::SDL_RenderDrawLines(renderer, line_poinst, 6);

            ::SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);

            const int circle_radius = circle_pend.radius * scale;
            const int c_x = x + proj_a_x + proj_b_x + circle_radius * std::sin(circle_pend.angle);
            const int c_y = y + proj_a_y + proj_b_y + circle_radius * std::cos(circle_pend.angle);
            ::DrawCircle(renderer, c_x, c_y, circle_radius);
            ::SDL_RenderDrawLine(renderer, c_x, c_y, x + proj_a_x + proj_b_x, y + proj_a_y + proj_b_y);
        }
    };
}

int main() {
    try {
        const struct SDL {
            SDL() {
                if (::SDL_Init(SDL_INIT_VIDEO) < 0)
                    throw std::runtime_error(::SDL_GetError());
            }
            ~SDL() {::SDL_Quit();}
        } sdl;
        const struct Window {
            SDL_Window* handle;
            Window() : handle{::SDL_CreateWindow("Pendulum", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN)} {
                if (!handle)
                    throw std::runtime_error{::SDL_GetError()};
            }
            ~Window() {::SDL_DestroyWindow(handle);}
        } window; 
        const struct Renderer {
            SDL_Renderer* handle;
            Renderer(const Window& window) : handle{::SDL_CreateRenderer(window.handle, -1, SDL_RENDERER_ACCELERATED)} {
                if (!handle)
                    throw std::runtime_error{::SDL_GetError()};
            }
            ~Renderer() {::SDL_DestroyRenderer(handle);}
        } renderer{window};

        Scene scene;

        for (bool running = true; running;) {
            static constexpr int frames_per_second = 60;
            static constexpr int ms_per_frame = 1000 / frames_per_second;

            static Uint32 t0 = ::SDL_GetTicks();

            for (static SDL_Event event; ::SDL_PollEvent(&event);) {
                switch (event.type) {
                    case SDL_QUIT: running = false; break;
                }
            }

            const Uint32 t1 = ::SDL_GetTicks();
            const Uint32 dt = t1 - t0;

            static int t = 0;

            for (t += dt; t >= ms_per_frame; t -= dt)
                scene.update(dt);

            t0 = t1;

            ::SDL_SetRenderDrawColor(renderer.handle, 0, 0, 0, 0);
            ::SDL_RenderClear(renderer.handle);

            scene.render(renderer.handle, 400, 200, 7);

            ::SDL_RenderPresent(renderer.handle);
            ::SDL_Delay(1);
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
    }
    return 0;
}

