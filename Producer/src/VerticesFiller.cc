#include "../interface/VerticesFiller.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

VerticesFiller::VerticesFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  getToken_(verticesToken_, _cfg, _coll, "common", "vertices");
  getToken_(scoresToken_, _cfg, _coll, "common", "vertices");
  getToken_(candidatesToken_, _cfg, _coll, "common", "pfCandidates");

  if (!isRealData_) {
    getToken_(puSummariesToken_, _cfg, _coll, "puSummaries");
    getToken_(genParticlesToken_, _cfg, _coll, "common", "genParticles");
  }
}

void
VerticesFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("npv");
  _eventBranches.emplace_back("vertices");
  if (!isRealData_) {
    _eventBranches.emplace_back("npvTrue");
    _eventBranches.emplace_back("genVertex");
  }
}

void
VerticesFiller::addOutput(TFile& _outputFile)
{
  hNPVReco_ = new TH1D("hNPVReco", "N_{PV}^{reco}", 100, 0., 100.);
  hNPVReco_->SetDirectory(&_outputFile);
  if (!isRealData_) {
    hNPVTrue_ = new TH1D("hNPVTrue", "N_{PV}^{true}", 100, 0., 100.);
    hNPVTrue_->SetDirectory(&_outputFile);
  }
}

void
VerticesFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const&)
{
  auto& inVertices(getProduct_(_inEvent, verticesToken_));
  auto& inScores(getProduct_(_inEvent, scoresToken_));
  // assuming MINIAOD
  auto& inCandidates(getProduct_(_inEvent, candidatesToken_));

  auto& outVertices(_outEvent.vertices);
  outVertices.reserve(inVertices.size());

  auto& objMap(objectMap_->get<reco::Vertex, suep::RecoVertex>());

  _outEvent.npv = npvCache_;

  // if MINIAOD
  std::vector<unsigned> ntrkCounters(inVertices.size(), 0);
  for (auto& cand : inCandidates) {
    auto* inPacked(dynamic_cast<pat::PackedCandidate const*>(&cand));
    if (inPacked) {
      auto&& vtxRef(inPacked->vertexRef());
      if (vtxRef.isNonnull())
        ntrkCounters.at(vtxRef.key()) += 1;
    }
  }

  unsigned iVtx(0);
  for (auto& inVtx : inVertices) {
    auto& outVtx(outVertices.create_back());
    auto ptr(inVertices.ptrAt(iVtx));

    outVtx.x = inVtx.x();
    outVtx.y = inVtx.y();
    outVtx.z = inVtx.z();
    outVtx.score = inScores[ptr];
    outVtx.ndof = inVtx.ndof();
    outVtx.chi2 = inVtx.chi2();

    // if AOD
    // outVtx.ntrk = inVtx.tracksSize();
    // if MINIAOD
    outVtx.ntrk = ntrkCounters[iVtx];

    objMap.add(ptr, outVtx);

    ++iVtx;
  }

  if (!isRealData_) {
    auto& inGenParticles(getProduct_(_inEvent, genParticlesToken_));

    _outEvent.npvTrue = npvTrueCache_;

    for (auto& gen : inGenParticles) {
      if (std::abs(gen.vx()) > 1.e-6 || std::abs(gen.vy()) > 1.e-6 || std::abs(gen.vz()) > 1.e-6) {
        _outEvent.genVertex.x = gen.vx();
        _outEvent.genVertex.y = gen.vy();
        _outEvent.genVertex.z = gen.vz();
        break;
      }
    }
  }
}

void
VerticesFiller::fillAll(edm::Event const& _inEvent, edm::EventSetup const&)
{
  auto& inVertices(getProduct_(_inEvent, verticesToken_));

  npvCache_ = 0;
  for (auto& vtx : inVertices) {
    if (vtx.ndof() < 4)
      continue;
    if (std::abs(vtx.z()) > 24.)
      continue;
    if (vtx.position().rho() > 2.)
      continue;

    ++npvCache_;
  }

  hNPVReco_->Fill(npvCache_);

  if (!isRealData_) {
    auto& puSummaries(getProduct_(_inEvent, puSummariesToken_));
    for (auto& pu : puSummaries) {
      if (pu.getBunchCrossing() == 0) {
        npvTrueCache_ = pu.getTrueNumInteractions();
        hNPVTrue_->Fill(npvTrueCache_);
        break;
      }
    }
  }
}

DEFINE_TREEFILLER(VerticesFiller);
