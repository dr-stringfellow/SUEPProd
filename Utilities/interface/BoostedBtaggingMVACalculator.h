#ifndef PANDAPROD_NTUPLER_FUNCTIONS_BOOSTEDBTAGGINGMVACALCULATOR_HH
#define PANDAPROD_NTUPLER_FUNCTIONS_BOOSTEDBTAGGINGMVACALCULATOR_HH

#include <string>

// forward class declarations
namespace TMVA {
  class Reader;
}

namespace suep {

  class BoostedBtaggingMVACalculator
  {
    public:
      
      BoostedBtaggingMVACalculator();
      ~BoostedBtaggingMVACalculator();
      
      void initialize(
                      const std::string MethodTag, const std::string WeightFile);
      
      bool isInitialized() const {return fIsInitialized;}
      
      float mvaValue(
	 	     		     const float massPruned, const float flavour, const float nbHadrons, const float ptPruned, const float etaPruned,
                                     const float SubJet_csv,const float z_ratio, const float trackSipdSig_3, const float trackSipdSig_2, const float trackSipdSig_1,
                                     const float trackSipdSig_0, const float trackSipdSig_1_0, const float trackSipdSig_0_0, const float trackSipdSig_1_1,
                                     const float trackSipdSig_0_1, const float trackSip2dSigAboveCharm_0, const float trackSip2dSigAboveBottom_0,
                                     const float trackSip2dSigAboveBottom_1, const float tau0_trackEtaRel_0, const float tau0_trackEtaRel_1, const float tau0_trackEtaRel_2,
                                     const float tau1_trackEtaRel_0, const float tau1_trackEtaRel_1, const float tau1_trackEtaRel_2, const float tau_vertexMass_0,
                                     const float tau_vertexEnergyRatio_0, const float tau_vertexDeltaR_0, const float tau_flightDistance2dSig_0, const float tau_vertexMass_1,
                                     const float tau_vertexEnergyRatio_1, const float tau_flightDistance2dSig_1, const float jetNTracks, const float nSV,
		     		     const bool printDebug=false);
     
    
    private:
      void initReader(TMVA::Reader *reader, const std::string filename);
      
      bool fIsInitialized;
      
      TMVA::Reader *fReader;
      std::string fMethodTag;
      // input variables to compute MVA value
      //
      float _SubJet_csv;
			float  _z_ratio ;
			float  _trackSipdSig_3 ;
			float _trackSipdSig_2;
			float _trackSipdSig_1;
			float _trackSipdSig_0;
			float _trackSipdSig_1_0;
			float _trackSipdSig_0_0;
			float _trackSipdSig_1_1;
			float _trackSipdSig_0_1;
			float _trackSip2dSigAboveCharm_0;
			float _trackSip2dSigAboveBottom_0;
			float _trackSip2dSigAboveBottom_1;
			float _tau0_trackEtaRel_0;
			float _tau0_trackEtaRel_1;
			float _tau0_trackEtaRel_2;
			float _tau1_trackEtaRel_0;
			float _tau1_trackEtaRel_1;
			float _tau1_trackEtaRel_2;
			float _tau_vertexMass_0;
			float _tau_vertexEnergyRatio_0;
			float _tau_vertexDeltaR_0;
			float _tau_flightDistance2dSig_0;
			float _tau_vertexMass_1;
			float _tau_vertexEnergyRatio_1;
			float _tau_flightDistance2dSig_1;
			float _massPruned;
			float _flavour;
			float _ptPruned;
			float _etaPruned;
      float _jetNTracks;
			float _nSV;
		  float	_nbHadrons;
  };
}
#endif
