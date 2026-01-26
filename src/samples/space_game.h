#pragma once

#include "engine/prefabs/includes.h"

class Ship : public GameObject
{
public:
    BodyComponent* body;
    SpriteComponent* sprite;
    PhysicsService* physics;

    void init() override
    {
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_dynamicBody;
                body_def.position = physics->convert_to_meters(Vector2{400.0f, 300.0f});
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();
                body_material.restitution = 0.1f;
                body_material.friction = 0.5f;

                b2ShapeDef polygon_shape_def = b2DefaultShapeDef();
                polygon_shape_def.density = 1.0f;
                polygon_shape_def.material = body_material;

                b2Vec2 vertices[3];
                vertices[0] = physics->convert_to_meters(Vector2{0.0f, -20.0f});
                vertices[1] = physics->convert_to_meters(Vector2{15.0f, 10.0f});
                vertices[2] = physics->convert_to_meters(Vector2{-15.0f, 10.0f});

                b2Polygon polygon_shape;
                polygon_shape.vertexCount = 3;
                for (int i = 0; i < 3; i++)
                {
                    polygon_shape.vertices[i] = vertices[i];
                }
                b2CreatePolygonShape(b.id, &polygon_shape_def, &polygon_shape);
            });

        sprite = add_component<SpriteComponent>("assets/space_game/ship.png", body);
    }
};

class SpaceScene : public Scene
{
public:
    void init_services() override
    {
        add_service<PhysicsService>();
        add_service<SoundService>();
    }
};