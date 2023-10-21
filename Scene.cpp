//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    // printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection inter = Scene::intersect(ray);
    if (!inter.happened) {
        return Vector3f(0.0f);
    }
    if (inter.m->hasEmission()) {
        return inter.m->getEmission();
    }

    if(depth > Scene::maxDepth){
        return Scene::backgroundColor;
    }

    if(inter.m->getType() == GLASS){
        Vector3f reflect_dir = reflect(ray.direction, inter.normal).normalized();
        Vector3f refract_dir = refract(ray.direction, inter.normal, inter.m->ior).normalized();

        Ray reflect_ray = Ray(inter.coords, reflect_dir);
        Ray refract_ray = Ray(inter.coords, refract_dir);

        float kr;
        fresnel(ray.direction, inter.normal, inter.m->ior, kr);

        return castRay(reflect_ray, depth + 1) * kr + castRay(refract_ray, depth + 1) * (1 - kr);
    }


    Vector3f L_dir = Vector3f(0.0f);
    Vector3f L_indir = Vector3f(0.0f);

    Intersection light_Pos;
    float light_Pdf;
    Scene::sampleLight(light_Pos, light_Pdf);

    // Sample Direct Light
    Vector3f light_direction = (light_Pos.coords - inter.coords).normalized();
    float light_distance = (light_Pos.coords - inter.coords).norm();
    Ray light_Ray = Ray(inter.coords, light_direction);
    Intersection light_inter = Scene::intersect(light_Ray);

    if(light_inter.happened && light_inter.m->hasEmission()){
        Vector3f fr = inter.m->eval(ray.direction, light_direction, inter.normal);
        L_dir = light_inter.m->getEmission() * fr * dotProduct(light_direction, inter.normal) / 
            (light_distance * light_distance) / light_Pdf;
    }

    // Sample Indirect Light
    if (get_random_float() < Scene::RussianRoulette) {
        Vector3f out_dir = inter.m->sample(ray.direction, inter.normal).normalized();
        Ray out_Ray = Ray(inter.coords, out_dir);
        Intersection out_inter = Scene::intersect(out_Ray);
        if (out_inter.happened && !out_inter.m->hasEmission()) {
            Vector3f fr = inter.m->eval(ray.direction, out_dir, inter.normal);
            float pdf = inter.m->pdf(ray.direction, out_dir, inter.normal);

            L_indir = castRay(out_Ray, depth + 1) * fr * dotProduct(out_dir, inter.normal) / 
                pdf / Scene::RussianRoulette;
        }
    }
    return L_dir + L_indir;
}