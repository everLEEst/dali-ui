/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/integration-api/view-impl.h>

using namespace Dali;
using namespace Dali::Ui;

/**
 * LayoutScale sample — four side-by-side comparisons.
 *
 * All four cases use the exact same 3-level nested hierarchy:
 *
 *   outer  (StackLayout VERTICAL,  210px wide, dark-gray, padding=10)
 *    ├─ innerRow (StackLayout HORIZONTAL, MATCH_PARENT, dark-blue, padding=6, margin=6)
 *    │   ├─ leaf1  (44×44, red,   margin=4)
 *    │   ├─ leaf2  (44×44, green, margin=4)
 *    │   └─ leaf3  (44×44, amber, margin=4)
 *    └─ bottomBar  (MATCH_PARENT × 18, magenta, margin-top=5)
 *
 * Case 1 [gray  bar] — No scale (baseline).
 * Case 2 [amber bar] — Actor Scale 1.5× on outer.
 *                      Stretches the pixel buffer post-render; the view does
 *                      not take extra space in layout and may overlap neighbours.
 * Case 3 [teal  bar] — LayoutScale (1.5, 1.5) on outer.
 *                      All children are re-measured and re-rendered at 1.5× the
 *                      natural size; adjacent views see the larger footprint.
 * Case 4 [blue  bar] — Mixed LayoutScale per level.
 *                      outer=(1.5, 1.0), innerRow=(1.0, 1.5), leaf3=(0.5, 0.5).
 *                      Scales cascade multiplicatively down the tree.
 *
 * Press Escape / Back to quit.
 */

// ── palette ────────────────────────────────────────────────────────────────

// Hierarchy colors (same for every case)
const Vector4 BG_OUTER  (0.22f, 0.22f, 0.22f, 1.0f); // dark gray
const Vector4 BG_INNER  (0.10f, 0.22f, 0.42f, 1.0f); // dark blue
const Vector4 BG_LEAF1  (0.85f, 0.18f, 0.18f, 1.0f); // red
const Vector4 BG_LEAF2  (0.13f, 0.63f, 0.27f, 1.0f); // green
const Vector4 BG_LEAF3  (0.82f, 0.67f, 0.00f, 1.0f); // amber
const Vector4 BG_BOTTOM (0.65f, 0.13f, 0.65f, 1.0f); // magenta

// Per-case indicator colors (thin bar at the top of each section)
const Vector4 IND1(0.45f, 0.45f, 0.45f, 1.0f); // gray  — no scale
const Vector4 IND2(0.82f, 0.67f, 0.00f, 1.0f); // amber — actor scale
const Vector4 IND3(0.16f, 0.65f, 0.42f, 1.0f); // teal  — layout scale
const Vector4 IND4(0.13f, 0.47f, 0.80f, 1.0f); // blue  — mixed layout scale

// ── shared hierarchy ────────────────────────────────────────────────────────

struct NestedViews
{
  StackLayout outer;
  StackLayout innerRow;
  View        leaf1;
  View        leaf2;
  View        leaf3;
};

NestedViews CreateNested()
{
  // leaf boxes: fixed 44×44, coloured, with margin on all sides
  View leaf1 = View::New();
  leaf1.SetBackgroundColor(BG_LEAF1);
  leaf1.SetRequestedWidth(44.0f);
  leaf1.SetRequestedHeight(44.0f);
  leaf1.SetViewMargin(Extents(4, 4, 4, 4));

  View leaf2 = View::New();
  leaf2.SetBackgroundColor(BG_LEAF2);
  leaf2.SetRequestedWidth(44.0f);
  leaf2.SetRequestedHeight(44.0f);
  leaf2.SetViewMargin(Extents(4, 4, 4, 4));

  View leaf3 = View::New();
  leaf3.SetBackgroundColor(BG_LEAF3);
  leaf3.SetRequestedWidth(44.0f);
  leaf3.SetRequestedHeight(44.0f);
  leaf3.SetViewMargin(Extents(4, 4, 4, 4));

  // inner row: horizontal stack, fills outer width, wraps content height
  StackLayout innerRow = StackLayout::New(StackOrientation::HORIZONTAL);
  innerRow.SetBackgroundColor(BG_INNER);
  innerRow.SetRequestedWidth(MATCH_PARENT);
  innerRow.SetRequestedHeight(WRAP_CONTENT);
  innerRow.SetViewPadding(Extents(6, 6, 6, 6));
  innerRow.SetViewMargin(Extents(6, 6, 6, 6));
  innerRow.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
  innerRow.Add(leaf1);
  innerRow.Add(leaf2);
  innerRow.Add(leaf3);

  // bottom bar: fills outer width, fixed height
  View bottomBar = View::New();
  bottomBar.SetBackgroundColor(BG_BOTTOM);
  bottomBar.SetRequestedHeight(18.0f);
  bottomBar.SetViewMargin(Extents(0, 0, 5, 0));
  bottomBar.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));

  // outer box: vertical stack, fixed width, wraps height
  StackLayout outer = StackLayout::New(StackOrientation::VERTICAL);
  outer.SetBackgroundColor(BG_OUTER);
  outer.SetRequestedWidth(210.0f);
  outer.SetRequestedHeight(WRAP_CONTENT);
  outer.SetViewPadding(Extents(10, 10, 10, 10));
  outer.Add(innerRow);
  outer.Add(bottomBar);

  return {outer, innerRow, leaf1, leaf2, leaf3};
}

// ── case section wrapper ─────────────────────────────────────────────────────
// Each section: thin coloured indicator bar + the nested hierarchy.
// Background colour of the section makes cases easy to tell apart.

StackLayout CreateSection(const Vector4& indicatorColor, View content)
{
  View indicator = View::New();
  indicator.SetBackgroundColor(indicatorColor);
  indicator.SetRequestedHeight(6.0f);
  indicator.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));

  StackLayout section = StackLayout::New(StackOrientation::VERTICAL);
  section.SetBackgroundColor(Vector4(0.94f, 0.94f, 0.94f, 1.0f));
  section.SetRequestedWidth(MATCH_PARENT);
  section.SetRequestedHeight(WRAP_CONTENT);
  section.SetViewPadding(Extents(16, 16, 10, 12));
  section.SetSpacing(10.0f);
  section.Add(indicator);
  section.Add(content);

  return section;
}

// ── application controller ───────────────────────────────────────────────────

class LayoutScaleController : public ConnectionTracker
{
public:
  LayoutScaleController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &LayoutScaleController::Create);
  }

  void Create(Application& application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);

    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetViewPadding(Extents(20, 20, 16, 16));
    root.SetSpacing(14.0f);

    // ── Case 1: No scale ─────────────────────────────────────────────────
    // Baseline. The hierarchy is displayed at its natural size.
    {
      NestedViews n = CreateNested();
      root.Add(CreateSection(IND1, n.outer));
    }

    // ── Case 2: Actor Scale 1.5× on outer ────────────────────────────────
    // SetScaleX/Y is a render transform applied to the Actor after the pixel
    // buffer is allocated at the original size.  The view visually appears
    // 1.5× larger (scaling from the Actor pivot at its centre) but the layout
    // engine sees the original measured size, so the section does NOT grow and
    // the scaled view can overlap the section below.
    {
      NestedViews n = CreateNested();
      Integration::GetImpl(n.outer).SetScaleX(1.5f);
      Integration::GetImpl(n.outer).SetScaleY(1.5f);
      root.Add(CreateSection(IND2, n.outer));
    }

    // ── Case 3: LayoutScale (1.5, 1.5) on outer ─────────────────────────
    // SetLayoutScaleX/Y integrates with the Measure/Arrange pipeline.
    // outer is measured as if it were 1/1.5 × the available constraint, then
    // its result is multiplied by 1.5 before reporting to the parent.
    // Every descendant is genuinely rendered at a 1.5× larger pixel size.
    // The section expands to accommodate the larger footprint.
    {
      NestedViews n = CreateNested();
      Integration::GetImpl(n.outer).SetLayoutScaleX(1.5f);
      Integration::GetImpl(n.outer).SetLayoutScaleY(1.5f);
      root.Add(CreateSection(IND3, n.outer));
    }

    // ── Case 4: Mixed LayoutScale (cascading) ────────────────────────────
    // Each level carries its own LayoutScale; scales compound multiplicatively.
    //
    //   outer    LayoutScale = (1.5, 1.0) → horizontal stretch only
    //   innerRow LayoutScale = (1.0, 1.5) → vertical stretch only
    //                          (cascades: innerRow effective = outer × own)
    //   leaf3    LayoutScale = (0.5, 0.5) → shrink
    //                          (cascades: leaf3 effective = outer × innerRow × own)
    {
      NestedViews n = CreateNested();
      Integration::GetImpl(n.outer).SetLayoutScaleX(1.5f);
      Integration::GetImpl(n.outer).SetLayoutScaleY(1.0f);
      Integration::GetImpl(n.innerRow).SetLayoutScaleX(1.0f);
      Integration::GetImpl(n.innerRow).SetLayoutScaleY(1.5f);
      Integration::GetImpl(n.leaf3).SetLayoutScaleX(0.5f);
      Integration::GetImpl(n.leaf3).SetLayoutScaleY(0.5f);
      root.Add(CreateSection(IND4, n.outer));
    }

    window.Add(root);
    window.KeyEventSignal().Connect(this, &LayoutScaleController::OnKeyEvent);
  }

  void OnKeyEvent(const KeyEvent& event)
  {
    if(event.GetState() == KeyEvent::DOWN)
    {
      if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
      {
        mApplication.Quit();
      }
    }
  }

private:
  Application& mApplication;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application           application = Application::New(&argc, &argv);
  LayoutScaleController controller(application);
  application.MainLoop();
  return 0;
}
