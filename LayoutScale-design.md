# LayoutScale Property — Design Document

## Problem

The existing `Scale` property (`Actor::Property::SCALE_X/Y`) is a **render transform** — it scales the visual output after the pixel buffer is allocated at the original size, then stretches it. This produces poor render quality when scaling up.

**Goal**: Add a new property that scales a view and all its children at the **layout level**, so they are actually measured and rendered at the larger size.

> Example: view measures 100×100, child measures 50×50.
> With `LayoutScale=1.5`, view becomes 150×150 and child becomes 75×75 — actually rendered at that size, not stretched.

---

## Recommended Name: `LayoutScale`

| Candidate | Verdict |
|---|---|
| `UIScale` | Too generic; conflicts with display DPI/density concepts |
| `ScaleFactor` | Ambiguous — could mean render or layout |
| `ContentScale` | iOS term for pixel density (misleading) |
| **`LayoutScale`** ✓ | Precisely describes its domain; consistent with existing naming (`RequestedWidth`, `MinimumHeight`, `ViewMargin`) |
| `RenderScale` | Wrong — this is the opposite of a render transform |

`LayoutScale` clearly distinguishes itself from the existing `Scale` (render transform) and `Actor::Property::SCALE`.

---

## Architecture Overview

The existing pipeline:

```
LayoutController
  └─ ViewImpl::Measure(wConstraint, hConstraint)
       └─ OnMeasure(w, h)  [virtual / callback]
            └─ LayoutManager::Measure(view, w, h)
                 └─ MeasureChild(child, w, h)  →  child->Measure(...)
  └─ ViewImpl::Arrange(bounds)
       └─ OnArrange(bounds)  [virtual / callback]
            └─ LayoutManager::ArrangeChildren(view, bounds)
                 └─ ArrangeChild(child, childBounds)  →  child->Arrange(...)
```

`Scale` today plugs into `Actor::Property::SCALE_X/Y` — entirely outside this pipeline. `LayoutScale` needs to plug **into** the pipeline at the Measure and Arrange wrappers.

---

## Implementation

### Pipeline with LayoutScale

```
ViewImpl::Measure(wConstraint, hConstraint)
│
├─ naturalW = wConstraint  / mLayoutScale   ← shrink constraint
│  naturalH = hConstraint / mLayoutScale      (children see smaller available space)
│
├─ naturalSize = OnMeasure(naturalW, naturalH)
│                └─ MeasureChild(child, naturalW, naturalH)
│                     └─ child->Measure(...)  [child handles its own LayoutScale]
│
└─ mMeasuredSize = naturalSize × mLayoutScale  ← report bigger size to parent


ViewImpl::Arrange(bounds)                   ← bounds = 150×150 (already scaled)
│
├─ naturalBounds = bounds / mLayoutScale    ← 100×100 for LayoutManager
│
├─ OnArrange(naturalBounds)
│    └─ LayoutManager::ArrangeChildren(view, naturalBounds)
│         └─ ArrangeChild(child, naturalChildBounds)
│              scaledChildBounds = naturalChildBounds × parent.mLayoutScale
│              child->Arrange(scaledChildBounds)   ← child gets 75×75
│
└─ Actor SIZE = bounds.GetWidth() × bounds.GetHeight()  ← 150×150, no SCALE used
```

### Touch Points

#### 1. `ViewImpl` — add member and API

In [view-impl.h](dali-ui/dali-ui-foundation/integration-api/view-impl.h) private members (alongside `mRequestedWidth` etc.):

```cpp
float mLayoutScale{1.0f};
```

Public API (mirroring existing scale/size API style):

```cpp
void  SetLayoutScale(float scale);
float GetLayoutScale() const;
```

`SetLayoutScale()` stores the value and calls `InvalidateMeasure()` to trigger re-layout.

#### 2. `ViewImpl::Measure()` wrapper

Scale constraints down before `OnMeasure`, scale result back up:

```cpp
// Only scale positive constraints; leave WRAP_CONTENT=-1 and MATCH_PARENT=-2 as-is
float nw = (widthConstraint  > 0.f) ? widthConstraint  / mLayoutScale : widthConstraint;
float nh = (heightConstraint > 0.f) ? heightConstraint / mLayoutScale : heightConstraint;

MeasuredSize natural = OnMeasure(nw, nh);   // or LayoutCallbacks
natural = ApplyConstraints(natural);         // min/max applied in natural space

mMeasuredSize = MeasuredSize(natural.GetWidth()  * mLayoutScale,
                              natural.GetHeight() * mLayoutScale);
```

#### 3. `ViewImpl::Arrange()` wrapper

Pass natural bounds to `OnArrange`; Actor gets the scaled bounds:

```cpp
LayoutRect naturalBounds(bounds.GetX(), bounds.GetY(),
                          bounds.GetWidth()  / mLayoutScale,
                          bounds.GetHeight() / mLayoutScale);

OnArrange(naturalBounds);   // LayoutManager arranges children in natural space

// Set Actor size to the full scaled bounds — no Actor::SCALE used
Self().SetProperty(Actor::Property::SIZE,
                   Vector3(bounds.GetWidth(), bounds.GetHeight(), 0.f));
```

#### 4. `LayoutManager::ArrangeChild()`

Apply parent's `mLayoutScale` to child bounds before calling `child->Arrange()`.

`ArrangeChildren(view, bounds)` already receives the parent `view`, so `mLayoutScale` is accessible. Apply it inside `ArrangeChild`:

```cpp
MeasuredSize LayoutManager::ArrangeChild(Integration::ViewImpl* parent,
                                          Integration::ViewImpl* child,
                                          const LayoutRect& naturalBounds)
{
    float scale = parent->GetLayoutScale();
    LayoutRect scaledBounds(
        naturalBounds.GetX()      * scale,
        naturalBounds.GetY()      * scale,
        naturalBounds.GetWidth()  * scale,
        naturalBounds.GetHeight() * scale);
    return child->Arrange(scaledBounds);
}
```

> **Note**: `ArrangeChild` currently takes only `child` and `bounds`. The signature needs to either store the parent inside `LayoutManager` when `ArrangeChildren()` is called, or change `ArrangeChild(child, bounds)` → `ArrangeChild(parent, child, bounds)`.

---

## Why This Approach

| Approach | Quality fixed? | Notes |
|---|---|---|
| **Measure/Arrange wrapper + ArrangeChild** ✓ | Yes | Children genuinely rendered at larger size; no Actor SCALE; works for all layout types |
| Modify `RequestedWidth/Height` on all children | No | Breaks WRAP_CONTENT/MATCH_PARENT; complex to undo on scale change |
| Post-scale children's Actor after `OnArrange` | No | Bypasses Arrange on children; pixel buffers still allocated at original size |
| Actor `SCALE` (existing behavior) | No | Exactly the problem being solved |

---

## Nested LayoutScale Behavior

`LayoutScale` values **compound at each level** — each level multiplies its immediate children's sizes:

```
Parent   LayoutScale=1.5
Child    LayoutScale=2.0
Grandchild natural size = 10×10

Result:
  Child       = 10 × 2.0 = 20×20  (child's LayoutScale)
  Child as seen by parent = 20 × 1.5 = 30×30
  Grandchild  = 10 × 2.0 = 20×20  (only child's LayoutScale affects grandchild directly)
```

Each `LayoutScale` transforms the coordinate space for its direct children. This is analogous to how `transform: scale()` compounds in CSS.

---

## Relation to Existing `Scale`

| Property | Where applied | Quality | Affects layout? |
|---|---|---|---|
| `Scale` (`SCALE_X/Y`) | Render transform (Actor) | Poor at >1.0 | No |
| `LayoutScale` | Measure/Arrange pipeline | Full quality at any value | Yes |

`LayoutScale=1.5` with `Scale=1.0` is **not** equivalent to `Scale=1.5` with `LayoutScale=1.0`.
The former allocates pixels at 150×150; the latter allocates at 100×100 and stretches.

---

## Summary

- **Property name**: `LayoutScale` — `GetLayoutScale()` / `SetLayoutScale(float)`
- **Storage**: `float mLayoutScale{1.0f}` in `ViewImpl` private members
- **Change set**:
  - `ViewImpl`: add member + getter/setter + `InvalidateMeasure()` on set
  - `ViewImpl::Measure()`: divide constraints, multiply result
  - `ViewImpl::Arrange()`: divide bounds for `OnArrange`, set Actor size to full bounds
  - `LayoutManager::ArrangeChild()`: multiply child bounds by parent's `mLayoutScale`
- **Key invariant**: `Actor::Property::SCALE` is **never used** — children are genuinely rendered at larger pixel sizes, fixing the quality issue entirely
