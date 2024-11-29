#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Utilities/make_tool.h"
#include "ConvolutionNetworkAlgo.h"
#include "ReconstructionAlgorithmBase.h"
#include "SignatureTools/SignatureToolBase.h"
#include "larreco/RecoAlg/SpacePointAlg.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include <memory>
#include <vector>
#include <map>

class SignatureReconstructionProducer : public art::EDProducer {
public:
    explicit SignatureReconstructionProducer(fhicl::ParameterSet const& pset);

    void produce(art::Event& evt) override;

private:
    std::string _SigProducer; 

    std::unique_ptr<ConvolutionNetworkAlgo> _cnn_algo;
    std::unique_ptr<reconstruction::ReconstructionAlgorithmBase> _reco_algo;
    std::unique_ptr<::signature::SignatureToolBase> _signature_tool;
    trkf::SpacePointAlg _space_point_alg;
};

SignatureReconstructionProducer::SignatureReconstructionProducer(fhicl::ParameterSet const& pset)
    : EDProducer{pset}
    , _cnn_algo(std::make_unique<ConvolutionNetworkAlgo>(pset.get<fhicl::ParameterSet>("ConvolutionNetworkAlgo")))
    , _reco_algo(art::make_tool<reconstruction::ReconstructionAlgorithmBase>(pset.get<fhicl::ParameterSet>("ReconstructionAlgorithm")))
    , _signature_tool(art::make_tool<::signature::SignatureToolBase>(pset.get<fhicl::ParameterSet>("SignatureTool")))
    , _space_point_alg(pset.get<fhicl::ParameterSet>("SpacePointAlg"))
    , _SigProducer{pset.get<std::string>("SigProducer", "signature")}
{
    produces<std::vector<recob::Track>>(_SigProducer);
    produces<std::vector<art::Ptr<recob::Hit>>>("HadronicHits").setModuleLabel(_SigProducer);
    produces<std::vector<art::Ptr<recob::Hit>>>("LeptonicHits").setModuleLabel(_SigProducer);
    produces<std::vector<recob::SpacePoint>>("HadronicSpacePoints").setModuleLabel(_SigProducer);
    produces<std::vector<recob::SpacePoint>>("LeptonicSpacePoints").setModuleLabel(_SigProducer);
    produces<art::Assns<recob::SpacePoint, recob::Hit>>("SpacePointHitAssns").setModuleLabel(_SigProducer);
}

void SignatureReconstructionProducer::produce(art::Event& evt) 
{
    std::vector<art::Ptr<recob::Hit>> hadronic_hits;
    std::vector<art::Ptr<recob::Hit>> leptonic_hits;

    auto const det_prop = art::ServiceHandle<detinfo::DetectorPropertiesService>()->DataFor(evt);
    auto const clock_data = art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(evt);

    std::map<int, std::vector<art::Ptr<recob::Hit>>> class_hits;
    _cnn_algo->infer(evt, class_hits);

    for (const auto& [hit_class, hits] : class_hits) {
        if (hit_class == 1) 
            hadronic_hits.insert(hadronic_hits.end(), hits.begin(), hits.end());
        else if (hit_class == 2) 
            leptonic_hits.insert(leptonic_hits.end(), hits.begin(), hits.end());
    }

    evt.put(std::make_unique<std::vector<art::Ptr<recob::Hit>>>(std::move(hadronic_hits)), "HadronicHits");
    evt.put(std::make_unique<std::vector<art::Ptr<recob::Hit>>>(std::move(leptonic_hits)), "LeptonicHits");

    std::vector<recob::SpacePoint> hadronic_space_points;
    std::vector<recob::SpacePoint> leptonic_space_points;
    auto sp_hit_assns = std::make_unique<art::Assns<recob::SpacePoint, recob::Hit>>();

    _space_point_alg.makeSpacePoints(clock_data, det_prop, hadronic_hits, hadronic_space_points);
    _space_point_alg.makeSpacePoints(clock_data, det_prop, leptonic_hits, leptonic_space_points);

    evt.put(std::make_unique<std::vector<recob::SpacePoint>>(std::move(hadronic_space_points)), "HadronicSpacePoints");
    evt.put(std::make_unique<std::vector<recob::SpacePoint>>(std::move(leptonic_space_points)), "LeptonicSpacePoints");
    evt.put(std::move(sp_hit_assns), "SpacePointHitAssns");

    std::vector<recob::Track> hadronic_tracks;
    _reco_algo->reconstruct(evt, leptonic_space_points, hadronic_space_points, hadronic_tracks);

    evt.put(std::make_unique<std::vector<recob::Track>>(std::move(hadronic_tracks)), _SigProducer);
}

DEFINE_ART_MODULE(SignatureReconstructionProducer)
