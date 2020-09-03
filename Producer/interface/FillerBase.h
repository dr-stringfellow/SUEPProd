#ifndef SUEPProd_Producer_Filler_h
#define SUEPProd_Producer_Filler_h

#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Candidate/interface/Candidate.h"

#include "SUEPTree/Objects/interface/Event.h"
#include "SUEPTree/Objects/interface/Run.h"
#include "ObjectMap.h"

#include "TFile.h"

#include "tbb/concurrent_unordered_map.h"

typedef std::vector<std::string> VString;
typedef std::vector<std::vector<std::string>> VVString;

//! Base class for tree fillers
/*!
 * There is no strict (programmatic) rule on the scope of each Filler. One basic guideline is to
 * define one Filler per object branch (collection etc.) of the Event.
 */
class FillerBase {
 public:
  FillerBase(std::string const& fillerName, edm::ParameterSet const&);
  virtual ~FillerBase() {}

  //! Add names of branches the filler wants to book. If nothing is specified, all branches are booked.
  virtual void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList& runBranches) const {}
  //! Override when the filler writes additional objects to the output file
  virtual void addOutput(TFile&) {}
  //! Main function
  virtual void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) = 0;
  //! Set references
  virtual void setRefs(ObjectMapStore const&) {}
  //! Fill "all events" information (guaranteed write regardless of skims)
  virtual void fillAll(edm::Event const&, edm::EventSetup const&) {}
  //! Fill the run tree
  virtual void fillBeginRun(suep::Run&, edm::Run const&, edm::EventSetup const&) {}
  //! Fill the run tree
  virtual void fillEndRun(suep::Run&, edm::Run const&, edm::EventSetup const&) {}
  //! Called (indirectly) by CMSSW framework whenever a new product is registered to Event
  virtual void notifyNewProduct(edm::BranchDescription const&, edm::ConsumesCollector&) {}

  std::string const& getName() const { return fillerName_; }
  bool enabled() const { return enabled_; }
  void setObjectMap(FillerObjectMap& map) { objectMap_ = &map; }

 private:
  std::string const fillerName_;
  bool const enabled_;

 protected:
  template <class Product>
  using NamedToken = std::pair<std::string, edm::EDGetTokenT<Product>>;

  //! get a parameter in the "global" (i.e. PSet for SUEPProducer) scope. Use dots to descend into sub-PSets
  template<class T>
  static T getGlobalParameter_(edm::ParameterSet const&, std::string const&);
  //! get a parameter in the "global" (i.e. PSet for SUEPProducer) scope. Use dots to descend into sub-PSets
  template<class T>
  static T getGlobalParameter_(edm::ParameterSet const&, std::string const&, T const&);

  //! shortcut to a filler-specific parameter.
  template<class T>
  static T getFillerParameter_(edm::ParameterSet const& cfg, std::string const& fname, std::string const& pname)
  { return getGlobalParameter_<T>(cfg, "fillers." + fname + "." + pname); }
  //! shortcut to a filler-specific parameter.
  template<class T>
  static T getFillerParameter_(edm::ParameterSet const& cfg, std::string const& fname, std::string const& pname, T const& d)
  { return getGlobalParameter_<T>(cfg, "fillers." + fname + "." + pname, d); }

  //! parameter for this filler module
  template<class T>
  T getParameter_(edm::ParameterSet const& cfg, std::string const& pname) const
  { return getFillerParameter_<T>(cfg, getName(), pname); }
  //! parameter for this filler module
  template<class T>
  T getParameter_(edm::ParameterSet const& cfg, std::string const& pname, T const& d) const
  { return getFillerParameter_<T>(cfg, getName(), pname, d); }

  //! get a token from a tag in this module's configuration
  template<class Product>
  void getToken_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InEvent>(token, cfg, coll, getName(), pname, mandatory); }
  //! get a token from a tag in the configuration of a module
  template<class Product>
  void getToken_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& fname, std::string const& pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InEvent>(token, cfg, coll, fname, pname, mandatory); }
  //! necessary overload to enable the second version when passed a c string
  template<class Product>
  void getToken_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& fname, char const* pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InEvent>(token, cfg, coll, fname, pname, mandatory); }

  //! get a token from a tag in this module's configuration
  template<class Product>
  void getTokenLumi_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InLumi>(token, cfg, coll, getName(), pname, mandatory); }
  //! get a token from a tag in the configuration of a module
  template<class Product>
  void getTokenLumi_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& fname, std::string const& pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InLumi>(token, cfg, coll, fname, pname, mandatory); }
  //! necessary overload to enable the second version when passed a c string
  template<class Product>
  void getTokenLumi_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& fname, char const* pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InLumi>(token, cfg, coll, fname, pname, mandatory); }

  //! get a token from a tag in this module's configuration
  template<class Product>
  void getTokenRun_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InRun>(token, cfg, coll, getName(), pname, mandatory); }
  //! get a token from a tag in the configuration of a module
  template<class Product>
  void getTokenRun_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& fname, std::string const& pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InRun>(token, cfg, coll, fname, pname, mandatory); }
  //! necessary overload to enable the second version when passed a c string
  template<class Product>
  void getTokenRun_(NamedToken<Product>& token, edm::ParameterSet const& cfg, edm::ConsumesCollector& coll, std::string const& fname, char const* pname, bool mandatory = true)
  { getTokenImpl_<Product, edm::InRun>(token, cfg, coll, fname, pname, mandatory); }

  //! get a token from a tag in the configuration of a module
  template<class Product, edm::BranchType B>
  void getTokenImpl_(NamedToken<Product>&, edm::ParameterSet const&, edm::ConsumesCollector&, std::string const& fname, std::string const& pname, bool mandatory = true);

  //! get a product from the Event or Run.
  template<class Principal, class Product>
  Product const& getProduct_(Principal const&, NamedToken<Product> const&, edm::Handle<Product>* = 0);
  //! get a product from the Event or Run. Return a null pointer if the product does not exist
  template<class Principal, class Product>
  Product const* getProductSafe_(Principal const&, NamedToken<Product> const&, edm::Handle<Product>* = 0);

  FillerObjectMap* objectMap_{0};

  bool isRealData_;
  bool useTrigger_;
};

template<class T>
/*static*/
T
FillerBase::getGlobalParameter_(edm::ParameterSet const& _cfg, std::string const& _pname)
{
  edm::ParameterSet const* pset(&_cfg);

  size_t begin(0);
  size_t end(std::string::npos);
  while ((end = _pname.find('.', begin)) != std::string::npos) {
    pset = &pset->getUntrackedParameterSet(_pname.substr(begin, end - begin));
    begin = end + 1;
  }

  return pset->getUntrackedParameter<T>(_pname.substr(begin, end));
}

template<class T>
/*static*/
T
FillerBase::getGlobalParameter_(edm::ParameterSet const& _cfg, std::string const& _pname, T const& _default)
{
  edm::ParameterSet const* pset(&_cfg);

  size_t begin(0);
  size_t end(std::string::npos);
  while ((end = _pname.find('.', begin)) != std::string::npos) {
    pset = &pset->getUntrackedParameterSet(_pname.substr(begin, end - begin));
    begin = end + 1;
  }

  return pset->getUntrackedParameter<T>(_pname.substr(begin, end), _default);
}

template<class Product, edm::BranchType B>
void
FillerBase::getTokenImpl_(NamedToken<Product>& _token, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll, std::string const& _fname, std::string const& _pname, bool _mandatory/* = true*/)
{
  _token.first = _pname;

  std::string paramValue(getGlobalParameter_<std::string>(_cfg, "fillers." + _fname + "." + _pname, ""));

  if (paramValue.empty()) {
    if (_mandatory)
      throw edm::Exception(edm::errors::Configuration, getName() + "::getToken_()")
        << "Missing or empty parameter fillers." << _fname << "." << _pname;
    else
      _token.second = edm::EDGetTokenT<Product>();
  }
  else 
    _token.second = _coll.consumes<Product, B>(edm::InputTag(paramValue));
}

template<class Principal, class Product>
Product const&
FillerBase::getProduct_(Principal const& _prn, NamedToken<Product> const& _token, edm::Handle<Product>* _handle/* = 0*/)
{
  edm::Handle<Product> handle;
  edm::Handle<Product>* handlePtr(_handle);
  if (!handlePtr)
    handlePtr = &handle;

  if (!_prn.getByToken(_token.second, *handlePtr))
    throw cms::Exception("ProductNotFound") << "fillers." << getName() << "." << _token.first;

  return **handlePtr;
}

template<class Principal, class Product>
Product const*
FillerBase::getProductSafe_(Principal const& _prn, NamedToken<Product> const& _token, edm::Handle<Product>* _handle/* = 0*/)
{
  if (_token.second.isUninitialized())
    return 0;

  edm::Handle<Product> handle;
  edm::Handle<Product>* handlePtr(_handle);
  if (!handlePtr)
    handlePtr = &handle;

  if (!_prn.getByToken(_token.second, *handlePtr))
    return 0;

  return handlePtr->product();
}

void fillP4(suep::Particle&, reco::Candidate const&);

//--------------------------------------------------------------------------------------------------
// FillerFactory: a trick to register individual fillers as "plugin modules"
//--------------------------------------------------------------------------------------------------

typedef std::function<FillerBase*(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&)> FillerFactory;
            //typedef FillerBase* (*FillerFactory)(std::string const& name, edm::ParameterSet const&, edm::ConsumesCollector&);

// A singleton class to store information of the filler plugins
class FillerFactoryStore {
 public:
  // A utility class whose instantiation triggers the registration of a filler plugin
  template<class Filler>
  struct Registration {
    Registration(char const* _name)
    {
      singleton()->registerFactory(_name,
      [](std::string const& name, edm::ParameterSet const& cfg, edm::ConsumesCollector& collector)->FillerBase*
      {
        return new Filler(name, cfg, collector);
      });
    }
  };

  // Register a FillerFactory (filler generator function) under a given name
  void registerFactory(std::string const& _name, FillerFactory _f) { fillerFactories_[_name] = _f; }

  // Retrieve the FillerFactory and instantiate the filler
  FillerBase* makeFiller(std::string const& className, std::string const& fillerName, edm::ParameterSet const&, edm::ConsumesCollector&) const;

  static FillerFactoryStore* singleton();

 protected:
  tbb::concurrent_unordered_map<std::string, FillerFactory> fillerFactories_;
};

// A macro that instantiates FillerFactoryStore::Registration for the class FILLER
#define DEFINE_TREEFILLER(FILLER) \
  FillerFactoryStore::Registration<FILLER> suep##FILLER##Registration(#FILLER)

#endif
