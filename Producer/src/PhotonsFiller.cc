#include "../interface/PhotonsFiller.h"

#include "RecoEcal/EgammaCoreTools/interface/EcalClusterLazyTools.h"
#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/Common/interface/RefToPtr.h"
#include "DataFormats/Math/interface/deltaR.h"

#include <cmath>

PhotonsFiller::PhotonsFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg),
  chIsoEA_(edm::FileInPath(getParameter_<std::string>(_cfg, "chIsoEA")).fullPath()),
  nhIsoEA_(edm::FileInPath(getParameter_<std::string>(_cfg, "nhIsoEA")).fullPath()),
  phIsoEA_(edm::FileInPath(getParameter_<std::string>(_cfg, "phIsoEA")).fullPath())
{
  getToken_(photonsToken_, _cfg, _coll, "photons");
  getToken_(smearedPhotonsToken_, _cfg, _coll, "smearedPhotons", false);
  getToken_(regressionPhotonsToken_, _cfg, _coll, "regressionPhotons", false);
  getToken_(pfCandidatesToken_, _cfg, _coll, "common", "pfCandidates");
  getToken_(ebHitsToken_, _cfg, _coll, "common", "ebHits");
  getToken_(eeHitsToken_, _cfg, _coll, "common", "eeHits");
  getToken_(looseIdToken_, _cfg, _coll, "looseId");
  getToken_(mediumIdToken_, _cfg, _coll, "mediumId");
  getToken_(tightIdToken_, _cfg, _coll, "tightId");
  getToken_(chIsoToken_, _cfg, _coll, "chIso");
  getToken_(nhIsoToken_, _cfg, _coll, "nhIso");
  getToken_(phIsoToken_, _cfg, _coll, "phIso");
  getToken_(chIsoMaxToken_, _cfg, _coll, "chIsoMax");
  getToken_(rhoToken_, _cfg, _coll, "rho", "rho");

  chIsoLeakage_[0].Compile(getParameter_<std::string>(_cfg, "chIsoLeakage.EB", "").c_str());
  chIsoLeakage_[1].Compile(getParameter_<std::string>(_cfg, "chIsoLeakage.EE", "").c_str());
  nhIsoLeakage_[0].Compile(getParameter_<std::string>(_cfg, "nhIsoLeakage.EB", "").c_str());
  nhIsoLeakage_[1].Compile(getParameter_<std::string>(_cfg, "nhIsoLeakage.EE", "").c_str());
  phIsoLeakage_[0].Compile(getParameter_<std::string>(_cfg, "phIsoLeakage.EB", "").c_str());
  phIsoLeakage_[1].Compile(getParameter_<std::string>(_cfg, "phIsoLeakage.EE", "").c_str());
}

void
PhotonsFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("photons");

  if (isRealData_)
    _eventBranches += {"!photons.matchedGen_"};
}

void
PhotonsFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  auto& inPhotons(getProduct_(_inEvent, photonsToken_));
  auto* inSmearedPhotons(getProductSafe_(_inEvent, smearedPhotonsToken_));
  auto* inRegressionPhotons(getProductSafe_(_inEvent, regressionPhotonsToken_));
  auto& pfCandidates(getProduct_(_inEvent, pfCandidatesToken_));
  auto& ebHits(getProduct_(_inEvent, ebHitsToken_));
  auto& eeHits(getProduct_(_inEvent, eeHitsToken_));
  auto& looseId(getProduct_(_inEvent, looseIdToken_));
  auto& mediumId(getProduct_(_inEvent, mediumIdToken_));
  auto& tightId(getProduct_(_inEvent, tightIdToken_));
  auto& chIso(getProduct_(_inEvent, chIsoToken_));
  auto& nhIso(getProduct_(_inEvent, nhIsoToken_));
  auto& phIso(getProduct_(_inEvent, phIsoToken_));
  auto& chIsoMax(getProduct_(_inEvent, chIsoMaxToken_));
  double rho(getProduct_(_inEvent, rhoToken_));

  auto findHit([&ebHits, &eeHits](DetId const& id)->EcalRecHit const* {
      EcalRecHitCollection const* hits(0);
      if (id.subdetId() == EcalBarrel)
        hits = &ebHits;
      else
        hits = &eeHits;

      auto&& hitItr(hits->find(id));
      if (hitItr == hits->end())
        return 0;

      return &*hitItr;
    });

  auto findPF([&pfCandidates](reco::Photon const& inPhoton)->reco::CandidatePtr {
      int iMatch(-1);

      double minDR(0.1);
      for (unsigned iPF(0); iPF != pfCandidates.size(); ++iPF) {
        auto& pf(pfCandidates.at(iPF));
        if (std::abs(pf.pdgId()) != 11 || pf.pdgId() == 22)
          continue;

        double dR(reco::deltaR(pf, inPhoton));
        if (dR < minDR) {
          minDR = dR;
          iMatch = iPF;
        }
      }

      if (iMatch >= 0)
        return pfCandidates.ptrAt(iMatch);
      else
        return reco::CandidatePtr();
    });

  std::vector<reco::Candidate const*> chargedPFCandidates;
  for (auto& cand : pfCandidates) {
    if (cand.charge() != 0)
      chargedPFCandidates.emplace_back(&cand);
  }

  noZS::EcalClusterLazyTools lazyTools(_inEvent, _setup, ebHitsToken_.second, eeHitsToken_.second);

  auto& outPhotons(_outEvent.photons);

  std::vector<edm::Ptr<reco::Photon>> ptrList;

  unsigned iPh(-1);
  for (auto& inPhoton : inPhotons) {
    ++iPh;
    auto&& inRef(inPhotons.refAt(iPh));

    bool isPAT(dynamic_cast<pat::Photon const*>(&inPhoton));

    auto&& scRef(inPhoton.superCluster());
    auto& sc(*scRef);
    double scRawPt(sc.rawEnergy() / std::cosh(sc.eta()));

    auto& outPhoton(outPhotons.create_back());

    fillP4(outPhoton, inPhoton);

    double scEta(std::abs(sc.eta()));

    unsigned iDet(scEta < 1.4442 ? 0 : 1);

    outPhoton.sieie = inPhoton.full5x5_sigmaIetaIeta();
    outPhoton.sipip = inPhoton.full5x5_showerShapeVariables().sigmaIphiIphi;
    outPhoton.hOverE = inPhoton.hadTowOverEm();

    outPhoton.chIso = chIso[inRef] - chIsoEA_.getEffectiveArea(scEta) * rho;
    if (chIsoLeakage_[iDet].IsValid())
      outPhoton.chIso -= chIsoLeakage_[iDet].Eval(outPhoton.pt());
    outPhoton.nhIso = nhIso[inRef] - nhIsoEA_.getEffectiveArea(scEta) * rho;
    if (nhIsoLeakage_[iDet].IsValid())
      outPhoton.nhIso -= nhIsoLeakage_[iDet].Eval(outPhoton.pt());
    outPhoton.phIso = phIso[inRef] - phIsoEA_.getEffectiveArea(scEta) * rho;
    if (phIsoLeakage_[iDet].IsValid())
      outPhoton.phIso -= phIsoLeakage_[iDet].Eval(outPhoton.pt());
    outPhoton.chIsoMax = chIsoMax[inRef];

    // backward compatibility
    outPhoton.loose = looseId[inRef];
    outPhoton.medium = mediumId[inRef];
    outPhoton.tight = tightId[inRef];
    // Effective area hard-coded!!
    double highptEA(0.);
    if (scEta < 0.9)
      highptEA = 0.17;
    else if (scEta < 1.479)
      highptEA = 0.14;
    else if (scEta < 2.)
      highptEA = 0.11;
    else if (scEta < 2.2)
      highptEA = 0.14;
    else
      highptEA = 0.22;

    if (scEta < 1.479)
      outPhoton.highpt = outPhoton.hOverE < 0.05 &&
        outPhoton.sieie < 0.0105 &&
        chIso[inRef] < 5. &&
        phIso[inRef] + 0.0045 * outPhoton.pt() - highptEA * rho < 5.25;
    else if (scEta < 2.)
      outPhoton.highpt = outPhoton.hOverE < 0.05 &&
        outPhoton.sieie < 0.028 &&
        chIso[inRef] < 5. &&
        phIso[inRef] + 0.0045 * outPhoton.pt() - highptEA * rho < 4.5;
    else
      outPhoton.highpt = outPhoton.hOverE < 0.05 &&
        outPhoton.sieie < 0.028 &&
        chIso[inRef] < 5. &&
        phIso[inRef] + 0.003 * outPhoton.pt() - highptEA * rho < 4.5;

    outPhoton.pixelVeto = !inPhoton.hasPixelSeed();
    if (isPAT)
      outPhoton.csafeVeto = static_cast<pat::Photon const&>(inPhoton).passElectronVeto();

    outPhoton.pfchVeto = true;
    for (auto* cand : chargedPFCandidates) {
      if (reco::deltaR(*cand, inPhoton) > 0.1)
        continue;

      if (cand->pt() / scRawPt > 0.6) {
        outPhoton.pfchVeto = false;
        break;
      }
    }

    outPhoton.mipEnergy = inPhoton.mipTotEnergy();

    outPhoton.r9 = inPhoton.r9();

    outPhoton.etaWidth = sc.etaWidth();
    outPhoton.phiWidth = sc.phiWidth();

    auto&& seedRef(sc.seed());
    if (seedRef.isNonnull()) {
      auto& seed(*seedRef);
      outPhoton.emax = lazyTools.eMax(seed);
      outPhoton.e2nd = lazyTools.e2nd(seed);
      outPhoton.eleft = lazyTools.eLeft(seed);
      outPhoton.eright = lazyTools.eRight(seed);
      outPhoton.etop = lazyTools.eTop(seed);
      outPhoton.ebottom = lazyTools.eBottom(seed);
      
      auto&& seedId(seed.seed());
      auto* seedHit(findHit(seedId));
      if (seedHit) {
        outPhoton.time = seedHit->time();

        if (seedId.subdetId() == EcalBarrel) {
          EBDetId ebid(seedId);
          outPhoton.ix = ebid.ietaAbs();
          outPhoton.iy = ebid.iphi();
        }
        else {
          EEDetId eeid(seedId);
          outPhoton.ix = eeid.ix();
          outPhoton.iy = eeid.iy();
        }
      }        
      else {
        outPhoton.time = -50.;
        outPhoton.ix = 0;
        outPhoton.iy = 0;
      }
    }

    outPhoton.timeSpan = 0.;    
    for (auto& hf : sc.hitsAndFractions()) {
      auto* hit(findHit(hf.first));
      if (!hit || hit->energy() < 1.)
        continue;

      double dt(outPhoton.time - hit->time());
      if (std::abs(dt) > std::abs(outPhoton.timeSpan))
        outPhoton.timeSpan = dt;
    }

    reco::CandidatePtr matchedPF(findPF(inPhoton));

    if (matchedPF.isNonnull())
      outPhoton.pfPt = matchedPF->pt();

    if (inSmearedPhotons) {
      for (auto& smeared : *inSmearedPhotons) {
        if (smeared.superCluster() == scRef) {
          outPhoton.smearedPt = smeared.pt();
          break;
        }
      }
    }

    if (inRegressionPhotons) {
      for (auto& reg : *inRegressionPhotons) {
        if (reg.superCluster() == scRef) {
          outPhoton.regPt = reg.pt();
          break;
        }
      }
    }

    ptrList.push_back(inPhotons.ptrAt(iPh));
  }

  auto originalIndices(outPhotons.sort(suep::Particle::PtGreater));

  // make reco <-> suep mapping
  auto& phoPhoMap(objectMap_->get<reco::Photon, suep::Photon>());
  auto& scPhoMap(objectMap_->get<reco::SuperCluster, suep::Photon>());
  auto& pfPhoMap(objectMap_->get<reco::Candidate, suep::Photon>("pf"));
  auto& genPhoMap(objectMap_->get<reco::Candidate, suep::Photon>("gen"));
  
  for (unsigned iP(0); iP != outPhotons.size(); ++iP) {
    auto& outPhoton(outPhotons[iP]);
    unsigned idx(originalIndices[iP]);
    phoPhoMap.add(ptrList[idx], outPhoton);
    scPhoMap.add(edm::refToPtr(ptrList[idx]->superCluster()), outPhoton);

    reco::CandidatePtr matchedPF(findPF(*ptrList[idx]));
    if (matchedPF.isNonnull())
      pfPhoMap.add(matchedPF, outPhoton);

    if (!isRealData_) {
      auto& inPhoton(*ptrList[idx]);

      if (dynamic_cast<pat::Photon const*>(&inPhoton)) {
        auto& patPhoton(static_cast<pat::Photon const&>(inPhoton));
        // PAT gen matching - will match to a photon even in case of a lepton FSR
        auto ref(patPhoton.genParticleRef());
        if (ref.isNonnull())
          genPhoMap.add(edm::refToPtr(ref), outPhoton);
      }
    }
  }
}

void
PhotonsFiller::setRefs(ObjectMapStore const& _objectMaps)
{
  auto& scPhoMap(objectMap_->get<reco::SuperCluster, suep::Photon>().bwdMap);
  auto& pfPhoMap(objectMap_->get<reco::Candidate, suep::Photon>("pf"));

  auto& scMap(_objectMaps.at("superClusters").get<reco::SuperCluster, suep::SuperCluster>().fwdMap);
  auto& pfMap(_objectMaps.at("pfCandidates").get<reco::Candidate, suep::PFCand>().fwdMap);

  for (auto& link : scPhoMap) { // suep -> edm
    auto& outPhoton(*link.first);
    auto& scPtr(link.second);

    outPhoton.superCluster.setRef(scMap.at(scPtr));
  }

  for (auto& link : pfPhoMap.bwdMap) { // suep -> edm
    auto& outPhoton(*link.first);
    auto& pfPtr(link.second);

    outPhoton.matchedPF.setRef(pfMap.at(pfPtr));
  }

  if (!isRealData_) {
    auto& genPhoMap(objectMap_->get<reco::Candidate, suep::Photon>("gen"));

    auto& genMap(_objectMaps.at("genParticles").get<reco::Candidate, suep::GenParticle>().fwdMap);

    for (auto& link : genPhoMap.bwdMap) {
      auto& genPtr(link.second);
      if (genMap.find(genPtr) == genMap.end())
        continue;

      auto& outPhoton(*link.first);
      outPhoton.matchedGen.setRef(genMap.at(genPtr));
    }
  }
}

DEFINE_TREEFILLER(PhotonsFiller);
