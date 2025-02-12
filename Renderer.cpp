//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#ifdef _OPENMP
    #include <omp.h>
#endif


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);

    std::cout << "SPP: " << scene.spp << "\n";
    std::cout << "RussianRoulette: " << scene.RussianRoulette << "\n";

    float progress = 0.0f;

    // [[0.25, 0.25], [0.75, 0.25], [0.25, 0.75], [0.75, 0.75]]
    std::vector<Vector2f> samples_poses = {
        Vector2f(0.25, 0.25),
        Vector2f(0.75, 0.25),
        Vector2f(0.25, 0.75),
        Vector2f(0.75, 0.75)
    };
    int area_size = (int)samples_poses.size();
    bool enable_super_sampling = false;

    #pragma omp parallel for num_threads(8) // use multi-threading for speedup if openmp is available
    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {

            int m = i + j * scene.width;
            // generate primary ray direction

            //TODO: modify this function to support anti-aliasing (Mutation-supersampling)
            if(enable_super_sampling){
                // iterate over 4 sample-areas per pixel
                Vector3f framebuffer_area[area_size];
                int scene_spp_per_area = scene.spp / area_size;
                for (int k_area = 0; k_area < area_size; k_area++){
                    float x = (2 * (i + samples_poses[k_area].x) / (float)scene.width - 1) *
                            imageAspectRatio * scale;
                    float y = (1 - 2 * (j + samples_poses[k_area].y) / (float)scene.height) * scale;
                    Vector3f dir = normalize(Vector3f(-x, y, 1));

                    for (int k = 0; k < scene_spp_per_area; k++){ // multiple samples per pixel
                        Vector3f tempresult = scene.castRay(Ray(eye_pos, dir), 0) / scene_spp_per_area;
                        framebuffer[m] += tempresult;  
                        framebuffer_area[k_area] += tempresult;
                    }
                }

                framebuffer[m] = framebuffer[m] / area_size;
                // for (int k_area = 0; k_area < area_size; k_area++){
                //     framebuffer[m] = Vector3f::Min(framebuffer_area[k_area], framebuffer[m]);
                // }
            }else{
                float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                      imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));

                for (int k = 0; k < scene.spp; k++){ // multiple samples per pixel
                    framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / scene.spp;  
                }
            }
        }
        progress += 1.0f / (float)scene.height;
        UpdateProgress(progress);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    // new file name, scene.spp-scene.RussianRoulette-binary.ppm
    std::string filename =  "binary-" + std::to_string(scene.spp) + "-" + std::to_string((int)(scene.RussianRoulette * 100.0f)) + ".ppm";
    FILE* fp = fopen(filename.c_str(), "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
