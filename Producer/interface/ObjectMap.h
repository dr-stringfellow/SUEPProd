#ifndef SUEPProd_Producer_ObjectMap_h
#define SUEPProd_Producer_ObjectMap_h

#include "DataFormats/Common/interface/Ptr.h"
#include "SUEPTree/Framework/interface/Object.h"

#include <map>
#include <string>
#include <tuple>
#include <typeinfo>

//! Abstract base to handle ObjectMaps for different types in a single container
class ObjectMapBase {
 public:
  typedef std::tuple<size_t, size_t, std::string> MapId;

  ObjectMapBase(std::string const& _label = "") : label(_label) {}
  virtual ~ObjectMapBase() {}
  virtual void clear() {};
  virtual MapId getId() const { return MapId(); };

  std::string label;
};

//! Actual EDM <-> suep map
template<class EDM, class PANDA>
class ObjectMap : public ObjectMapBase {
  typedef edm::Ptr<EDM> EDMPtr;

 public:
  std::map<EDMPtr, PANDA*> fwdMap;
  std::map<PANDA*, EDMPtr> bwdMap;

  void clear() override { fwdMap.clear(); bwdMap.clear(); }
  MapId getId() const override { return MapId(typeid(EDM).hash_code(), typeid(PANDA).hash_code(), label); }

  void add(EDMPtr const& edmRef, PANDA& suepObj) { fwdMap.emplace(edmRef, &suepObj); bwdMap.emplace(&suepObj, edmRef); }
};

//! ObjectMap for a single filler
class FillerObjectMap : public std::map<ObjectMapBase::MapId, ObjectMapBase*> {
 public:
  ~FillerObjectMap() { for (auto& m : *this) delete m.second; }

  void clearMaps() { for (auto& m : *this) m.second->clear(); }

  template<class EDM, class PANDA>
  ObjectMap<EDM, PANDA>& get(std::string label = "");

  template<class EDM, class PANDA>
  ObjectMap<EDM, PANDA> const& get(std::string label = "") const;
};

typedef std::map<std::string, FillerObjectMap> ObjectMapStore;

template<class EDM, class PANDA>
ObjectMap<EDM, PANDA>&
FillerObjectMap::get(std::string label/* = ""*/)
{
  ObjectMapBase::MapId id(typeid(EDM).hash_code(), typeid(PANDA).hash_code(), label);

  auto sItr(find(id));

  if (sItr == end())
    sItr = emplace(id, new ObjectMap<EDM, PANDA>).first;

  return static_cast<ObjectMap<EDM, PANDA>&>(*sItr->second);
}

template<class EDM, class PANDA>
ObjectMap<EDM, PANDA> const&
FillerObjectMap::get(std::string label/* = ""*/) const
{
  ObjectMapBase::MapId id(typeid(EDM).hash_code(), typeid(PANDA).hash_code(), label);
  return static_cast<ObjectMap<EDM, PANDA> const&>(*at(id));
}

#endif
