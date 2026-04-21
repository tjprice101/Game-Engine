#pragma once
#include "ecs/ECS.h"
#include "ecs/Components.h"

// ---- AnimationSystem ---------------------------------------------------------
// Advances CAnimation timers, updates CSprite UVs accordingly.
// Call update() once per frame. Reads CAnimation + writes CSprite.

class AnimationSystem {
public:
    // Advance all animations in the entity manager
    static void update(EntityManager& em, float dt);

    // ---- Clip helpers -------------------------------------------------------
    // Add a standard humanoid clip set to a CAnimation component.
    // sheetCols / frameW / frameH must already be set on the component.
    static void addIdleClip  (CAnimation& anim, int startFrame, int frames = 4,  float fps = 8.f);
    static void addWalkClip  (CAnimation& anim, int startFrame, int frames = 8,  float fps = 12.f);
    static void addRunClip   (CAnimation& anim, int startFrame, int frames = 8,  float fps = 18.f);
    static void addJumpClip  (CAnimation& anim, int startFrame, int frames = 4,  float fps = 10.f);
    static void addFallClip  (CAnimation& anim, int startFrame, int frames = 2,  float fps = 8.f);
    static void addAttackClip(CAnimation& anim, int startFrame, int frames = 6,  float fps = 20.f);
    static void addDeathClip (CAnimation& anim, int startFrame, int frames = 8,  float fps = 10.f);
    static void addHurtClip  (CAnimation& anim, int startFrame, int frames = 3,  float fps = 15.f);

private:
    // Compute the UV rect for a given frame index in a spritesheet
    // frameIndex is 0-based across the whole sheet (row-major)
    static void applyFrame(CSprite& sprite, const CAnimation& anim, int frameIndex);
};
