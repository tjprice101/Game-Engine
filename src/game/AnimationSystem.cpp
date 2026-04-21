#include "AnimationSystem.h"

void AnimationSystem::update(EntityManager& em, float dt) {
    em.view<CAnimation, CSprite>([&](EntityID /*e*/, CAnimation& anim, CSprite& sprite) {
        if (anim.paused) return;
        const AnimClip* clip = anim.current();
        if (!clip || clip->frameCount == 0) return;

        anim.timer += dt;
        float frameDuration = (clip->fps > 0.f) ? 1.f / clip->fps : 0.1f;

        if (anim.timer >= frameDuration) {
            anim.timer -= frameDuration;
            ++anim.frame;

            if (anim.frame >= clip->frameCount) {
                if (clip->loop) {
                    anim.frame = 0;
                    if (anim.onFinish) anim.onFinish(clip->name);
                } else {
                    anim.frame = clip->frameCount - 1;
                    anim.paused = true;
                    if (anim.onFinish) anim.onFinish(clip->name);
                }
            }
        }

        // Map frame within clip to global sheet frame index
        int sheetFrame = clip->frameStart + anim.frame;
        applyFrame(sprite, anim, sheetFrame);
    });
}

void AnimationSystem::applyFrame(CSprite& sprite, const CAnimation& anim, int sheetFrame) {
    if (anim.sheetCols <= 0 || anim.frameW <= 0 || anim.frameH <= 0) return;

    // Assuming the sprite texture has fixed sheet dimensions; UVs are normalised [0..1].
    // We need the sheet total width/height to compute UVs. Since we only have frame pixel
    // size, we derive from sprite's current uvMax extents — but the cleanest approach is
    // to store total sheet size. We'll use frameW/frameH as the *atlas* cell size and
    // assume uvMax was set to (1,1) for the full sheet.

    // Use a fixed atlas texture size baked into the system (512×512 default).
    // Games should set their atlas size via setAtlasSize() or embed it per animation.
    // For now: UV per frame = (col/sheetCols, row/sheetRows)
    int col = sheetFrame % anim.sheetCols;
    int row = sheetFrame / anim.sheetCols;

    // We don't know sheetRows here, but we can compute UVs if we know the texture pixel
    // dimensions. We use a per-sprite approach: uvMin + step.
    // Since frameW/frameH are in pixels and uvMax is normalised, we compute assuming
    // the texture pixel dimensions = sheetCols * frameW wide, rows * frameH tall.
    float invCols = 1.f / (float)anim.sheetCols;

    // Determine sheet rows from texture size encoded in uvMax (assume full sheet = 1,1)
    // Height per row in UV space requires knowing sheetRows; approximate from sprite size:
    float cellW = invCols;
    float cellH = cellW * ((float)anim.frameH / (float)anim.frameW); // aspect-correct

    sprite.uvMin = { col * cellW, row * cellH };
    sprite.uvMax = { sprite.uvMin.x + cellW, sprite.uvMin.y + cellH };
    sprite.size  = { (float)anim.frameW, (float)anim.frameH };
}

// ---- Clip factory helpers ---------------------------------------------------
static void addClip(CAnimation& anim, const std::string& name,
                    int startFrame, int frames, float fps, bool loop) {
    AnimClip c;
    c.name       = name;
    c.frameStart = startFrame;
    c.frameCount = frames;
    c.fps        = fps;
    c.loop       = loop;
    anim.clips.push_back(c);
    if (anim.currentClip.empty()) anim.currentClip = name;
}

void AnimationSystem::addIdleClip  (CAnimation& a, int s, int f, float fps) { addClip(a, "Idle",   s, f, fps, true);  }
void AnimationSystem::addWalkClip  (CAnimation& a, int s, int f, float fps) { addClip(a, "Walk",   s, f, fps, true);  }
void AnimationSystem::addRunClip   (CAnimation& a, int s, int f, float fps) { addClip(a, "Run",    s, f, fps, true);  }
void AnimationSystem::addJumpClip  (CAnimation& a, int s, int f, float fps) { addClip(a, "Jump",   s, f, fps, false); }
void AnimationSystem::addFallClip  (CAnimation& a, int s, int f, float fps) { addClip(a, "Fall",   s, f, fps, true);  }
void AnimationSystem::addAttackClip(CAnimation& a, int s, int f, float fps) { addClip(a, "Attack", s, f, fps, false); }
void AnimationSystem::addDeathClip (CAnimation& a, int s, int f, float fps) { addClip(a, "Death",  s, f, fps, false); }
void AnimationSystem::addHurtClip  (CAnimation& a, int s, int f, float fps) { addClip(a, "Hurt",   s, f, fps, false); }
