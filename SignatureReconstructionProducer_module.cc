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
    enum class HitType {
        Hadronic = 2,
        Leptonic = 3
    };

    struct ModelConfig {
        std::unique_ptr<ConvolutionNetworkAlgo> cnn_algo;
        std::unique_ptr<reconstruction::ReconstructionAlgorithmBase> reco_algo;
        std::unique_ptr<::signature::SignatureToolBase> signature_tool;
    };

    std::vector<ModelConfig> _models;
    trkf::SpacePointAlg _space_point_alg;

    void loadModels(const fhicl::ParameterSet& pset);
};

SignatureReconstructionProducer::SignatureReconstructionProducer(fhicl::ParameterSet const& pset)
    : _space_point_alg(pset.get<fhicl::ParameterSet>("SpacePointAlg")) {
    this->loadModels(pset.get<fhicl::ParameterSet>("Models"));
    produces<std::vector<recob::Track>>();
    produces<std::vector<art::Ptr<recob::Hit>>>("HadronicHits");
    produces<std::vector<art::Ptr<recob::Hit>>>("LeptonicHits");
    produces<std::vector<recob::SpacePoint>>("HadronicSpacePoints");
    produces<std::vector<recob::SpacePoint>>("LeptonicSpacePoints");
    produces<std::vector<recob::SpacePoint>>("TrueHadronicSpacePoints");
    produces<std::vector<recob::SpacePoint>>("TrueLeptonicSpacePoints");
}

void SignatureReconstructionProducer::loadModels(const fhicl::ParameterSet& pset) {
    for (const auto& model_name : pset.get_pset_names()) {
        const auto model_pset = pset.get<fhicl::ParameterSet>(model_name);

        ModelConfig model;
        model.cnn_algo = std::make_unique<ConvolutionNetworkAlgo>(model_pset.get<fhicl::ParameterSet>("ConvolutionNetworkAlgo"));
        model.reco_algo = art::make_tool<reconstruction::ReconstructionAlgorithmBase>(model_pset.get<fhicl::ParameterSet>("ReconstructionAlgorithm"));
        model.signature_tool = art::make_tool<::signature::SignatureToolBase>(model_pset.get<fhicl::ParameterSet>("SignatureTool"));

        _models.push_back(std::move(model));
    }
}

void SignatureReconstructionProducer::produce(art::Event& evt) {
    std::vector<recob::Track> output_tracks;
    std::vector<art::Ptr<recob::Hit>> hadronic_hits;
    std::vector<art::Ptr<recob::Hit>> leptonic_hits;
    std::vector<recob::SpacePoint> hadronic_space_points;
    std::vector<recob::SpacePoint> leptonic_space_points;
    std::vector<recob::SpacePoint> true_hadronic_space_points;
    std::vector<recob::SpacePoint> true_leptonic_space_points;

    auto const detProp = art::ServiceHandle<detinfo::DetectorPropertiesService>()->DataFor(evt);
    auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(evt);

    for (auto& model : _models) {
        std::map<int, std::vector<art::Ptr<recob::Hit>>> classified_hits;
        model.cnn_algo->infer(evt, classified_hits);

        for (const auto& [hit_type, hits] : classified_hits) {
            if (hit_type == static_cast<int>(HitType::Hadronic)) {
                hadronic_hits.insert(hadronic_hits.end(), hits.begin(), hits.end());
            } else if (hit_type == static_cast<int>(HitType::Leptonic)) {
                leptonic_hits.insert(leptonic_hits.end(), hits.begin(), hits.end());
            }
        }
    }

    _space_point_alg.makeSpacePoints(clockData, detProp, hadronic_hits, hadronic_space_points);
    _space_point_alg.makeSpacePoints(clockData, detProp, leptonic_hits, leptonic_space_points);
    _space_point_alg.makeMCTruthSpacePoints(clockData, detProp, hadronic_hits, true_hadronic_space_points);
    _space_point_alg.makeMCTruthSpacePoints(clockData, detProp, leptonic_hits, true_leptonic_space_points);

    for (auto& model : _models) {
        model.reco_algo->reconstruct(evt, leptonic_space_points, hadronic_space_points, output_tracks);
    }

    evt.put(std::make_unique<std::vector<recob::Track>>(std::move(output_tracks)));
    evt.put(std::make_unique<std::vector<art::Ptr<recob::Hit>>>(std::move(hadronic_hits)), "HadronicHits");
    evt.put(std::make_unique<std::vector<art::Ptr<recob::Hit>>>(std::move(leptonic_hits)), "LeptonicHits");
    evt.put(std::make_unique<std::vector<recob::SpacePoint>>(std::move(hadronic_space_points)), "HadronicSpacePoints");
    evt.put(std::make_unique<std::vector<recob::SpacePoint>>(std::move(leptonic_space_points)), "LeptonicSpacePoints");
    evt.put(std::make_unique<std::vector<recob::SpacePoint>>(std::move(true_hadronic_space_points)), "TrueHadronicSpacePoints");
    evt.put(std::make_unique<std::vector<recob::SpacePoint>>(std::move(true_leptonic_space_points)), "TrueLeptonicSpacePoints");
}

DEFINE_ART_MODULE(SignatureReconstructionProducer)
