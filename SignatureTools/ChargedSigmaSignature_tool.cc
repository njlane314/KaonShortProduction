#ifndef SIGNATURE_CHARGEDSIGMA_CXX
#define SIGNATURE_CHARGEDSIGMA_CXX

#include <iostream>
#include "SignatureToolBase.h"

namespace signature {

class ChargedSigmaSignature : public SignatureToolBase 
{
    
public:
    explicit ChargedSigmaSignature(const fhicl::ParameterSet& pset)
    : _MCPproducer{pset.get<art::InputTag>("MCPproducer", "largeant")}
    {
        configure(pset); 
    }
    ~ChargedSigmaSignature() {}

    void configure(fhicl::ParameterSet const& pset) override
    {
        SignatureToolBase::configure(pset);
    }

protected:
    void findSignature(art::Event const& evt, Signature& signature, bool& signature_found) override;

private:
    art::InputTag _MCPproducer;  
};

void ChargedSigmaSignature::findSignature(art::Event const& evt, Signature& signature, bool& signature_found)
{
    auto const &mcp_h = evt.getValidHandle<std::vector<simb::MCParticle>>(_MCPproducer);
    std::map<int, art::Ptr<simb::MCParticle>> mcp_map;
    for (size_t mcp_i = 0; mcp_i < mcp_h->size(); mcp_i++) {
        const art::Ptr<simb::MCParticle> mcp(mcp_h, mcp_i);
        mcp_map[mcp->TrackId()] = mcp;
    }

    for (const auto &mcp : *mcp_h) 
    {
        /*if (mcp.PdgCode() == 3112 && mcp.Process() == "primary" && !signature_found)
        {
            const art::Ptr<simb::MCParticle> sigma = mcp_map.at(mcp.TrackId());
            while (sigma->EndProcess() != "Decay")
            {
                if (!this->assessParticle(sigma))
                    break;

                this->fillSignature(sigma, signature);
                auto scat_dtrs = common::GetDaughters(mcp_map.at(sigma->TrackId()), mcp_map);
                if (scat_dtrs.empty()) 
                    break;

                scat_dtrs.erase(std::remove_if(scat_dtrs.begin(), scat_dtrs.end(), [](const auto& dtr) {
                    return dtr->PdgCode() != 3112;
                }), dtrs.end());

                sigma = dtrs.at(0);
            }

            if (sigma->EndProcess() == "Decay")
            {
                auto decay_dtrs = common::GetDaughters(mcp_map.at(sigma->TrackId()), mcp_map);
                std::vector<int> expected_dtrs = std::vector<int>{2112, -211}; // sigma+ -> n + pi-
                std::vector<int> found_dtrs;
                for (const auto &elem : decay_dtrs) 
                    found_dtrs.push_back(elem->PdgCode());

                std::sort(expected_dtrs.begin(), expected_dtrs.end());
                std::sort(found_dtrs.begin(), found_dtrs.end());

                if (found_dtrs == expected_dtrs) 
                {   
                    
            }
        }
    }*/


        if (std::abs(mcp.PdgCode()) == 3112 && mcp.Process() == "primary" && mcp.EndProcess() == "Decay"  && !signature_found) 
        {
            auto dtrs = common::GetDaughters(mcp_map.at(mcp.TrackId()), mcp_map);
            dtrs.erase(std::remove_if(dtrs.begin(), dtrs.end(), [](const auto& dtr) {
                return dtr->Process() != "Decay";
            }), dtrs.end());

            std::vector<int> expected_dtrs = std::vector<int>{2112, -211}; // Sigma- -> N + Pi-

            std::vector<int> found_dtrs;
            for (const auto &dtr : dtrs) 
                found_dtrs.push_back(dtr->PdgCode());

            std::sort(expected_dtrs.begin(), expected_dtrs.end());
            std::sort(found_dtrs.begin(), found_dtrs.end());

            if (found_dtrs == expected_dtrs) 
            {   
                bool all_pass = std::all_of(dtrs.begin(), dtrs.end(), [&](const auto& dtr) {
                    return this->assessParticle(*dtr);
                });

                if (all_pass) 
                {
                    signature_found = true;

                    this->fillSignature(mcp_map[mcp.TrackId()], signature);
                    for (const auto &dtr : dtrs) 
                    {
                        const TParticlePDG* info = TDatabasePDG::Instance()->GetParticle(dtr->PdgCode());
                        if (info->Charge() != 0.0) 
                            this->fillSignature(dtr, signature);
                    }

                    break;
                }
            }
        }
    }
}

DEFINE_ART_CLASS_TOOL(ChargedSigmaSignature)

} 

#endif