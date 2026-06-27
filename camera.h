#ifndef CAMERA_H
#define CAMERA_H

#include <thread>
#include <random>

#include "hittable.h"
#include "material.h"

class camera {
    public:
        double aspect_ratio = 1.0;  // ratio of image width over height
        int image_width = 100;      // rendered image width in pixel count
        int samples_per_pixel = 10; // count of random samples for each pixel
        int max_depth = 10;         // maximum number of ray bounces into scene

        double vfov = 90; // vertical view angle (field of view)
        point3 lookfrom = point3(0,0,0);
        point3 lookat = point3(0,0,-1);
        vec3 vup = vec3(0,1,0);

        double defocus_angle = 0; // variation angles of rays through each pixel
        double focus_dist = 10; // distance from camera lookfrom point to plane of perfect focus

        void render(const hittable& world) {
            initialize();

            std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

            std::vector<color> framebuffer(image_width * image_height);

            int num_threads = std::thread::hardware_concurrency();
            int remainder = image_height % num_threads;

            std::vector<std::thread> threads;

            for (int t = 0; t < num_threads; t++) {
                int thread_rows = (image_height / num_threads) + (t < remainder ? 1 : 0);
                int start_row = t * (image_height / num_threads) + std::min(t, remainder);
                int end_row = start_row + thread_rows;

                threads.emplace_back([this, start_row, end_row, &framebuffer, &world]{
                    std::mt19937 gen(std::random_device{}());
                    for (int j = start_row; j < end_row; j++) {
                        for (int i = 0; i < image_width; i++) {
                            color pixel_color(0,0,0);
                            for (int sample = 0; sample < samples_per_pixel; sample++) {
                                ray r = get_ray(gen, i, j);
                                pixel_color += ray_color(gen, r, max_depth, world);
                            }
                            framebuffer[j * image_width + i] = pixel_samples_scale * pixel_color;
                        }
                    }
                });
                
            }

            for (auto &thread : threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            for (int j = 0; j < image_height; j++) {
                for (int i = 0; i < image_width; i++) {
                    color pixel_color = framebuffer[j * image_width + i];
                    write_color(std::cout, pixel_color);
                }
            }

            std::clog << "\rDone.                  \n";
        }

    private:
        int image_height;           // rendered image height
        double pixel_samples_scale; // color scale factor for a sum of pixel samples
        point3 center;              // camera center
        point3 pixel00_loc;         // location of pixel 0, 0
        vec3 pixel_delta_u;         // offset to pixel to the right
        vec3 pixel_delta_v;         // offset to pixel below
        vec3 u, v, w;
        vec3 defocus_disk_u;        // defocus disk horizontal radius
        vec3 defocus_disk_v;        // defocus disk vertical radius

        void initialize() {
            image_height = int(image_width / aspect_ratio);
            image_height = (image_height < 1) ? 1 : image_height;

            pixel_samples_scale = 1.0 / samples_per_pixel;

            center = lookfrom;

            // Determine viewport dimensions.
            auto theta = degrees_to_radians(vfov);
            auto h = std::tan(theta/2);
            auto viewport_height = 2 * h * focus_dist;
            auto viewport_width = viewport_height * (double(image_width)/image_height);

            // calculate the u,v,w unit basis vectors for the camera coordinate frame
            w = unit_vector(lookfrom - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w, u);

            // Calculate the vectors across the horizontal and down the vertical viewport edges.
            vec3 viewport_u = viewport_width * u; // vector across viewport horizontal edge
            vec3 viewport_v = viewport_height * -v; // vector down viewport vertical edge

            // Calculate the horizontal and vertical delta vectors from pixel to pixel.
            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            // Calculate the location of the upper left pixel.
            auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
            pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

            // calculate the camera defocus fisk basis vectors
            auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
            defocus_disk_u = u * defocus_radius;
            defocus_disk_v = v * defocus_radius;
        }

        ray get_ray(int i, int j) const {
            // construct a camera ray originating from the defocus disk and directed at
            // sampled point around the pixel location i, j.
            
            auto offset = sample_square();

            auto pixel_sample = pixel00_loc
                              + ((i + offset.x()) * pixel_delta_u)
                              + ((j + offset.y()) * pixel_delta_v);

            auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
            auto ray_direction = pixel_sample - ray_origin;

            return ray(ray_origin, ray_direction);
        }

        ray get_ray(std::mt19937& rng, int i, int j) const {
            auto offset = sample_square(rng);

            auto pixel_sample = pixel00_loc
                              + ((i + offset.x()) * pixel_delta_u)
                              + ((j + offset.y()) * pixel_delta_v);

            auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample(rng);
            auto ray_direction = pixel_sample - ray_origin;

            return ray(ray_origin, ray_direction);
        }

        vec3 sample_square() const {
            // returns the rector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
            return vec3(random_double() - 0.5, random_double() - 0.5, 0); 
        }

        vec3 sample_square(std::mt19937& rng) const {
            return vec3(random_double(rng) - 0.5, random_double(rng) - 0.5, 0); 
        }

        point3 defocus_disk_sample() const {
            // returns a random point in the camera defocus disk
            auto p = random_in_unit_disk();
            return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
        }

        point3 defocus_disk_sample(std::mt19937& rng) const {
            auto p = random_in_unit_disk(rng);
            return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
        }

        color ray_color(const ray& r, int depth, const hittable& world) const {
            // if we've exeeded the ray bounce limit, no more light is gathered
            if (depth <= 0)
                return color(0,0,0);

            hit_record rec;
            if (world.hit(r, interval(0.001, infinity), rec)) {
                ray scattered;
                color attenuation;
                if (rec.mat->scatter(r, rec, attenuation, scattered))
                    return attenuation * ray_color(scattered, depth-1, world);
                return color(0,0,0);
            }
               

            vec3 unit_direction = unit_vector(r.direction());
            auto a = 0.5*(unit_direction.y() + 1.0);
            return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
        }

        color ray_color(std::mt19937& rng, const ray& r, int depth, const hittable& world) const {
            if (depth <= 0)
                return color(0,0,0);

            hit_record rec;
            if (world.hit(r, interval(0.001, infinity), rec)) {
                ray scattered;
                color attenuation;
                if (rec.mat->scatter(rng, r, rec, attenuation, scattered))
                    return attenuation * ray_color(rng, scattered, depth-1, world);
                return color(0,0,0);
            }
               

            vec3 unit_direction = unit_vector(r.direction());
            auto a = 0.5*(unit_direction.y() + 1.0);
            return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
        }
};

#endif