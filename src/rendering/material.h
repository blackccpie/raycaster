#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "onb.h"
#include "texture.h"
#include "rtweekend.h"

struct hit_record;

class material {
    public:
        virtual color emitted(const hit_record& rec) const {
            return color(0,0,0);
        }
        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& albedo_out, ray& scattered, double& pdf
        ) const {
            return false;
        }

        virtual double scattering_pdf(
            const ray& r_in, const hit_record& rec, const ray& scattered
        ) const {
            return 0;
        }
};

class lambertian final : public material {
    public:
        explicit lambertian(const color& a) : albedo(std::make_shared<solid_color>(a)) {}
        explicit lambertian(std::shared_ptr<texture> a) : albedo(a) {}

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& albedo_out, ray& scattered, double& pdf
        ) const override {
            onb uvw;
            uvw.build_from_w(rec.normal);
            auto direction = uvw.local(random_cosine_direction());
            scattered = ray(rec.p, unit_vector(direction), r_in.time());
            albedo_out = albedo->value(rec.u, rec.v, rec.p);
            pdf = dot(uvw.w(), scattered.direction()) / pi;
            return true;
        }
        virtual double scattering_pdf(
            const ray& r_in, const hit_record& rec, const ray& scattered
        ) const override {
            auto cosine = dot(rec.normal, unit_vector(scattered.direction()));
            return cosine < 0 ? 0 : cosine/pi;
        }

    public:
        std::shared_ptr<texture> albedo;
};

class metal final : public material {
    public:
        metal(const color& a, double f) : albedo(a), fuzz(f < 1. ? f : 1.) {}

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& albedo_out, ray& scattered, double& pdf
        ) const override {
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere(), r_in.time());
            albedo_out = albedo;
            return (dot(scattered.direction(), rec.normal) > 0);
        }

    public:
        color albedo;
        double fuzz;
};

class dielectric final : public material {
    public:
        explicit dielectric(double index_of_refraction) : ir(index_of_refraction) {}

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& albedo_out, ray& scattered, double& pdf
        ) const override {
            albedo_out = color(1.0, 1.0, 1.0);
            double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

            vec3 unit_direction = unit_vector(r_in.direction());
            double cos_theta = std::min(dot(-unit_direction, rec.normal), 1.0);
            double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

            bool cannot_refract = refraction_ratio * sin_theta > 1.0;
            vec3 direction;

            if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
                direction = reflect(unit_direction, rec.normal);
            else
                direction = refract(unit_direction, rec.normal, refraction_ratio);

            scattered = ray(rec.p, direction, r_in.time());
            return true;
        }

    private:
        double ir; // Index of Refraction
    
    private:
        static double reflectance(double cosine, double ref_idx) {
            // Use Schlick's approximation for reflectance.
            auto r0 = (1-ref_idx) / (1+ref_idx);
            r0 = r0*r0;
            return r0 + (1-r0)*pow((1 - cosine),5);
        }
};

class diffuse_light final : public material  {
    public:
        explicit diffuse_light(std::shared_ptr<texture> a) : emit(a) {}
        explicit diffuse_light(color c) : emit(std::make_shared<solid_color>(c)) {}

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& albedo_out, ray& scattered, double& pdf
        ) const override {
            return false;
        }

        virtual color emitted(const hit_record& rec) const override {
            if (rec.front_face)
                return emit->value(rec.u, rec.v, rec.p);
            else
                return color(0,0,0);
        }

    public:
        std::shared_ptr<texture> emit;
};

class isotropic final : public material {
    public:
        explicit isotropic(color c) : albedo(std::make_shared<solid_color>(c)) {}
        explicit isotropic(std::shared_ptr<texture> a) : albedo(a) {}

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& albedo_out, ray& scattered, double& pdf
        ) const override {
            scattered = ray(rec.p, random_in_unit_sphere(), r_in.time());
            albedo_out = albedo->value(rec.u, rec.v, rec.p);
            return true;
        }

    public:
        std::shared_ptr<texture> albedo;
};

#endif
