// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/METReco/interface/PFMETCollection.h"

#include "SUEPTree/Objects/interface/Recoil.h"

//
// class declaration
//

class MonoXFilter : public edm::EDFilter {
public:
  explicit MonoXFilter(const edm::ParameterSet&);
  ~MonoXFilter();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  virtual bool filter(edm::Event&, const edm::EventSetup&) override;
  bool isGoodMuon(const pat::Muon&);
  bool isGoodElectron(const pat::Electron&);
  bool isGoodPhoton(const pat::Photon&);

  edm::EDGetTokenT<pat::METCollection> met_token ;
  edm::Handle<pat::METCollection> met_handle;

  edm::EDGetTokenT<pat::METCollection> puppimet_token;
  edm::Handle<pat::METCollection> puppimet_handle;

  edm::EDGetTokenT<pat::MuonCollection> mu_token;
  edm::Handle<pat::MuonCollection> mu_handle;

  edm::EDGetTokenT<pat::ElectronCollection> el_token;
  edm::Handle<pat::ElectronCollection> el_handle;

  edm::EDGetTokenT<pat::PhotonCollection> ph_token;
  edm::Handle<pat::PhotonCollection> ph_handle;

  double minU;
  bool saveWlv;
  bool saveZll;
  bool savePho;
  bool taggingMode;
};

MonoXFilter::MonoXFilter(const edm::ParameterSet& iConfig):
  met_token( consumes<pat::METCollection>(iConfig.getParameter<edm::InputTag>("met")) ),
  puppimet_token( consumes<pat::METCollection>(iConfig.getParameter<edm::InputTag>("puppimet")) ),
  mu_token(  consumes<pat::MuonCollection>(iConfig.getParameter<edm::InputTag>("muons")) ),
  el_token(  consumes<pat::ElectronCollection>(iConfig.getParameter<edm::InputTag>("electrons")) ),
  ph_token(  consumes<pat::PhotonCollection>(iConfig.getParameter<edm::InputTag>("photons")) ),
  minU(   iConfig.getParameter<double>("minU")),
  saveWlv(iConfig.getParameter<bool>("saveWlv")),
  saveZll(iConfig.getParameter<bool>("saveZll")),
  savePho(iConfig.getParameter<bool>("savePho")),
  taggingMode(iConfig.getParameter<bool>("taggingMode"))
{
  produces<int>("categories");
  produces<double>("max");
}


MonoXFilter::~MonoXFilter()
{
}

bool MonoXFilter::isGoodMuon(const pat::Muon& muon){
  if (muon.pt()<15) return false;
  if (not muon.isLooseMuon()) return false;
  if (muon.pfIsolationR04().sumChargedHadronPt/muon.pt()>0.12) return false;
  return true;
}

bool MonoXFilter::isGoodElectron(const pat::Electron& ele){
  if (ele.pt()<15) return false;
  if (ele.chargedHadronIso()/ele.pt()>0.1) return false;
  bool isEB = ele.isEB();
  bool isEE = ele.isEE();
  if (isEB and ele.full5x5_sigmaIetaIeta()>0.011) return false;
  if (isEE and ele.full5x5_sigmaIetaIeta()>0.030) return false;
  return true;
}

bool MonoXFilter::isGoodPhoton(const pat::Photon& photon){
  if (photon.chargedHadronIso()>10) return false;
  if (not photon.isEB()) return false;
  if (photon.full5x5_sigmaIetaIeta()>0.020) return false;
  return true;
}


bool
MonoXFilter::filter(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;
  iEvent.getByToken(met_token, met_handle);
  iEvent.getByToken(puppimet_token, puppimet_handle);
  iEvent.getByToken(mu_token, mu_handle);
  iEvent.getByToken(el_token, el_handle);
  iEvent.getByToken(ph_token, ph_handle);

  int categories(0);
  double maxRecoil(0.);

  std::vector<reco::Candidate::LorentzVector> mets;
  for (auto met = met_handle->begin(); met!=met_handle->end(); met++){
    mets.push_back(met->p4());
  }

  for (auto met = puppimet_handle->begin(); met!=puppimet_handle->end(); met++) {
    mets.push_back(met->p4());
  }

  for (auto &met : mets) {
    // if MET is big enough keep the event
    if (met.pt()>minU) {
      categories |= (1 << suep::Recoil::rMET);
      if (met.pt() > maxRecoil)
        maxRecoil = met.pt();
    }

    if (saveWlv){
      // loop over leptons to get W+jets events
      for (pat::MuonCollection::const_iterator muon = mu_handle->begin(); muon!=mu_handle->end(); muon++){
        if (not isGoodMuon(*muon)) continue;
        double recoil((met+muon->p4()).pt());
        if (recoil>minU) {
          categories |= (1 << suep::Recoil::rMonoMu);
          if (recoil > maxRecoil)
            maxRecoil = recoil;
          break;
        }
      }
      for (pat::ElectronCollection::const_iterator ele = el_handle->begin(); ele!=el_handle->end(); ele++){
        if (not isGoodElectron(*ele)) continue;
        double recoil((met+ele->p4()).pt());
        if (recoil>minU) {
          categories |= (1 << suep::Recoil::rMonoE);
          if (recoil > maxRecoil)
            maxRecoil = recoil;
          break;
        }
      }
    }

    if (saveZll){
      // loop over dilepton pairs
      for (unsigned int imuon=0; imuon+1<mu_handle->size(); imuon++){
        if (not isGoodMuon(mu_handle->at(imuon))) continue;
        for (unsigned int jmuon=imuon+1; jmuon<mu_handle->size(); jmuon++){
          if (not isGoodMuon(mu_handle->at(jmuon))) continue;
          double recoil((met+mu_handle->at(imuon).p4()+mu_handle->at(jmuon).p4()).pt());
          if (recoil>minU) {
            categories |= (1 << suep::Recoil::rDiMu);
            if (recoil > maxRecoil)
              maxRecoil = recoil;
            break;
          }
        }
        if ((categories & 8) != 0)
          break;
      }
      for (unsigned int iele=0; iele+1<el_handle->size(); iele++){
        if (not isGoodElectron(el_handle->at(iele))) continue;
        for (unsigned int jele=iele+1; jele<el_handle->size(); jele++){
          if (not isGoodElectron(el_handle->at(jele))) continue;
          double recoil((met+el_handle->at(iele).p4()+el_handle->at(jele).p4()).pt());
          if (recoil>minU) {
            categories |= (1 << suep::Recoil::rDiE);
            if (recoil > maxRecoil)
              maxRecoil = recoil;
            break;
          }
        }
        if ((categories & 16) != 0)
          break;
      }
    }

    if (savePho){
      for (unsigned int ipho=0; ipho<ph_handle->size(); ipho++){
        if (not isGoodPhoton(ph_handle->at(ipho))) continue;
        double recoil((met+ph_handle->at(ipho).p4()).pt());
        if (recoil>minU) {
          categories |= (1 << suep::Recoil::rGamma);
          if (recoil > maxRecoil)
            maxRecoil = recoil;
          break;
        }
      }
    }
  }

  auto categoriesP = std::make_unique<int>(categories);
  auto maxP = std::make_unique<double>(maxRecoil);

  iEvent.put(std::move(categoriesP), "categories");
  iEvent.put(std::move(maxP), "max");

  // PInfo("SUEPProd::MonoXFilter::analyze",
  //     TString::Format("Rejecting event %llu",iEvent.id().event()));
  return taggingMode || categories != 0;
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
MonoXFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(MonoXFilter);
