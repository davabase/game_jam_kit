#pragma once

#include <algorithm>
#include <vector>

#include <box2d/box2d.h>
#include "math_extensions.h"

/**
 * The raycast result struct with information about a raycast.
 */
struct RayHit {
    bool hit = false;
    b2BodyId body = b2_nullBodyId;
    float fraction = 1.0f;
    float distance = 0.0f;
    b2Vec2 point = {0, 0};
    b2Vec2 normal = {0, 0};
};

/**
 * The raycast context struct used for passing data during a raycast call.
 * For internal use only.
 */
struct RayContextClosest {
    RayHit closest;
    b2BodyId ignore_body = b2_nullBodyId;
    b2Vec2 translation;
};

/**
 * Raycast callback for finding the closest hit.
 * For internal use only.
 *
 * @param shape_id The ID of the shape that was hit.
 * @param point The point where the ray hit the shape.
 * @param normal The normal of the shape at the hit point.
 * @param fraction The fraction of the ray length where the hit occurred.
 * @param context The context for the raycast.
 *
 * @return The fraction of the ray length to continue searching.
 */
static float raycast_closest_callback(b2ShapeId shape_id, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
    auto* ctx = static_cast<RayContextClosest*>(context);
    // Check for the ignored body.
    b2BodyId hit_body = b2Shape_GetBody(shape_id);
    if (b2Body_IsValid(ctx->ignore_body) && b2Body_IsValid(hit_body) && hit_body.index1 == ctx->ignore_body.index1) {
        // Continue searching.
        return 1.0f;
    }

    if (fraction < ctx->closest.fraction) {
        ctx->closest.hit = true;
        ctx->closest.fraction = fraction;
        ctx->closest.distance = b2Length(ctx->translation) * fraction;
        ctx->closest.point = point;
        ctx->closest.normal = normal;
        ctx->closest.body = hit_body;
    }
    return fraction;
}

/**
 * Cast a ray and return the closest hit.
 *
 * @param world The world to cast the ray in.
 * @param ignore_body The body to ignore.
 * @param origin The starting point of the ray.
 * @param translation The length of the ray.
 *
 * @return Information about the closest hit.
 */
RayHit raycast_closest(b2WorldId world, b2BodyId ignore_body, b2Vec2 origin, b2Vec2 translation) {
    RayContextClosest ctx;
    ctx.ignore_body = ignore_body;
    ctx.translation = translation;

    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_CastRay(world, origin, translation, filter, raycast_closest_callback, &ctx);

    return ctx.closest;
}

/**
 * The raycast context struct used for passing data during a raycast call.
 * For internal use only.
 */
struct RayContextAll {
    std::vector<RayHit> all;
    b2BodyId ignore_body = b2_nullBodyId;
    b2Vec2 translation;
};

/**
 * Raycast callback for finding the closest hit.
 * For internal use only.
 *
 * @param shape_id The ID of the shape that was hit.
 * @param point The point where the ray hit the shape.
 * @param normal The normal of the shape at the hit point.
 * @param fraction The fraction of the ray length where the hit occurred.
 * @param context The context for the raycast.
 *
 * @return The fraction of the ray length to continue searching.
 */
static float raycast_all_callback(b2ShapeId shape_id, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
    auto* ctx = static_cast<RayContextAll*>(context);
    // Check for the ignored body.
    b2BodyId hit_body = b2Shape_GetBody(shape_id);
    if (b2Body_IsValid(ctx->ignore_body) && b2Body_IsValid(hit_body) && hit_body.index1 == ctx->ignore_body.index1) {
        // Continue searching.
        return 1.0f;
    }

    RayHit hit = {true, hit_body, fraction, b2Length(ctx->translation) * fraction, point, normal};
    ctx->all.push_back(hit);
    return fraction;
}

/**
 * Cast a ray and return all hit bodies on the ray.
 *
 * @param world The world to cast the ray in.
 * @param ignore_body The body to ignore.
 * @param origin The starting point of the ray.
 * @param translation The length of the ray.
 *
 * @return Information about the hit bodies.
 */
std::vector<RayHit> raycast_all(b2WorldId world, b2BodyId ignore_body, b2Vec2 origin, b2Vec2 translation) {
    RayContextAll ctx;
    ctx.ignore_body = ignore_body;
    ctx.translation = translation;

    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_CastRay(world, origin, translation, filter, raycast_all_callback, &ctx);

    return ctx.all;
}

/**
 * The shape hit context struct used for passing data during a shape overlap call.
 * For internal use only.
 */
struct ShapeHitContext {
    b2BodyId ignore_body = b2_nullBodyId;
    std::vector<b2BodyId> hits;
};

/**
 * Shape hit callback for finding shape overlaps.
 * For internal use only.
 *
 * @param shape_id The ID of the shape that was hit.
 * @param point The point where the ray hit the shape.
 * @param normal The normal of the shape at the hit point.
 * @param fraction The fraction of the ray length where the hit occurred.
 * @param context The context for the raycast.
 *
 * @return The fraction of the ray length to continue searching.
 */
static bool shape_hit_callback(b2ShapeId shape_id, void* context) {
    auto* ctx = static_cast<ShapeHitContext*>(context);
    // Check for the ignored body.
    b2BodyId hit_body = b2Shape_GetBody(shape_id);
    if (b2Body_IsValid(ctx->ignore_body) && b2Body_IsValid(hit_body) && hit_body.index1 == ctx->ignore_body.index1) {
        // Continue searching.
        return true;
    }

    ctx->hits.push_back(hit_body);
    return true;
}

/**
 * Cast a ray and return the closest hit.
 *
 * @param world The world to cast the ray in.
 * @param ignore_body The body to ignore.
 * @param origin The starting point of the ray.
 * @param translation The length of the ray.
 *
 * @return Information about the closest hit.
 */
std::vector<b2BodyId> shape_hit(b2WorldId world, b2BodyId ignore_body, b2ShapeProxy proxy) {
    ShapeHitContext ctx;
    ctx.ignore_body = ignore_body;

    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_OverlapShape(world, &proxy, filter, shape_hit_callback, &ctx);

    // Remove duplicate bodies.
    std::sort(ctx.hits.begin(), ctx.hits.end());
    ctx.hits.erase( std::unique(ctx.hits.begin(), ctx.hits.end() ), ctx.hits.end());

    return ctx.hits;
}

std::vector<b2BodyId> circle_hit(b2WorldId world, b2BodyId ignore_body, b2Vec2 center, float radius) {
    b2Circle circle = {center, radius};
    b2ShapeProxy proxy = b2MakeProxy(&circle.center, 1, circle.radius);
    return shape_hit(world, ignore_body, proxy);
}

std::vector<b2BodyId> rectangle_hit(b2WorldId world, b2BodyId ignore_body, b2Vec2 center, b2Vec2 size, float rotation = 0.0f) {
    // 4 corners in local space
    b2Vec2 half = {size.x / 2.0f, size.y / 2.0f};
    b2Vec2 local[4] = {
        {-half.x, -half.y},
        {half.x, -half.y},
        {half.x,  half.y},
        {-half.x,  half.y},
    };

    // transform corners to world space
    auto rot = b2MakeRot(rotation * DEG2RAD);
    b2Transform transform = {center, rot};
    b2Vec2 pts[4];
    for (int i = 0; i < 4; ++i) {
        pts[i] = b2TransformPoint(transform, local[i]);
    }

    b2ShapeProxy proxy = b2MakeProxy(pts, 4, 0);
    return shape_hit(world, ignore_body, proxy);
}
