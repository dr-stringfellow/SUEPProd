#ifndef SUEPProd_Auxiliary_getProduct_h
#define SUEPProd_Auxiliary_getProduct_h

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "DataFormats/Common/interface/Handle.h"

// Templated function for product retrieval
template<class C>
C const*
getProduct(edm::Event const& _event, edm::EDGetTokenT<C> const& _token, edm::Handle<C>* _handle = 0)
{
  edm::Handle<C> handle;

  edm::Handle<C>* handleP(0);
  if (_handle)
    handleP = _handle;
  else
    handleP = &handle;

  if (!_event.getByToken(_token, *handleP))
    throw cms::Exception("ProductNotFound") << typeid(C).name();

  return handleP->product();
}

#endif
