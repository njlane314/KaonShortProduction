#ifndef RECONSTRUCTIONTOOLBASE_H
#define RECONSTRUCTIONTOOLBASE_H

#include "art/Framework/Principal/Event.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "lardataobj/RecoBase/Track.h"
#include "SignatureTools/SignatureToolBase.h"

#include <vector>

namespace reconstruction {

class ReconstructionToolBase 
{
public:
    virtual ~ReconstructionToolBase() = default;

    virtual void configure(fhicl::ParameterSet const& pset) = 0;

    virtual void reconstruct(const art::Event& evt, const signature::SignatureCollection& signatures, std::vector<recob::Track>& output_tracks) = 0;
};

} 

#endif
