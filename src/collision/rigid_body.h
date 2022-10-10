#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "../math/basis.h"
#include "../math/transform.h"

#define RIGID_BODY_NO_ROOM  0xFFFF

#define MAX_PORTAL_SPEED (1000.0f / 64.0f)

enum RigidBodyFlags {
    RigidBodyIsKinematic = (1 << 10),
    RigidBodyIsSleeping = (1 << 11),
};

struct RigidBody {
    struct Transform transform;
    struct Vector3 velocity;
    struct Vector3 angularVelocity;

    float mass;
    float massInv;
    // most objects are going to be spheres
    // and cubes. Any other object will just
    // have some inaccurate physics 
    // for cube m * side * side / 6
    // for sphere 2/5 * m * r * r
    float momentOfInertia;
    float momentOfInertiaInv;
    
    enum RigidBodyFlags flags;
};

void rigidBodyInit(struct RigidBody* rigidBody, float mass, float momentOfIniteria);
void rigidBodyMarkKinematic(struct RigidBody* rigidBody);
void rigidBodyAppyImpulse(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* impulse);
void rigidBodyUpdate(struct RigidBody* rigidBody);
void rigidBodyVelocityAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* worldVelocity);
void rigidBodyVelocityAtWorldPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* worldVelocity);
void rigidBodyTeleport(struct RigidBody* rigidBody, struct Transform* from, struct Transform* to, int toRoom);

int rigidBodyCheckPortals(struct RigidBody* rigidBody);


#endif