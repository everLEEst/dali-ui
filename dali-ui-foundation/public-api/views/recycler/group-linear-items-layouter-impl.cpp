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
 *
 */

#include <dali-ui-foundation/public-api/views/recycler/group-linear-items-layouter-impl.h>

namespace Dali
{
namespace Ui
{

GroupLinearItemsLayouterImplPtr GroupLinearItemsLayouterImpl::New(Orientation orientation)
{
  return new GroupLinearItemsLayouterImpl(orientation);
}

GroupLinearItemsLayouterImpl::GroupLinearItemsLayouterImpl(Orientation orientation)
: LinearItemsLayouterImpl(orientation)
{
}

GroupLinearItemsLayouterImpl::~GroupLinearItemsLayouterImpl() = default;

void GroupLinearItemsLayouterImpl::SetGroupAdapter(GroupAdapter adapter)
{
  mGroupAdapter = std::move(adapter);
}

void GroupLinearItemsLayouterImpl::SetBodyHorizontalMargin(float left, float right)
{
  mBodyMarginLeft  = left;
  mBodyMarginRight = right;
  InvalidateLayout();
}

float GroupLinearItemsLayouterImpl::GetBodyMarginLeft() const
{
  return mBodyMarginLeft;
}

float GroupLinearItemsLayouterImpl::GetBodyMarginRight() const
{
  return mBodyMarginRight;
}

LinearItemsLayouterImpl::ItemInset GroupLinearItemsLayouterImpl::GetItemCrossInset(uint32_t position) const
{
  if(!mGroupAdapter || (mBodyMarginLeft == 0.0f && mBodyMarginRight == 0.0f))
  {
    return {};
  }

  const GroupRowType rt = mGroupAdapter.GetRowType(position);
  if(rt == GroupRowType::BODY_SINGLE ||
     rt == GroupRowType::BODY_TOP ||
     rt == GroupRowType::BODY_MIDDLE ||
     rt == GroupRowType::BODY_BOTTOM)
  {
    return {mBodyMarginLeft, mBodyMarginRight};
  }

  return {};
}

LayoutRect GroupLinearItemsLayouterImpl::GetItemBounds(uint32_t position, float crossExtent) const
{
  LayoutRect bounds = LinearItemsLayouterImpl::GetItemBounds(position, crossExtent);

  if(!mGroupAdapter || (mBodyMarginLeft == 0.0f && mBodyMarginRight == 0.0f))
  {
    return bounds;
  }

  const GroupRowType rt = mGroupAdapter.GetRowType(position);
  if(rt == GroupRowType::BODY_SINGLE ||
     rt == GroupRowType::BODY_TOP ||
     rt == GroupRowType::BODY_MIDDLE ||
     rt == GroupRowType::BODY_BOTTOM)
  {
    bounds.x += mBodyMarginLeft;
    bounds.width -= (mBodyMarginLeft + mBodyMarginRight);
    if(bounds.width < 0.0f) bounds.width = 0.0f;
  }

  return bounds;
}

} // namespace Ui
} // namespace Dali
