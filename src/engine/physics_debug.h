#pragma once

#include <cmath>
#include <vector>

#include <box2d/box2d.h>
#include <raylib.h>

/**
 * The debug draw context for rendering Box2D debug information.
 */
struct DebugDrawCtx
{
    float meters_to_pixels = 30.0f;
    float line_thickness = 1.0f;
};

/**
 * Convert a b2HexColor to a Raylib Color.
 *
 * @param c The b2HexColor to convert.
 * @param a The alpha value.
 * @return The converted Color.
 */
static Color ToRaylibColor(b2HexColor c, float a = 1.0f)
{
    unsigned int v = (unsigned int)c;
    unsigned char r = (v >> 16) & 0xFF;
    unsigned char g = (v >> 8) & 0xFF;
    unsigned char b = (v >> 0) & 0xFF;
    a = std::fmax(0.0f, std::fmin(1.0f, a)) * 255.0f;
    unsigned char alpha = static_cast<unsigned char>(a);
    return Color{r, g, b, alpha};
}

/**
 * Draw a line segment.
 *
 * @param p1 The start point.
 * @param p2 The end point.
 * @param color The color of the line.
 * @param context The debug draw context.
 */
static void DrawSegment(const b2Vec2 p1, const b2Vec2 p2, b2HexColor color, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;
    Vector2 a{p1.x * ctx->meters_to_pixels, p1.y * ctx->meters_to_pixels};
    Vector2 b{p2.x * ctx->meters_to_pixels, p2.y * ctx->meters_to_pixels};
    DrawLineEx(a, b, ctx->line_thickness, ToRaylibColor(color));
}

/**
 * Draw a polygon outline.
 *
 * @param v The vertices of the polygon.
 * @param count The number of vertices.
 * @param color The color of the polygon.
 * @param context The debug draw context.
 */
static void DrawPolygon(const b2Vec2* v, int count, b2HexColor color, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;
    Color c = ToRaylibColor(color);

    for (int i = 0; i < count; ++i)
    {
        b2Vec2 p0 = v[i];
        b2Vec2 p1 = v[(i + 1) % count];
        DrawLineEx({p0.x * ctx->meters_to_pixels, p0.y * ctx->meters_to_pixels},
                   {p1.x * ctx->meters_to_pixels, p1.y * ctx->meters_to_pixels},
                   ctx->line_thickness,
                   c);
    }
}

/**
 * Draw a filled polygon.
 *
 * @param xf The transform of the polygon.
 * @param v The vertices of the polygon.
 * @param count The number of vertices.
 * @param radius The radius for edge rounding (not used).
 * @param color The color of the polygon.
 * @param context The debug draw context.
 */
static void DrawSolidPolygon(b2Transform xf, const b2Vec2* v, int count, float radius, b2HexColor color, void* context)
{
    (void)radius;
    auto* ctx = (DebugDrawCtx*)context;

    // Fill color with some alpha so you can see overlap.
    Color fill = ToRaylibColor(color, 0.8);
    Color line = ToRaylibColor(color);

    // Transform to pixels.
    std::vector<Vector2> pts;
    pts.reserve((size_t)count);

    Vector2 center{0, 0};
    for (int i = 0; i < count; ++i)
    {
        b2Vec2 wp = b2TransformPoint(xf, v[i]);
        Vector2 p{wp.x * ctx->meters_to_pixels, wp.y * ctx->meters_to_pixels};
        pts.push_back(p);
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= count;
    center.y /= count;

    for (int i = 0; i < count - 1; i++)
    {
        DrawTriangle(pts[i], center, pts[i + 1], fill);
    }
    DrawTriangle(pts[count - 1], center, pts[0], fill);

    // Outline
    for (int i = 0; i < count; ++i)
    {
        const Vector2& a = pts[i];
        const Vector2& b = pts[(i + 1) % count];
        DrawLineEx(a, b, ctx->line_thickness, line);
    }
}

/**
 * Draw a circle outline.
 *
 * @param center The center of the circle.
 * @param radius The radius of the circle.
 * @param color The color of the circle.
 * @param context The debug draw context.
 */
static void DrawCircleOutline(b2Vec2 center, float radius, b2HexColor color, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;
    DrawCircleLines((int)(center.x * ctx->meters_to_pixels),
                    (int)(center.y * ctx->meters_to_pixels),
                    radius * ctx->meters_to_pixels,
                    ToRaylibColor(color));
}

/**
 * Draw a filled circle.
 *
 * @param xf The transform of the circle.
 * @param radius The radius of the circle.
 * @param color The color of the circle.
 * @param context The debug draw context.
 */
static void DrawSolidCircle(b2Transform xf, float radius, b2HexColor color, void* context)
{
    Color fill = ToRaylibColor(color, 0.8);
    Color line = ToRaylibColor(color);

    auto* ctx = (DebugDrawCtx*)context;
    b2Vec2 center = xf.p;
    DrawCircle((int)(center.x * ctx->meters_to_pixels),
               (int)(center.y * ctx->meters_to_pixels),
               radius * ctx->meters_to_pixels,
               fill);

    b2Vec2 line_end = b2TransformPoint(xf, {radius, 0});
    DrawLineEx({center.x * ctx->meters_to_pixels, center.y * ctx->meters_to_pixels},
               {line_end.x * ctx->meters_to_pixels, line_end.y * ctx->meters_to_pixels},
               ctx->line_thickness,
               line);
}

/**
 * Compute the length of a vector.
 *
 * @param v The vector.
 * @return The length of the vector.
 */
static float Len(Vector2 v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

/**
 * Normalize a vector.
 *
 * @param v The vector.
 * @return The normalized vector.
 */
static Vector2 Normalize(Vector2 v)
{
    float l = Len(v);
    if (l <= 1e-6f)
        return Vector2{0, 0};
    return Vector2{v.x / l, v.y / l};
}

/**
 * Compute the perpendicular vector.
 *
 * @param v The vector.
 * @return The perpendicular vector.
 */
static Vector2 Perp(Vector2 v)
{
    return Vector2{-v.y, v.x};
}

/**
 * Draw a filled capsule.
 *
 * @param p1 The first endpoint.
 * @param p2 The second endpoint.
 * @param radius_m The radius of the capsule.
 * @param color The color of the capsule.
 * @param context The debug draw context.
 */
static void DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius_m, b2HexColor color, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;

    Color fill = ToRaylibColor(color, 0.8);
    Color line = ToRaylibColor(color);

    Vector2 a = {p1.x * ctx->meters_to_pixels, p1.y * ctx->meters_to_pixels};
    Vector2 b = {p2.x * ctx->meters_to_pixels, p2.y * ctx->meters_to_pixels};

    float r = radius_m * ctx->meters_to_pixels;

    Vector2 ab = Vector2{b.x - a.x, b.y - a.y};

    Vector2 dir = Normalize(ab);
    Vector2 n = Perp(dir);
    Vector2 off = Vector2{n.x * r, n.y * r};

    // Rectangle corners that connect the tangent points of end circles
    Vector2 aL = Vector2{a.x + off.x, a.y + off.y};
    Vector2 aR = Vector2{a.x - off.x, a.y - off.y};
    Vector2 bL = Vector2{b.x + off.x, b.y + off.y};
    Vector2 bR = Vector2{b.x - off.x, b.y - off.y};

    // ---- Fill ----
    // Middle quad as two triangles
    DrawTriangle(aL, bL, bR, fill);
    DrawTriangle(aL, bR, aR, fill);

    // End caps as circles
    DrawCircleV(a, r, fill);
    DrawCircleV(b, r, fill);

    // ---- Outline ----
    // Side lines
    DrawLineEx(aL, bL, ctx->line_thickness, line);
    DrawLineEx(aR, bR, ctx->line_thickness, line);

    // Circle outlines (approx)
    DrawCircleLines((int)a.x, (int)a.y, r, line);
    DrawCircleLines((int)b.x, (int)b.y, r, line);

    // Optional: draw axis line (helps debug orientation)
    DrawLineEx(a, b, ctx->line_thickness, line);
}

/**
 * Draw a point.
 *
 * @param p The position of the point.
 * @param size The size of the point.
 * @param color The color of the point.
 * @param context The debug draw context.
 */
static void DrawPoint(b2Vec2 p, float size, b2HexColor color, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;
    DrawCircleV({p.x * ctx->meters_to_pixels, p.y * ctx->meters_to_pixels}, size, ToRaylibColor(color));
}

/**
 * Draw a transform.
 *
 * @param xf The transform to draw.
 * @param context The debug draw context.
 */
static void DrawTransform(b2Transform xf, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;

    // Draw axes: x axis in red, y axis in green
    b2Vec2 p = xf.p;
    b2Vec2 xAxis = b2RotateVector(xf.q, b2Vec2{1, 0});
    b2Vec2 yAxis = b2RotateVector(xf.q, b2Vec2{0, 1});

    float L = 0.5f; // meters
    DrawLineEx({p.x * ctx->meters_to_pixels, p.y * ctx->meters_to_pixels},
               {(p.x + L * xAxis.x) * ctx->meters_to_pixels, (p.y + L * xAxis.y) * ctx->meters_to_pixels},
               ctx->line_thickness,
               RED);
    DrawLineEx({p.x * ctx->meters_to_pixels, p.y * ctx->meters_to_pixels},
               {(p.x + L * yAxis.x) * ctx->meters_to_pixels, (p.y + L * yAxis.y) * ctx->meters_to_pixels},
               ctx->line_thickness,
               GREEN);
}

/**
 * Draw a string.
 *
 * @param p The position to draw the string.
 * @param s The string to draw.
 * @param color The color of the string.
 * @param context The debug draw context.
 */
static void DrawString(b2Vec2 p, const char* s, b2HexColor color, void* context)
{
    auto* ctx = (DebugDrawCtx*)context;
    DrawText(s, (int)(p.x * ctx->meters_to_pixels), (int)(p.y * ctx->meters_to_pixels), 10, ToRaylibColor(color));
}

/**
 * The physics debug renderer.
 */
class PhysicsDebugRenderer
{
public:
    DebugDrawCtx ctx;
    b2DebugDraw dd{};

    /**
     * Initialize the debug renderer.
     *
     * @param meters_to_pixels The scale factor from meters to pixels.
     * @param line_thickness The thickness of lines.
     */
    void init(float meters_to_pixels = 30.0f, float line_thickness = 1.0f)
    {
        ctx.meters_to_pixels = meters_to_pixels;
        ctx.line_thickness = line_thickness;

        // Callbacks
        dd.DrawPolygonFcn = &DrawPolygon;
        dd.DrawSolidPolygonFcn = &DrawSolidPolygon;
        dd.DrawCircleFcn = &DrawCircleOutline;
        dd.DrawSolidCircleFcn = &DrawSolidCircle;
        dd.DrawSolidCapsuleFcn = &DrawSolidCapsule;
        dd.DrawSegmentFcn = &DrawSegment;
        dd.DrawTransformFcn = &DrawTransform;
        dd.DrawPointFcn = &DrawPoint;
        dd.DrawStringFcn = &DrawString;

        // Options
        dd.useDrawingBounds = false;
        dd.drawShapes = true;
        dd.drawJoints = false;
        dd.drawBounds = false;
        dd.drawMass = false;
        dd.drawBodyNames = false;
        dd.drawContacts = false;
        dd.drawGraphColors = false;
        dd.drawContactNormals = true;
        dd.drawContactImpulses = false;
        dd.drawContactFeatures = true;
        dd.drawFrictionImpulses = false;
        dd.drawIslands = true;

        dd.context = &ctx;
    }

    /**
     * Draw the debug information for the given world.
     *
     * @param world The Box2D world.
     */
    void draw_debug(b2WorldId world)
    {
        b2World_Draw(world, &dd);
    }
};
