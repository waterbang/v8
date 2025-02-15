// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_UNITTESTS_HEAP_HEAP_UTILS_H_
#define V8_UNITTESTS_HEAP_HEAP_UTILS_H_

#include "src/base/macros.h"
#include "src/common/globals.h"
#include "src/heap/heap.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace v8 {
namespace internal {

class HeapInternalsBase {
 protected:
  void SimulateIncrementalMarking(Heap* heap, bool force_completion);
  void SimulateFullSpace(
      v8::internal::NewSpace* space,
      std::vector<Handle<FixedArray>>* out_handles = nullptr);
  void SimulateFullSpace(v8::internal::PagedSpace* space);
  bool FillCurrentPageButNBytes(
      v8::internal::NewSpace* space, int extra_bytes,
      std::vector<Handle<FixedArray>>* out_handles = nullptr);
  bool FillCurrentPage(v8::internal::NewSpace* space,
                       std::vector<Handle<FixedArray>>* out_handles = nullptr);
  std::vector<Handle<FixedArray>> CreatePadding(
      Heap* heap, int padding_size, AllocationType allocation,
      int object_size = kMaxRegularHeapObjectSize);
  int FixedArrayLenFromSize(int size);

 private:
  void SimulateFullSpace(
      v8::internal::PagedNewSpace* space,
      std::vector<Handle<FixedArray>>* out_handles = nullptr);
};

template <typename TMixin>
class WithHeapInternals : public TMixin, HeapInternalsBase {
 public:
  WithHeapInternals() = default;
  WithHeapInternals(const WithHeapInternals&) = delete;
  WithHeapInternals& operator=(const WithHeapInternals&) = delete;

  void CollectGarbage(i::AllocationSpace space) {
    heap()->CollectGarbage(space, i::GarbageCollectionReason::kTesting);
  }

  void FullGC() {
    heap()->CollectGarbage(OLD_SPACE, i::GarbageCollectionReason::kTesting);
  }

  void YoungGC() {
    heap()->CollectGarbage(NEW_SPACE, i::GarbageCollectionReason::kTesting);
  }

  void CollectAllAvailableGarbage() {
    heap()->CollectAllAvailableGarbage(i::GarbageCollectionReason::kTesting);
  }

  Heap* heap() const { return this->i_isolate()->heap(); }

  void SimulateIncrementalMarking(bool force_completion = true) {
    return HeapInternalsBase::SimulateIncrementalMarking(heap(),
                                                         force_completion);
  }

  void SimulateFullSpace(
      v8::internal::NewSpace* space,
      std::vector<Handle<FixedArray>>* out_handles = nullptr) {
    return HeapInternalsBase::SimulateFullSpace(space, out_handles);
  }
  void SimulateFullSpace(v8::internal::PagedSpace* space) {
    return HeapInternalsBase::SimulateFullSpace(space);
  }

  void GrowNewSpace() {
    SafepointScope scope(heap());
    if (!heap()->new_space()->IsAtMaximumCapacity()) {
      heap()->new_space()->Grow();
    }
  }

  void SealCurrentObjects() {
    // If you see this check failing, disable the flag at the start of your
    // test: FLAG_stress_concurrent_allocation = false; Background thread
    // allocating concurrently interferes with this function.
    CHECK(!FLAG_stress_concurrent_allocation);
    FullGC();
    FullGC();
    heap()->mark_compact_collector()->EnsureSweepingCompleted(
        MarkCompactCollector::SweepingForcedFinalizationMode::kV8Only);
    heap()->old_space()->FreeLinearAllocationArea();
    for (Page* page : *heap()->old_space()) {
      page->MarkNeverAllocateForTesting();
    }
  }
};

using TestWithHeapInternals =                  //
    WithHeapInternals<                         //
        WithInternalIsolateMixin<              //
            WithIsolateScopeMixin<             //
                WithIsolateMixin<              //
                    WithDefaultPlatformMixin<  //
                        ::testing::Test>>>>>;

using TestWithHeapInternalsAndContext =  //
    WithContextMixin<                    //
        TestWithHeapInternals>;

inline void FullGC(v8::Isolate* isolate) {
  reinterpret_cast<i::Isolate*>(isolate)->heap()->CollectAllGarbage(
      i::Heap::kNoGCFlags, i::GarbageCollectionReason::kTesting);
}

inline void YoungGC(v8::Isolate* isolate) {
  reinterpret_cast<i::Isolate*>(isolate)->heap()->CollectGarbage(
      i::NEW_SPACE, i::GarbageCollectionReason::kTesting);
}

}  // namespace internal
}  // namespace v8

#endif  // V8_UNITTESTS_HEAP_HEAP_UTILS_H_
