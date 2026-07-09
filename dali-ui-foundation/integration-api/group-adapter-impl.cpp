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

#include <dali-ui-foundation/integration-api/group-adapter-impl.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali/integration-api/debug.h>
#include <limits>

namespace Dali
{
namespace Ui
{

constexpr uint32_t GAP_VIEW_TYPE    = GroupAdapter::RESERVED_VIEW_TYPE_GAP;
constexpr uint32_t HEADER_VIEW_TYPE = GroupAdapter::RESERVED_VIEW_TYPE_HEADER;

const GroupAdapter::FlatRecord GroupAdapterImpl::sInvalidRecord{
  GroupRowType::NONE,
  ItemViewHolder::INVALID_GROUP_INDEX,
  std::numeric_limits<uint32_t>::max()};

GroupAdapterImplPtr GroupAdapterImpl::New()
{
  return new GroupAdapterImpl();
}

GroupAdapterImpl::GroupAdapterImpl()
: mFlatAdapter(ItemAdapter::New())
{
  mFlatAdapter.GetItemCountSignal().Connect(mSlotDelegate, &GroupAdapterImpl::OnGetItemCount);
  mFlatAdapter.GetItemViewTypeSignal().Connect(mSlotDelegate, &GroupAdapterImpl::OnGetItemViewType);
  mFlatAdapter.CreateViewHolderSignal().Connect(mSlotDelegate, &GroupAdapterImpl::OnCreateViewHolder);
  mFlatAdapter.BindViewHolderSignal().Connect(mSlotDelegate, &GroupAdapterImpl::OnBindViewHolder);
  mFlatAdapter.RecycleViewHolderSignal().Connect(mSlotDelegate, &GroupAdapterImpl::OnRecycleViewHolder);
}

void GroupAdapterImpl::SetDataSource(GroupDataSource& source)
{
  mSource = &source;
  Rebuild();
}

void GroupAdapterImpl::ClearDataSource()
{
  mSource = nullptr;
  mRecords.clear();
  mFlatAdapter.NotifyDataSetChanged();
}

void GroupAdapterImpl::SetInnerAdapter(ItemAdapter inner)
{
  mInner = std::move(inner);
}

void GroupAdapterImpl::SetGapHeight(float height)
{
  mGapHeight = height;
}

float GroupAdapterImpl::GetGapHeight() const
{
  return mGapHeight;
}

Signal<void(ItemViewHolder&)>& GroupAdapterImpl::CreateHeaderViewHolderSignal()
{
  return mCreateHeaderSignal;
}

Signal<void(ItemViewHolder&)>& GroupAdapterImpl::BindHeaderViewHolderSignal()
{
  return mBindHeaderSignal;
}

Signal<void(ItemViewHolder&)>& GroupAdapterImpl::RecycleHeaderViewHolderSignal()
{
  return mRecycleHeaderSignal;
}

void GroupAdapterImpl::NotifyDataSetChanged()
{
  Rebuild();
  mFlatAdapter.NotifyDataSetChanged();
}

ItemAdapter GroupAdapterImpl::GetFlatAdapter() const
{
  return mFlatAdapter;
}

uint32_t GroupAdapterImpl::GetFlatCount() const
{
  return static_cast<uint32_t>(mRecords.size());
}

const GroupAdapter::FlatRecord& GroupAdapterImpl::GetRecord(uint32_t flatPosition) const
{
  if(flatPosition >= mRecords.size())
  {
    return sInvalidRecord;
  }
  return mRecords[flatPosition];
}

GroupRowType GroupAdapterImpl::GetRowType(uint32_t flatPosition) const
{
  return GetRecord(flatPosition).rowType;
}

uint32_t GroupAdapterImpl::GetGroupIndex(uint32_t flatPosition) const
{
  return GetRecord(flatPosition).groupIndex;
}

// ---------------------------------------------------------------------------

void GroupAdapterImpl::Rebuild()
{
  mRecords.clear();
  if(!mSource)
  {
    return;
  }

  const uint32_t groupCount = mSource->GetGroupCount();
  uint32_t       innerPos   = 0u;

  for(uint32_t g = 0u; g < groupCount; ++g)
  {
    if(g > 0u)
    {
      mRecords.push_back({GroupRowType::GAP, g, std::numeric_limits<uint32_t>::max()});
    }

    if(mSource->HasGroupHeader(g))
    {
      mRecords.push_back({GroupRowType::HEADER, g, std::numeric_limits<uint32_t>::max()});
    }

    const uint32_t bodyCount = mSource->GetGroupItemCount(g);
    for(uint32_t b = 0u; b < bodyCount; ++b)
    {
      GroupRowType rt;
      if(bodyCount == 1u)
      {
        rt = GroupRowType::BODY_SINGLE;
      }
      else if(b == 0u)
      {
        rt = GroupRowType::BODY_TOP;
      }
      else if(b == bodyCount - 1u)
      {
        rt = GroupRowType::BODY_BOTTOM;
      }
      else
      {
        rt = GroupRowType::BODY_MIDDLE;
      }
      mRecords.push_back({rt, g, innerPos++});
    }
  }
}

void GroupAdapterImpl::FillHolderGroupInfo(ItemViewHolder& holder) const
{
  const GroupAdapter::FlatRecord& rec = GetRecord(holder.position);
  holder.rowType                      = rec.rowType;
  holder.groupIndex                   = rec.groupIndex;
}

uint32_t GroupAdapterImpl::OnGetItemCount()
{
  return static_cast<uint32_t>(mRecords.size());
}

uint32_t GroupAdapterImpl::OnGetItemViewType(uint32_t flatPosition)
{
  const GroupAdapter::FlatRecord& rec = GetRecord(flatPosition);
  if(rec.rowType == GroupRowType::GAP)
  {
    return GAP_VIEW_TYPE;
  }
  if(rec.rowType == GroupRowType::HEADER)
  {
    return HEADER_VIEW_TYPE;
  }
  if(!mInner)
  {
    return 0u;
  }
  const uint32_t queryPos = (rec.innerPosition != std::numeric_limits<uint32_t>::max())
                              ? rec.innerPosition
                              : flatPosition;
  const uint32_t type     = mInner.GetItemViewType(queryPos);
  if(type == GAP_VIEW_TYPE || type == HEADER_VIEW_TYPE)
  {
    DALI_LOG_ERROR(
      "GroupAdapter: inner adapter view type %u collides with a reserved type "
      "(RESERVED_VIEW_TYPE_GAP or RESERVED_VIEW_TYPE_HEADER). "
      "Clamping to 0 to prevent recycle pool corruption.\n",
      type);
    return 0u;
  }
  return type;
}

void GroupAdapterImpl::OnCreateViewHolder(ItemViewHolder& holder)
{
  FillHolderGroupInfo(holder);

  if(holder.rowType == GroupRowType::GAP)
  {
    View spacer = View::New();
    spacer.SetRequestedHeight(mGapHeight);
    holder.view = spacer;
    return;
  }

  if(holder.rowType == GroupRowType::HEADER)
  {
    if(mCreateHeaderSignal.GetConnectionCount() > 0u)
    {
      mCreateHeaderSignal.Emit(holder);
    }
    if(!holder.view)
    {
      DALI_LOG_ERROR(
        "GroupAdapter: CreateHeaderViewHolderSignal did not set holder.view "
        "for groupIndex=%u. Using invisible spacer.\n",
        holder.groupIndex);
      View spacer = View::New();
      spacer.SetRequestedHeight(0.0f);
      holder.view = spacer;
    }
    return;
  }

  if(!mInner)
  {
    return;
  }

  const GroupAdapter::FlatRecord& rec     = GetRecord(holder.position);
  const uint32_t                  origPos = holder.position;

  if(rec.innerPosition != std::numeric_limits<uint32_t>::max())
  {
    holder.position = rec.innerPosition;
  }

  mInner.CreateViewHolder(holder);
  holder.position   = origPos;
  holder.rowType    = rec.rowType;
  holder.groupIndex = rec.groupIndex;
}

void GroupAdapterImpl::OnBindViewHolder(ItemViewHolder& holder)
{
  FillHolderGroupInfo(holder);

  if(holder.rowType == GroupRowType::GAP)
  {
    return;
  }

  if(holder.rowType == GroupRowType::HEADER)
  {
    if(mBindHeaderSignal.GetConnectionCount() > 0u)
    {
      mBindHeaderSignal.Emit(holder);
    }
    return;
  }

  if(!mInner)
  {
    return;
  }

  const GroupAdapter::FlatRecord& rec     = GetRecord(holder.position);
  const uint32_t                  origPos = holder.position;

  if(rec.innerPosition != std::numeric_limits<uint32_t>::max())
  {
    holder.position = rec.innerPosition;
  }

  mInner.BindViewHolder(holder);
  holder.position   = origPos;
  holder.rowType    = rec.rowType;
  holder.groupIndex = rec.groupIndex;
}

void GroupAdapterImpl::OnRecycleViewHolder(ItemViewHolder& holder)
{
  if(holder.rowType == GroupRowType::GAP)
  {
    return;
  }

  if(holder.rowType == GroupRowType::HEADER)
  {
    if(mRecycleHeaderSignal.GetConnectionCount() > 0u)
    {
      mRecycleHeaderSignal.Emit(holder);
    }
    return;
  }

  if(mInner)
  {
    mInner.RecycleViewHolder(holder);
  }
}

} // namespace Ui
} // namespace Dali
