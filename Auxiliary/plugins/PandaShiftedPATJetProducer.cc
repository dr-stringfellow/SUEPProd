/** \class SUEPShiftedPATJetProducer
 *
 * Replacement for PhysicsTools/PatUtils/plugins/ShiftedPATJetProducer, which wrongly uses
 * JetCorrExtractorT<pat::Jet> instead of PATJetCorrExtractor.
 */

#include "PhysicsTools/PatUtils/interface/ShiftedJetProducerT.h"
#include "PhysicsTools/PatUtils/interface/PATJetCorrExtractor.h"
#include "DataFormats/PatCandidates/interface/Jet.h"

typedef ShiftedJetProducerT<pat::Jet, PATJetCorrExtractor > SUEPShiftedPATJetProducer;

#include "FWCore/Framework/interface/MakerMacros.h"

DEFINE_FWK_MODULE(SUEPShiftedPATJetProducer);
