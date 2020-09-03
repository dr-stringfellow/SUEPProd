#include "../interface/FillerBase.h"

FillerBase::FillerBase(std::string const& _fillerName, edm::ParameterSet const& _cfg) :
  fillerName_(_fillerName),
  enabled_(getParameter_<bool>(_cfg, "enabled")),
  isRealData_(getGlobalParameter_<bool>(_cfg, "isRealData")),
  useTrigger_(getGlobalParameter_<bool>(_cfg, "useTrigger"))
{
}

void
fillP4(suep::Particle& _out, reco::Candidate const& _in)
{
  _out.setPtEtaPhiM(_in.pt(), _in.eta(), _in.phi(), _in.mass());
}

FillerBase*
FillerFactoryStore::makeFiller(std::string const& _className, std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _collector) const
{
  try {
    return fillerFactories_.at(_className)(_name, _cfg, _collector);
  }
  catch (std::out_of_range& ex) {
    return 0;
  }
}

/*static*/
FillerFactoryStore*
FillerFactoryStore::singleton()
{
  static FillerFactoryStore fillerFactoryStore;
  return &fillerFactoryStore;
}
