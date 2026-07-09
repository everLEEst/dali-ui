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

#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/views/recycler/group-data-source.h>
#include <dali-ui-foundation/public-api/views/recycler/item-adapter.h>
#include <dali-ui-foundation/public-api/views/recycler/item-view-holder.h>
#include <dali/public-api/object/base-handle.h>
#include <dali/public-api/signals/dali-signal.h>
#include <cstdint>
#include <limits>

namespace Dali
{
namespace Ui
{

class GroupAdapterImpl;

/**
 * @brief Adapter handle that maps a grouped list structure to a flat item index.
 *
 * GroupAdapter is a reference-counted handle (BaseHandle). Create via GroupAdapter::New().
 * It owns an internal flat ItemAdapter (returned by GetAdapter()) that is passed to RecyclerView.
 * It injects GAP rows between groups and HEADER rows if the data source declares them.
 * Body items are delegated to the inner adapter supplied by SetInnerAdapter.
 *
 * GAP views are created internally as transparent spacers.
 * Header views are created via CreateHeaderViewHolderSignal / BindHeaderViewHolderSignal.
 *
 * Lifetime requirements:
 *   - GroupDataSource must outlive this GroupAdapter (or ClearDataSource() must be called first).
 *   - The inner ItemAdapter handle may be replaced at any time via SetInnerAdapter.
 *     GroupAdapter holds a reference to the inner adapter — it stays alive as long as the
 *     GroupAdapter holds it. To detach, call SetInnerAdapter({}).
 *
 * Usage:
 * @code
 *   GroupAdapter groupAdapter = GroupAdapter::New();
 *   groupAdapter.SetDataSource(mySource);
 *   groupAdapter.SetInnerAdapter(myItemAdapter);
 *   recyclerView.SetAdapter(groupAdapter.GetAdapter());
 * @endcode
 */
class DALI_UI_API GroupAdapter : public Dali::BaseHandle
{
public:
  // View types reserved for internal use. The inner adapter's GetItemViewTypeSignal
  // must never return these values — doing so corrupts the recycle pool.
  static constexpr uint32_t RESERVED_VIEW_TYPE_GAP    = std::numeric_limits<uint32_t>::max();
  static constexpr uint32_t RESERVED_VIEW_TYPE_HEADER = std::numeric_limits<uint32_t>::max() - 1u;

  // Flat record describing a single row in the flattened list.
  struct FlatRecord
  {
    GroupRowType rowType{GroupRowType::NONE};
    uint32_t     groupIndex{ItemViewHolder::INVALID_GROUP_INDEX};
    uint32_t     innerPosition{std::numeric_limits<uint32_t>::max()};
  };

  /**
   * @brief Creates a new GroupAdapter. Returns an initialized handle.
   */
  static GroupAdapter New();

  /**
   * @brief Creates an uninitialized (null) handle. Evaluates to false.
   */
  GroupAdapter() = default;

  ~GroupAdapter() = default;

  GroupAdapter(const GroupAdapter&)            = default;
  GroupAdapter(GroupAdapter&&)                 = default;
  GroupAdapter& operator=(const GroupAdapter&) = default;
  GroupAdapter& operator=(GroupAdapter&&)      = default;

  // source must outlive this GroupAdapter. Call ClearDataSource() before destroying source.
  void SetDataSource(GroupDataSource& source);
  // Explicitly clears the data source pointer and empties the flat index.
  void ClearDataSource();

  // Sets the inner adapter. GroupAdapter holds a reference (RC handle).
  // Pass an empty handle (GroupAdapter{}) to detach.
  void SetInnerAdapter(ItemAdapter inner);

  // Height of injected GAP rows between groups. Default: 16.
  void  SetGapHeight(float height);
  float GetGapHeight() const;

  // Header view signals — connect these to supply header views.
  // CreateHeaderViewHolderSignal handler must set holder.view before returning.
  Dali::Signal<void(ItemViewHolder&)>& CreateHeaderViewHolderSignal();
  Dali::Signal<void(ItemViewHolder&)>& BindHeaderViewHolderSignal();
  Dali::Signal<void(ItemViewHolder&)>& RecycleHeaderViewHolderSignal();

  // Call after the underlying data set changes to rebuild the flat index.
  void NotifyDataSetChanged();

  // Returns the internal flat ItemAdapter to pass to RecyclerView::SetAdapter.
  ItemAdapter GetAdapter() const;

  // --- Mapping helpers (consumed by GroupLinearItemsLayouter) ----------------

  uint32_t          GetFlatCount() const;
  const FlatRecord& GetRecord(uint32_t flatPosition) const;
  GroupRowType      GetRowType(uint32_t flatPosition) const;
  uint32_t          GetGroupIndex(uint32_t flatPosition) const;

public: // Not intended for application developers
  /// @cond internal
  explicit GroupAdapter(GroupAdapterImpl* impl);

  GroupAdapterImpl&       GetImpl();
  const GroupAdapterImpl& GetImpl() const;
  /// @endcond
};

} // namespace Ui
} // namespace Dali
