#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "larcore/Geometry/Geometry.h"
#include "TMath.h"
#include "ReconstructionAlgorithmBase.h"

#include <cmath>
#include <vector>

namespace reconstruction {

class ChargedKinkReconstruction : public ReconstructionAlgorithmBase {
public:
    explicit ChargedKinkReconstruction() = default;

    void configure(fhicl::ParameterSet const& pset) override {
        _angle_tolerance = pset.get<float>("AngleTolerance", 10.0); 
        _min_segment_length = pset.get<float>("MinSegmentLength", 5.0); 
    }

    void reconstruct(const art::Event& evt, 
                     const std::vector<art::Ptr<recob::Hit>>& hits, 
                     std::vector<recob::Track>& output_tracks) override {
        if (hits.size() < 3) return; 

        std::vector<TVector3> hit_positions;
        art::ServiceHandle<geo::Geometry> geo;

        for (const auto& hit : hits) {
            const auto& wire_id = hit->WireID();
            auto position = geo->WirePosition(wire_id);
            hit_positions.emplace_back(position[0], position[1], hit->PeakTime()); 
        }

        std::vector<TVector3> segment_directions;
        for (size_t i = 1; i < hit_positions.size(); ++i) {
            TVector3 direction = hit_positions[i] - hit_positions[i - 1];
            segment_directions.push_back(direction.Unit());
        }

        std::vector<size_t> segment_start_indices = {0};

        for (size_t i = 1; i < segment_directions.size(); ++i) {
            if (detectKink(segment_directions[i - 1], segment_directions[i])) {
                segment_start_indices.push_back(i);
            }
        }
        segment_start_indices.push_back(hits.size() - 1);

        for (size_t i = 0; i < segment_start_indices.size() - 1; ++i) {
            size_t start_idx = segment_start_indices[i];
            size_t end_idx = segment_start_indices[i + 1];

            float segment_length = computeSegmentLength(hit_positions[start_idx], hit_positions[end_idx]);
            if (segment_length < _min_segment_length) continue;

            std::vector<art::Ptr<recob::Hit>> segment_hits(hits.begin() + start_idx, hits.begin() + end_idx + 1);
            recob::Track track;
            output_tracks.push_back(track);
        }
    }

private:
    float _angle_tolerance;
    float _min_segment_length;

    bool detectKink(const TVector3& vec1, const TVector3& vec2) const {
        float dot_product = vec1.Dot(vec2);
        float angle = std::acos(dot_product) * TMath::RadToDeg();
        return angle > _angle_tolerance;
    }

    float computeSegmentLength(const TVector3& start, const TVector3& end) const {
        return (end - start).Mag();
    }
};

DEFINE_ART_CLASS_TOOL(ChargedKinkReconstruction)

} 
