#pragma once

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

#include <dali-ui-foundation/public-api/views/recycler/group-adapter.h>
#include <dali-ui-foundation/public-api/views/recycler/group-data-source.h>
#include <dali-ui-foundation/public-api/views/recycler/item-adapter.h>
#include <dali-ui-foundation/public-api/views/recycler/item-view-holder.h>
#include <dali/public-api/common/intrusive-ptr.h>
#include <dali/public-api/object/base-object.h>
#include <dali/public-api/signals/dali-signal.h>
#include <dali/public-api/signals/slot-delegate.h>
#include <limits>
#include <vector>

namespace Dali
{
namespace Ui
{

class GroupAdapterImpl;
using GroupAdapterImplPtr = IntrusivePtr<GroupAdapterImpl>;

/**
 * @brief Reference-counted implementation object for GroupAdapter.
 *
 * Owns an internal flat ItemAdapter (mFlatAdapter) that is passed to RecyclerView,
 * and holds a handle to the user-supplied inner ItemAdapter (mInner).
 * SlotDelegate is used to connect to mFlatAdapter's signals without inheriting
 * from ConnectionTracker.
 */
class DALI_UI_API GroupAdapterImpl : public Dali::BaseObject
{
public:
  static GroupAdapterImplPtr New();

  void SetDataSource(GroupDataSource& source);
  void ClearDataSource();

  void  SetInnerAdapter(ItemAdapter inner);
  float GetGapHeight() const;
  void  SetGapHeight(float height);

  Dali::Signal<void(ItemViewHolder&)>& CreateHeaderViewHolderSignal();
  Dali::Signal<void(ItemViewHolder&)>& BindHeaderViewHolderSignal();
  Dali::Signal<void(ItemViewHolder&)>& RecycleHeaderViewHolderSignal();

  void NotifyDataSetChanged();

  ItemAdapter GetFlatAdapter() const;

  uint32_t                        GetFlatCount() const;
  const GroupAdapter::FlatRecord& GetRecord(uint32_t flatPosition) const;
  GroupRowType                    GetRowType(uint32_t flatPosition) const;
  uint32_t                        GetGroupIndex(uint32_t flatPosition) const;

protected:
  GroupAdapterImpl();
  ~GroupAdapterImpl() override = default;

private:
  void Rebuild();
  void FillHolderGroupInfo(ItemViewHolder& holder) const;

  uint32_t OnGetItemCount();
  uint32_t OnGetItemViewType(uint32_t flatPosition);
  void     OnCreateViewHolder(ItemViewHolder& holder);
  void     OnBindViewHolder(ItemViewHolder& holder);
  void     OnRecycleViewHolder(ItemViewHolder& holder);

  GroupDataSource*                      mSource{nullptr};
  ItemAdapter                           mInner;
  ItemAdapter                           mFlatAdapter;
  float                                 mGapHeight{16.0f};
  std::vector<GroupAdapter::FlatRecord> mRecords;

  Dali::Signal<void(ItemViewHolder&)> mCreateHeaderSignal;
  Dali::Signal<void(ItemViewHolder&)> mBindHeaderSignal;
  Dali::Signal<void(ItemViewHolder&)> mRecycleHeaderSignal;

  Dali::SlotDelegate<GroupAdapterImpl> mSlotDelegate{this};

  static const GroupAdapter::FlatRecord sInvalidRecord;
};

} // namespace Ui
} // namespace Dali
