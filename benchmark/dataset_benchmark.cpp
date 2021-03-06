/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <benchmark/benchmark.h>

#include <numeric>
#include <random>

#include "dataset.h"

// Dataset::get requires a search based on a tag defined by the type and is thus
// potentially expensive.
static void BM_Dataset_get_with_many_columns(benchmark::State &state) {
  Dataset d;
  for (int i = 0; i < state.range(0); ++i)
    d.insert<Data::Value>("name" + std::to_string(i), Dimensions{}, 1);
  d.insert<Data::Int>("name", Dimensions{}, 1);
  for (auto _ : state)
    d.get<Data::Int>();
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_get_with_many_columns)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);

// Benchmark demonstrating a potential use of Dataset to replace Histogram. What
// are the performance implications?
static void BM_Dataset_as_Histogram(benchmark::State &state) {
  gsl::index nPoint = state.range(0);
  Dataset d;
  d.insert<Coord::Tof>({Dimension::Tof, nPoint}, nPoint);
  d.insert<Data::Value>("", {Dimension::Tof, nPoint}, nPoint);
  d.insert<Data::Variance>("", {Dimension::Tof, nPoint}, nPoint);
  std::vector<Dataset> histograms;
  gsl::index nSpec = std::min(1000000l, 10000000 / (nPoint + 1));
  for (gsl::index i = 0; i < nSpec; ++i) {
    auto hist(d);
    // Break sharing
    hist.get<Data::Value>();
    hist.get<Data::Variance>();
    histograms.push_back(hist);
  }

  for (auto _ : state) {
    auto sum = histograms[0];
    for (gsl::index i = 1; i < nSpec; ++i)
      sum += histograms[i];
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 2 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_as_Histogram)->RangeMultiplier(2)->Range(0, 2 << 14);

static void BM_Dataset_as_Histogram_with_slice(benchmark::State &state) {
  Dataset d;
  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);
  gsl::index nSpec = 10000;
  Dimensions dims({{Dimension::Tof, 1000}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());

  for (auto _ : state) {
    auto sum = slice(d, Dimension::Spectrum, 0);
    for (gsl::index i = 1; i < nSpec; ++i)
      sum += slice(d, Dimension::Spectrum, i);
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * 1000 * 2 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_as_Histogram_with_slice);

Dataset makeSingleDataDataset(const gsl::index nSpec, const gsl::index nPoint) {
  Dataset d;

  d.insert<Coord::DetectorId>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorPosition>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, nSpec}, nSpec);
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, nSpec}, nSpec);

  d.insert<Coord::Tof>({Dimension::Tof, nPoint}, nPoint);
  Dimensions dims({{Dimension::Tof, nPoint}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());

  return d;
}

Dataset makeDataset(const gsl::index nSpec, const gsl::index nPoint) {
  Dataset d;

  d.insert<Coord::DetectorId>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorPosition>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, nSpec}, nSpec);
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, nSpec}, nSpec);

  d.insert<Coord::Tof>({Dimension::Tof, nPoint}, nPoint);
  Dimensions dims({{Dimension::Tof, nPoint}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());
  d.insert<Data::Value>("background", dims, dims.volume());
  d.insert<Data::Variance>("background", dims, dims.volume());

  return d;
}

static void BM_Dataset_plus(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  for (auto _ : state) {
    d += d;
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 6 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_plus)->RangeMultiplier(2)->Range(2 << 9, 2 << 12);

static void BM_Dataset_multiply(benchmark::State &state) {
  gsl::index nSpec = state.range(0);
  gsl::index nPoint = 1024;
  auto d = makeSingleDataDataset(nSpec, nPoint);
  const auto d2 = makeSingleDataDataset(nSpec, nPoint);
  for (auto _ : state) {
    d *= d2;
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 6 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_multiply)->RangeMultiplier(2)->Range(2 << 0, 2 << 12);
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(8)
    ->UseRealTime();
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(12)
    ->UseRealTime();
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(24)
    ->UseRealTime();

Dataset doWork(Dataset d) {
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  return d;
}

static void BM_Dataset_cache_blocking_reference(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  auto out(d);
  for (auto _ : state) {
    d = doWork(std::move(d));
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2+2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 8 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_cache_blocking_reference)
    ->RangeMultiplier(2)
    ->Range(2 << 9, 2 << 12);

static void BM_Dataset_cache_blocking(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  for (auto _ : state) {
    for (gsl::index i = 0; i < nSpec; ++i) {
      d.setSlice(doWork(slice(d, Dimension::Spectrum, i)), Dimension::Spectrum,
                 i);
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2+2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 8 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_cache_blocking)
    ->RangeMultiplier(2)
    ->Range(2 << 9, 2 << 14);

static void BM_Dataset_cache_blocking_no_slicing(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  const auto d = makeDataset(nSpec, nPoint);
  std::vector<Dataset> slices;
  for (gsl::index i = 0; i < nSpec; ++i) {
    slices.emplace_back(slice(d, Dimension::Spectrum, i));
  }

  for (auto _ : state) {
    for (gsl::index i = 0; i < nSpec; ++i) {
      slices[i] = doWork(std::move(slices[i]));
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2+2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 8 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_cache_blocking_no_slicing)
    ->RangeMultiplier(2)
    ->Range(2 << 9, 2 << 14);

Dataset makeBeamline(const gsl::index nComp, const gsl::index nDet) {
  Dataset d;

  d.insert<Coord::DetectorId>({Dimension::Detector, nDet});
  d.insert<Coord::DetectorIsMonitor>({Dimension::Detector, nDet});
  d.insert<Coord::DetectorMask>({Dimension::Detector, nDet});
  d.insert<Coord::DetectorPosition>({Dimension::Detector, nDet});
  d.insert<Coord::DetectorRotation>({Dimension::Detector, nDet});
  d.insert<Coord::DetectorParent>({Dimension::Detector, nDet});
  d.insert<Coord::DetectorScale>({Dimension::Detector, nDet});
  // TODO As it is, this will break coordinate matching. We need a special
  // comparison for referenced shapes, or a shape factory.
  // d.insert<Coord::DetectorShape>({Dimension::Detector, nDet}, nDet,
  //                               std::make_shared<std::array<double, 100>>());

  d.insert<Coord::ComponentChildren>({Dimension::Component, nComp});
  d.insert<Coord::ComponentName>({Dimension::Component, nComp});
  d.insert<Coord::ComponentParent>({Dimension::Component, nComp});
  d.insert<Coord::ComponentPosition>({Dimension::Component, nComp});
  d.insert<Coord::ComponentRotation>({Dimension::Component, nComp});
  d.insert<Coord::ComponentScale>({Dimension::Component, nComp});
  d.insert<Coord::ComponentShape>({Dimension::Component, nComp});
  d.insert<Coord::ComponentSubtreeRange>({Dimension::Component, nComp});
  d.insert<Coord::DetectorSubtreeRange>({Dimension::Component, nComp});

  d.insert<Attr::ExperimentLog>("NeXus logs", {});

  // These are special, length is same, but there is no association with the
  // index in the dimension. Should handle this differently? Put it into a
  // zero-dimensional variable?
  d.insert<Coord::DetectorSubtree>({Dimension::Detector, nDet});
  d.insert<Coord::ComponentSubtree>({Dimension::Component, nComp});

  auto ids = d.get<Coord::DetectorId>();
  std::iota(ids.begin(), ids.end(), 1);

  return d;
}

Dataset makeSpectra(const gsl::index nSpec) {
  Dataset d;

  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, nSpec});
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, nSpec});
  auto groups = d.get<Coord::DetectorGrouping>();
  for (gsl::index i = 0; i < groups.size(); ++i)
    groups[i] = {i};
  auto nums = d.get<Coord::SpectrumNumber>();
  std::iota(nums.begin(), nums.end(), 1);

  return d;
}

Dataset makeData(const gsl::index nSpec, const gsl::index nPoint) {
  Dataset d;
  d.insert<Coord::Tof>({Dimension::Tof, nPoint + 1});
  auto tofs = d.get<Coord::Tof>();
  std::iota(tofs.begin(), tofs.end(), 0.0);
  Dimensions dims({{Dimension::Tof, nPoint}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims);
  d.insert<Data::Variance>("sample", dims);
  return d;
}

Dataset makeWorkspace2D(const gsl::index nSpec, const gsl::index nPoint) {
  auto d = makeBeamline(nSpec / 100, nSpec);
  d.merge(makeSpectra(nSpec));
  d.merge(makeData(nSpec, nPoint));
  return d;
}

static void BM_Dataset_Workspace2D_create(benchmark::State &state) {
  gsl::index nSpec = 1024 * 1024;
  gsl::index nPoint = 2;
  for (auto _ : state) {
    auto d = makeWorkspace2D(nSpec, nPoint);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_Workspace2D_create);

static void BM_Dataset_Workspace2D_copy(benchmark::State &state) {
  gsl::index nSpec = 1024 * 1024;
  gsl::index nPoint = 2;
  auto d = makeWorkspace2D(nSpec, nPoint);
  for (auto _ : state) {
    auto copy(d);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_Workspace2D_copy);

static void BM_Dataset_Workspace2D_copy_and_write(benchmark::State &state) {
  gsl::index nSpec = 1024 * 1024;
  gsl::index nPoint = state.range(0);
  auto d = makeWorkspace2D(nSpec, nPoint);
  for (auto _ : state) {
    auto copy(d);
    copy.get<Data::Value>()[0] = 1.0;
    copy.get<Data::Variance>()[0] = 1.0;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_Workspace2D_copy_and_write)
    ->RangeMultiplier(2)
    ->Range(2, 2 << 7);

static void BM_Dataset_Workspace2D_rebin(benchmark::State &state) {
  gsl::index nSpec = state.range(0) * 1024;
  gsl::index nPoint = 1024;

  auto newCoord = makeVariable<Coord::Tof>({Dimension::Tof, nPoint / 2});
  double value = 0.0;
  for(auto &tof : newCoord.get<Coord::Tof>()) {
    tof = value;
    value += 3.0;
  }

  for (auto _ : state) {
    state.PauseTiming();
    auto d = makeData(nSpec, nPoint);
    state.ResumeTiming();
    auto rebinned = rebin(d, newCoord);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * nSpec * (nPoint + nPoint / 2) *
                          (1 + 1) * sizeof(double));
}
BENCHMARK(BM_Dataset_Workspace2D_rebin)
    ->RangeMultiplier(2)
    ->Range(32, 1024)
    ->UseRealTime();

Dataset makeEventWorkspace(const gsl::index nSpec, const gsl::index nEvent) {
  auto d = makeBeamline(nSpec / 100, nSpec);
  d.merge(makeSpectra(nSpec));

  d.insert<Coord::Tof>({Dimension::Tof, 2});

  d.insert<Data::Events>("events", {Dimension::Spectrum, nSpec});
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(0, nEvent);
  Dataset empty;
  empty.insert<Data::Tof>("", {Dimension::Event, 0});
  empty.insert<Data::PulseTime>("", {Dimension::Event, 0});
  for (auto &eventList : d.get<Data::Events>()) {
    // 1/4 of the event lists is empty
    gsl::index count = std::max(0l, dist(mt) - nEvent / 4);
    if (count == 0)
      eventList = empty;
    else {
      eventList.insert<Data::Tof>("", {Dimension::Event, count});
      eventList.insert<Data::PulseTime>("", {Dimension::Event, count});
    }
  }

  return d;
}

static void BM_Dataset_EventWorkspace_create(benchmark::State &state) {
  gsl::index nSpec = 1024 * 1024;
  gsl::index nEvent = 0;
  for (auto _ : state) {
    auto d = makeEventWorkspace(nSpec, nEvent);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_EventWorkspace_create);

static void BM_Dataset_EventWorkspace_copy(benchmark::State &state) {
  gsl::index nSpec = 1024 * 1024;
  gsl::index nEvent = 0;
  auto d = makeEventWorkspace(nSpec, nEvent);
  for (auto _ : state) {
    auto copy(d);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_EventWorkspace_copy);

static void BM_Dataset_EventWorkspace_copy_and_write(benchmark::State &state) {
  gsl::index nSpec = 1024 * 1024;
  gsl::index nEvent = state.range(0);
  auto d = makeEventWorkspace(nSpec, nEvent);
  for (auto _ : state) {
    auto copy(d);
    auto eventLists = copy.get<Data::Events>();
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_EventWorkspace_copy_and_write)
    ->RangeMultiplier(8)
    ->Range(2, 2 << 10);

static void BM_Dataset_EventWorkspace_plus(benchmark::State &state) {
  gsl::index nSpec = 128 * 1024;
  gsl::index nEvent = state.range(0);
  auto d = makeEventWorkspace(nSpec, nEvent);
  for (auto _ : state) {
    auto sum = d + d;
  }

  gsl::index actualEvents = 0;
  for (auto &eventList : d.get<const Data::Events>())
    actualEvents += eventList.dimensions().size(Dimension::Event);
  state.SetItemsProcessed(state.iterations());
  // 2 for Tof and PulseTime
  // 1+1+2+2 for loads and save
  state.SetBytesProcessed(state.iterations() * actualEvents * 2 * 6 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_EventWorkspace_plus)
    ->RangeMultiplier(2)
    ->Range(2, 2 << 12);

static void BM_Dataset_EventWorkspace_grow(benchmark::State &state) {
  gsl::index nSpec = 128 * 1024;
  gsl::index nEvent = state.range(0);
  auto d = makeEventWorkspace(nSpec, nEvent);
  auto update = makeEventWorkspace(nSpec, 100);
  for (auto _ : state) {
    state.PauseTiming();
    auto sum(d);
    static_cast<void>(sum.get<Data::Events>());
    state.ResumeTiming();
    sum += update;
  }

  gsl::index actualEvents = 0;
  for (auto &eventList : update.get<const Data::Events>())
    actualEvents += eventList.dimensions().size(Dimension::Event);
  state.SetItemsProcessed(state.iterations() * actualEvents);
}
BENCHMARK(BM_Dataset_EventWorkspace_grow)
    ->RangeMultiplier(2)
    ->Range(2, 2 << 13);

BENCHMARK_MAIN();
