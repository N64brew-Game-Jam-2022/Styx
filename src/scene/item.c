#include "item.h"

#include "../util/time.h"
#include "../util/memory.h"

#include "../build/assets/materials/static.h"
#include "../build/assets/models/pumpkin.h"
#include "../build/assets/models/conveyor.h"
#include "../defs.h"

#define MAX_SNAP_SPEED      4.0f

struct ItemTypeDefinition gItemDefinitions[ItemTypeCount] = {
    [ItemTypePumpkin] = {
        pumpkin_model_gfx,
        ITEMS_INDEX,
        0,
        0,
        NULL,
        NULL,
    },
};

void itemInit(struct Item* item, enum ItemType itemType, struct Transform* initialPose) {
    item->next = NULL;
    item->type = itemType;
    item->transform = *initialPose;
    item->target = *initialPose;
    item->flags = ITEM_FLAGS_ATTACHED;
    
    struct ItemTypeDefinition* definition = &gItemDefinitions[itemType];

    if (definition->boneCount) {
        item->flags |= ITEM_FLAGS_HAS_ARMATURE;
        
        skArmatureInit(
            &item->armature, 
            definition->dl, 
            definition->boneCount, 
            definition->defaultBones, 
            definition->boneParent, 
            definition->attachmentCount
        );

        skAnimatorInit(&item->animator, definition->boneCount, NULL, NULL);
    }
}

void itemUpdate(struct Item* item) {
    if (item->flags & ITEM_FLAGS_HAS_ARMATURE) {
        skAnimatorUpdate(&item->animator, item->armature.boneTransforms, 1.0f);
    }

    if (item->flags & ITEM_FLAGS_ATTACHED) {
        item->transform = item->target;
    } else {
        if (vector3MoveTowards(&item->transform.position, &item->target.position, MAX_SNAP_SPEED * FIXED_DELTA_TIME, &item->transform.position)) {
            item->transform.rotation = item->target.rotation;
            item->flags |= ITEM_FLAGS_ATTACHED;
        } else {
            quatLerp(&item->transform.rotation, &item->target.rotation, 0.1f, &item->transform.rotation);
        }
    }
}

void itemRender(struct Item* item, struct RenderScene* renderScene) {
    struct ItemTypeDefinition* definition = &gItemDefinitions[item->type];

    Mtx* mtx = renderStateRequestMatrices(renderScene->renderState, 1);

    struct Transform transform;
    transform.position = gZeroVec;
    quatIdent(&transform.rotation);
    transform.scale = gOneVec;

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    Mtx* armature = NULL;

    if (definition->boneCount) {
        armature = renderStateRequestMatrices(renderScene->renderState, definition->boneCount);
        skCalculateTransforms(&item->armature, armature);
    }

    renderSceneAdd(renderScene, definition->dl, mtx, definition->materialIndex, &item->transform.position, armature);
}

void itemUpdateTarget(struct Item* item, struct Transform* transform) {
    item->target = *transform;
}

void itemMarkNewTarget(struct Item* item) {
    item->flags &= ~ITEM_FLAGS_ATTACHED;
}

void itemPoolInit(struct ItemPool* itemPool) {
    itemPool->itemHead = NULL;
    itemPool->unusedHead = NULL;
    itemPool->itemCount = 0;
}

struct Item* itemPoolNew(struct ItemPool* itemPool, enum ItemType itemType, struct Transform* initialPose) {
    struct Item* result = NULL;

    if (itemPool->unusedHead) {
        result = itemPool->unusedHead;
        itemPool->unusedHead = itemPool->unusedHead->next;
    } else {
        result = malloc(sizeof(struct Item));
    }

    result->next = itemPool->itemHead;
    itemPool->itemHead = result;

    ++itemPool->itemCount;

    return result;
}

void itemPoolFree(struct ItemPool* itemPool, struct Item* item) {
    struct Item* prev = NULL;
    struct Item* current = itemPool->itemHead;

    while (current != NULL && current != item) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        return;
    }

    if (prev) {
        prev->next = current->next;
    } else {
        itemPool->itemHead = current->next;
    }

    item->next = itemPool->unusedHead;
    itemPool->unusedHead = item;

    --itemPool->itemCount;
}

void itemPoolUpdate(struct ItemPool* itemPool) {
    struct Item* current = itemPool->itemHead;

    while (current != NULL) {
        itemUpdate(current);

        current = current->next;
    }
}

void itemPoolRender(struct ItemPool* itemPool, struct SpotLight* spotLights, int spotLightCount, struct RenderScene* renderScene) {
    struct Item* current = itemPool->itemHead;

    struct LightConfiguration* configurations = stackMalloc(sizeof(struct LightConfiguration) * itemPool->itemCount);
    struct LightConfiguration* currentConfiguration = configurations;

    while (current != NULL) {
        spotLightsFindConfiguration(spotLights, spotLightCount, &current->transform.position, NULL, currentConfiguration);
        spotLightsSetupLight(currentConfiguration, &current->transform.position, renderScene->renderState);
        itemRender(current, renderScene);

        current = current->next;
        ++currentConfiguration;
    }

    stackMallocFree(configurations);
}