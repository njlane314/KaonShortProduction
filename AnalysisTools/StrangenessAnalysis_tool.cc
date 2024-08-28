#ifndef ANALYSIS_STRANGENESS_CXX
#define ANALYSIS_STRANGENESS_CXX

#include <iostream>
#include "AnalysisToolBase.h"

#include "TDatabasePDG.h"
#include "TParticlePDG.h"

#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/MCBase/MCShower.h"
#include "TVector3.h"

#include "../CommonFuncs/BacktrackingFuncs.h"
#include "../CommonFuncs/TrackShowerScoreFuncs.h"
#include "../CommonFuncs/SpaceChargeCorrections.h"
#include "../CommonFuncs/Scatters.h"
#include "../CommonFuncs/Geometry.h"

#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

namespace analysis
{

class StrangenessAnalysis : public AnalysisToolBase
{

public:
  
    StrangenessAnalysis(const fhicl::ParameterSet &pset);

    ~StrangenessAnalysis(){};

    void configure(fhicl::ParameterSet const &pset);

    void analyzeEvent(art::Event const &e, bool fData) override;

    void analyzeSlice(art::Event const &e, std::vector<common::ProxyPfpElem_t> &slice_pfp_v, bool fData, bool selected) override;

    void setBranches(TTree *_tree) override;

    void resetTTree(TTree *_tree) override;

private:

    art::InputTag fMCTproducer;
    art::InputTag fMCRproducer;
    art::InputTag fMCPproducer;
    art::InputTag fHproducer;
    art::InputTag fBacktrackTag;
    art::InputTag fPFPproducer;
    art::InputTag fCLSproducer; 
    art::InputTag fSLCproducer;
    art::InputTag fTRKproducer;
    art::InputTag fVTXproducer;
    art::InputTag fPCAproducer;
    art::InputTag fSHRproducer;

    TParticlePDG *neutral_kaon = TDatabasePDG::Instance()->GetParticle(311);
    TParticlePDG *kaon_short = TDatabasePDG::Instance()->GetParticle(310);
    TParticlePDG *kaon_long = TDatabasePDG::Instance()->GetParticle(130);
    TParticlePDG *lambda = TDatabasePDG::Instance()->GetParticle(3122);
    TParticlePDG *sigma_plus = TDatabasePDG::Instance()->GetParticle(3222); 
    TParticlePDG *sigma_minus = TDatabasePDG::Instance()->GetParticle(3112);
    TParticlePDG *sigma_zero = TDatabasePDG::Instance()->GetParticle(3212);

    unsigned int _mc_piplus_n_elas;
    unsigned int _mc_piplus_n_inelas;
    unsigned int _mc_piminus_n_elas;
    unsigned int _mc_piminus_n_inelas;

    int _mc_kshrt_piplus_tid; 
    int _mc_kshrt_piminus_tid;

    int _mc_kshrt_piplus_pdg;
    float _mc_kshrt_piplus_energy;
    float _mc_kshrt_piplus_px, _mc_kshrt_piplus_py, _mc_kshrt_piplus_pz;

    int _mc_kshrt_piminus_pdg;
    float _mc_kshrt_piminus_energy;
    float _mc_kshrt_piminus_px, _mc_kshrt_piminus_py, _mc_kshrt_piminus_pz;

    float _mc_kshrt_total_energy;

    float _mc_neutrino_vertex_x, _mc_neutrino_vertex_y, _mc_neutrino_vertex_z;
    float _mc_kaon_decay_x, _mc_kaon_decay_y, _mc_kaon_decay_z;
    float _mc_kaon_decay_distance;

    float _mc_piplus_impact_param;
    float _mc_piminus_impact_param;

    float _mc_piplus_phi;
    float _mc_piminus_phi;

    std::string _mc_piplus_endprocess;
    std::string _mc_piminus_endprocess;

    bool _mc_is_kshort_decay_pionic;

    bool _mc_has_lambda;
    bool _mc_has_sigma_minus;
    bool _mc_has_sigma_plus;
    bool _mc_has_sigma_zero;

    std::vector<float> _pfp_trk_sep_v;
    std::vector<float> _pfp_trk_phi_v;
    std::vector<float> _pfp_trk_d_v;
};

StrangenessAnalysis::StrangenessAnalysis(const fhicl::ParameterSet &pset)
{
    fPFPproducer = pset.get<art::InputTag>("PFPproducer");
    fCLSproducer = pset.get<art::InputTag>("CLSproducer");
    fSLCproducer = pset.get<art::InputTag>("SLCproducer");
    fTRKproducer = pset.get<art::InputTag>("TRKproducer");
    fVTXproducer = pset.get<art::InputTag>("VTXproducer");
    fPCAproducer = pset.get<art::InputTag>("PCAproducer");
    fSHRproducer = pset.get<art::InputTag>("SHRproducer");
    fMCTproducer = pset.get<art::InputTag>("MCTproducer", "");
    fMCRproducer = pset.get<art::InputTag>("MCRproducer", "");
    fMCPproducer = pset.get<art::InputTag>("MCPproducer", "");
    fHproducer = pset.get<art::InputTag>("Hproducer", "");
    fBacktrackTag = pset.get<art::InputTag>("BacktrackTag", ""); 
}

void StrangenessAnalysis::configure(fhicl::ParameterSet const &pset)
{
}

void StrangenessAnalysis::analyzeEvent(art::Event const &e, bool fData)
{
    if (fData) return;

    std::cout << "[StrangenessAnalysis] Analysing event..." << std::endl;
  
    // Load generator truth 
    auto const &mct_h = e.getValidHandle<std::vector<simb::MCTruth>>(fMCTproducer);

    // Load transportation truth
    auto const &mcp_h = e.getValidHandle<std::vector<simb::MCParticle>>(fMCRproducer);

    std::map<int, art::Ptr<simb::MCParticle>> mcp_map;
    for (size_t d = 0; d < mcp_h->size(); d++)
    {
        const art::Ptr<simb::MCParticle> mcp(mcp_h, d);
        mcp_map[mcp->TrackId()] = mcp;
    }
    
    std::cout << "[StrangenessAnalysis] Number of truth events " << mct_h->size() << std::endl;

    auto mct = mct_h->at(0);
    auto neutrino = mct.GetNeutrino();
    auto nu = neutrino.Nu();

    _mc_neutrino_vertex_x = nu.Vx();
    _mc_neutrino_vertex_y = nu.Vy();
    _mc_neutrino_vertex_z = nu.Vz();

    TVector3 neutrino_vertex(_mc_neutrino_vertex_x, _mc_neutrino_vertex_y, _mc_neutrino_vertex_z);

    _mc_is_kshort_decay_pionic = false;
    for (size_t i = 0; i < mcp_h->size(); i++)
    {
        auto const &t_part = mcp_h->at(i);

        if (abs(t_part.PdgCode()) == lambda->PdgCode() && t_part.Process() == "primary")
            _mc_has_lambda = true;

        if (abs(t_part.PdgCode()) == sigma_plus->PdgCode() && t_part.Process() == "primary")
            _mc_has_sigma_plus = true;

        if (abs(t_part.PdgCode()) == sigma_zero->PdgCode() && t_part.Process() == "primary")
            _mc_has_sigma_zero = true;

        if (abs(t_part.PdgCode()) == sigma_minus->PdgCode() && t_part.Process() == "primary")
            _mc_has_sigma_minus = true;

        // Look for K0 at the generator level
        if (abs(t_part.PdgCode()) == neutral_kaon->PdgCode() && t_part.Process() == "primary" && t_part.EndProcess() == "Decay" && t_part.NumberDaughters() == 1 && !_mc_is_kshort_decay_pionic) 
        {
            std::vector<art::Ptr<simb::MCParticle>> dtrs = common::GetDaughters(mcp_map.at(t_part.TrackId()), mcp_map);
            if (dtrs.size() != 1) 
                continue; 

            auto g_part = dtrs.at(0);

            if (g_part->PdgCode() == kaon_short->PdgCode() && g_part->Process() == "Decay" && g_part->EndProcess() == "Decay" && g_part->NumberDaughters() == 2 && !_mc_is_kshort_decay_pionic)
            {
                auto daughters = common::GetDaughters(mcp_map.at(g_part->TrackId()), mcp_map);
                if (daughters.size() == 2) 
                {
                    std::vector<int> exp_dtrs = {-211, 211};
                    std::vector<int> fnd_dtrs;

                    for (const auto &dtr : daughters) 
                    {
                        fnd_dtrs.push_back(dtr->PdgCode());
                    }

                    std::sort(exp_dtrs.begin(), exp_dtrs.end());
                    std::sort(fnd_dtrs.begin(), fnd_dtrs.end());

                    if (fnd_dtrs == exp_dtrs) 
                    {
                        TVector3 kaon_decay(g_part->EndX(), g_part->EndY(), g_part->EndZ());
                        float decay_length = (kaon_decay - neutrino_vertex).Mag();

                        if (decay_length > 50) continue;

                        _mc_kshrt_total_energy = g_part->E();

                        _mc_kaon_decay_x = g_part->EndX();
                        _mc_kaon_decay_y = g_part->EndY();
                        _mc_kaon_decay_z = g_part->EndZ();

                        std::cout << "[StrangenessAnalysis] The kaon decay was found at " 
                                    << _mc_kaon_decay_x << ", "
                                    << _mc_kaon_decay_y << ", " 
                                    << _mc_kaon_decay_z << std::endl;
                        
                        float phi_h = std::atan2(kaon_decay.Y(), kaon_decay.X());

                        std::cout << "[StrangenessAnalysis] The separation is "
                                    << decay_length << std::endl;

                        _mc_kaon_decay_distance = decay_length;

                        for (const auto &dtr : daughters) 
                        {
                            std::cout << dtr->PdgCode() << std::endl;
                            TVector3 pion_mom(dtr->Px(), dtr->Py(), dtr->Pz());
                            float phi_i = std::atan2(pion_mom.Y(), pion_mom.X());
                            float d_0 = decay_length * std::sin(phi_i - phi_h);

                            std::cout << "Mom " << pion_mom.X() << " " << pion_mom.Y() << std::endl;
                            std::cout << "Phi " << phi_i << std::endl;

                            unsigned int n_elas = 0;
                            unsigned int n_inelas = 0;

                            art::Ptr<simb::MCParticle> scat_part;
                            std::string scat_end_process;

                            common::GetNScatters(mcp_h, dtr, scat_part, n_elas, n_inelas);
                            scat_end_process = common::GetEndState(dtr, mcp_h);
                            std::cout << scat_end_process << std::endl;

                            if (dtr->PdgCode() == 211) // pion-plus
                            { 
                                _mc_kshrt_piplus_tid = dtr->TrackId();
                                _mc_kshrt_piplus_pdg = dtr->PdgCode();
                                _mc_kshrt_piplus_energy = dtr->E();
                                _mc_kshrt_piplus_px = dtr->Px();
                                _mc_kshrt_piplus_py = dtr->Py();
                                _mc_kshrt_piplus_pz = dtr->Pz();
                                _mc_piplus_phi = phi_i;
                                _mc_piplus_impact_param = d_0;
                                _mc_piplus_n_elas = n_elas;
                                _mc_piplus_n_inelas = n_inelas;
                                _mc_piplus_endprocess = scat_end_process;
                            } 
                            else if (dtr->PdgCode() == -211) // pion-minus
                            { 
                                _mc_kshrt_piminus_tid = dtr->TrackId();
                                _mc_kshrt_piminus_pdg = dtr->PdgCode();
                                _mc_kshrt_piminus_energy = dtr->E();
                                _mc_kshrt_piminus_px = dtr->Px();
                                _mc_kshrt_piminus_py = dtr->Py();
                                _mc_kshrt_piminus_pz = dtr->Pz();
                                _mc_piminus_phi = phi_i;
                                _mc_piminus_impact_param = d_0;
                                _mc_piminus_n_elas = n_elas;
                                _mc_piminus_n_inelas = n_inelas;
                                _mc_piminus_endprocess = scat_end_process;

                                std::cout << _mc_piminus_phi << std::endl;
                            }
                        }

                        _mc_is_kshort_decay_pionic = true;
                        std::cout << "Found pionic kaon short decay!" << std::endl;
                    }
                }
            }
        }

        if (_mc_is_kshort_decay_pionic) break;  
    }
}

void StrangenessAnalysis::analyzeSlice(art::Event const &e, std::vector<common::ProxyPfpElem_t> &slice_pfp_v, bool fData, bool selected)
{
    common::ProxyPfpColl_t const &pfp_proxy = proxy::getCollection<std::vector<recob::PFParticle>>(e, fPFPproducer,
                                                        proxy::withAssociated<larpandoraobj::PFParticleMetadata>(fPFPproducer),
                                                        proxy::withAssociated<recob::Cluster>(fCLSproducer),
                                                        proxy::withAssociated<recob::Slice>(fSLCproducer),
                                                        proxy::withAssociated<recob::Track>(fTRKproducer),
                                                        proxy::withAssociated<recob::Vertex>(fVTXproducer),
                                                        proxy::withAssociated<recob::PCAxis>(fPCAproducer),
                                                        proxy::withAssociated<recob::Shower>(fSHRproducer),
                                                        proxy::withAssociated<recob::SpacePoint>(fPFPproducer));

    for (const common::ProxyPfpElem_t &pfp_pxy : pfp_proxy)
    {
        double reco_nu_vtx_sce_x; 
        double reco_nu_vtx_sce_y;
        double reco_nu_vtx_sce_z;

        if (pfp_pxy->IsPrimary()) 
        {
            double xyz[3] = {};

            auto vtx = pfp_pxy.get<recob::Vertex>();
            if (vtx.size() == 1)
            {
                vtx.at(0)->XYZ(xyz);
                auto nuvtx = TVector3(xyz[0], xyz[1], xyz[2]);

                double reco_nu_vtx_x = nuvtx.X();
                double reco_nu_vtx_y = nuvtx.Y();
                double reco_nu_vtx_z = nuvtx.Z();

                float reco_nu_vtx_sce[3];
                common::ApplySCECorrectionXYZ(reco_nu_vtx_x, reco_nu_vtx_y, reco_nu_vtx_z, reco_nu_vtx_sce);
                reco_nu_vtx_sce_x = reco_nu_vtx_sce[0];
                reco_nu_vtx_sce_y = reco_nu_vtx_sce[1];
                reco_nu_vtx_sce_z = reco_nu_vtx_sce[2];
            }
        }
        
        auto trk_v = pfp_pxy.get<recob::Track>();

        if (trk_v.size() == 1) 
        {
            auto trk = trk_v.at(0);

            auto trk_strt = trk->Start();
            float trk_strt_sce[3];
            common::ApplySCECorrectionXYZ(trk_strt.X(), trk_strt.Y(), trk_strt.Z(), trk_strt_sce);
            float phi_h = std::atan2(trk_strt_sce[0], trk_strt_sce[1]);

            auto trk_end = trk->End();
            float trk_end_sce[3];
            common::ApplySCECorrectionXYZ(trk_end.X(), trk_end.Y(), trk_end.Z(), trk_end_sce);

            float trk_sep = common::distance3d(reco_nu_vtx_sce_x, reco_nu_vtx_sce_y, reco_nu_vtx_sce_z, trk_end_sce[0], trk_end_sce[1], trk_end_sce[2]);
            float trk_phi = std::atan2(trk_end_sce[0], trk_end_sce[1]);
            float trk_d = trk_sep * sin(trk_phi - phi_h);

            _pfp_trk_sep_v.push_back(trk_sep); 
            _pfp_trk_phi_v.push_back(trk_phi);
            _pfp_trk_d_v.push_back(trk_d);
        }
        else 
        {
            _pfp_trk_sep_v.push_back(std::numeric_limits<float>::lowest()); 
            _pfp_trk_phi_v.push_back(std::numeric_limits<float>::lowest());
            _pfp_trk_d_v.push_back(std::numeric_limits<float>::lowest());
        }
    }

    return;
}


void StrangenessAnalysis::setBranches(TTree *_tree)
{
    _tree->Branch("mc_piplus_tid", &_mc_kshrt_piplus_tid, "mc_piplus_tid/i");
    _tree->Branch("mc_piminus_tid", &_mc_kshrt_piminus_tid, "mc_piminus_tid/i");

    _tree->Branch("mc_piplus_n_elas", &_mc_piplus_n_elas, "mc_piplus_n_elas/i");
    _tree->Branch("mc_piplus_n_inelas", &_mc_piplus_n_inelas, "mc_piplus_n_inelas/i");
    _tree->Branch("mc_piminus_n_elas", &_mc_piminus_n_elas, "mc_piminus_n_elas/i");
    _tree->Branch("mc_piminus_n_inelas", &_mc_piminus_n_inelas, "mc_piminus_n_inelas/i");

    _tree->Branch("mc_kshrt_piplus_pdg", &_mc_kshrt_piplus_pdg, "mc_kshrt_piplus_pdg/I");
    _tree->Branch("mc_kshrt_piplus_energy", &_mc_kshrt_piplus_energy, "mc_kshrt_piplus_energy/F");
    _tree->Branch("mc_kshrt_piplus_px", &_mc_kshrt_piplus_px, "mc_kshrt_piplus_px/F");
    _tree->Branch("mc_kshrt_piplus_py", &_mc_kshrt_piplus_py, "mc_kshrt_piplus_py/F");
    _tree->Branch("mc_kshrt_piplus_pz", &_mc_kshrt_piplus_pz, "mc_kshrt_piplus_pz/F");

    _tree->Branch("mc_kshrt_piminus_pdg", &_mc_kshrt_piminus_pdg, "mc_kshrt_piminus_pdg/I");
    _tree->Branch("mc_kshrt_piminus_energy", &_mc_kshrt_piminus_energy, "mc_kshrt_piminus_energy/F");
    _tree->Branch("mc_kshrt_piminus_px", &_mc_kshrt_piminus_px, "mc_kshrt_piminus_px/F");
    _tree->Branch("mc_kshrt_piminus_py", &_mc_kshrt_piminus_py, "mc_kshrt_piminus_py/F");
    _tree->Branch("mc_kshrt_piminus_pz", &_mc_kshrt_piminus_pz, "mc_kshrt_piminus_pz/F");

    _tree->Branch("mc_kshrt_total_energy", &_mc_kshrt_total_energy, "mc_kshrt_total_energy/F");

    _tree->Branch("mc_neutrino_vertex_x", &_mc_neutrino_vertex_x, "mc_neutrino_vertex_x/F");
    _tree->Branch("mc_neutrino_vertex_y", &_mc_neutrino_vertex_y, "mc_neutrino_vertex_y/F");
    _tree->Branch("mc_neutrino_vertex_z", &_mc_neutrino_vertex_z, "mc_neutrino_vertex_z/F");

    _tree->Branch("mc_kaon_decay_x", &_mc_kaon_decay_x, "mc_kaon_decay_x/F");
    _tree->Branch("mc_kaon_decay_y", &_mc_kaon_decay_y, "mc_kaon_decay_y/F");
    _tree->Branch("mc_kaon_decay_z", &_mc_kaon_decay_z, "mc_kaon_decay_z/F");
    _tree->Branch("mc_kaon_decay_distance", &_mc_kaon_decay_distance, "mc_kaon_decay_distance/F");

    _tree->Branch("mc_piplus_impact_param", &_mc_piplus_impact_param, "mc_piplus_impact_param/F");
    _tree->Branch("mc_piminus_impact_param", &_mc_piminus_impact_param, "mc_piminus_impact_param/F");

    _tree->Branch("mc_piplus_phi", &_mc_piplus_phi, "mc_piplus_phi/F");
    _tree->Branch("mc_piminus_phi", &_mc_piminus_phi, "mc_piminus_phi/F");

    _tree->Branch("mc_piplus_endprocess", &_mc_piplus_endprocess); 
    _tree->Branch("mc_piminus_endprocess", &_mc_piminus_endprocess); 

    _tree->Branch("mc_is_kshort_decay_pionic", &_mc_is_kshort_decay_pionic);

    _tree->Branch("mc_has_lambda", &_mc_has_lambda);
    _tree->Branch("mc_has_sigma_plus", &_mc_has_sigma_plus);
    _tree->Branch("mc_has_sigma_minus", &_mc_has_sigma_minus);
    _tree->Branch("mc_has_sigma_zero", &_mc_has_sigma_zero);

    _tree->Branch("all_pfp_trk_sep", &_pfp_trk_sep_v);
    _tree->Branch("all_pfp_trk_d", &_pfp_trk_d_v);
    _tree->Branch("all_pfp_trk_phi", &_pfp_trk_phi_v);
}

void StrangenessAnalysis::resetTTree(TTree *_tree)
{
    _mc_piplus_n_elas = 0;
    _mc_piplus_n_inelas = 0;
    _mc_piminus_n_elas = 0;
    _mc_piminus_n_inelas = 0;

    _mc_kshrt_piplus_pdg = 0;
    _mc_kshrt_piplus_energy = std::numeric_limits<float>::lowest();
    _mc_kshrt_piplus_px = std::numeric_limits<float>::lowest();
    _mc_kshrt_piplus_py = std::numeric_limits<float>::lowest();
    _mc_kshrt_piplus_pz = std::numeric_limits<float>::lowest();

    _mc_kshrt_piminus_pdg = 0;
    _mc_kshrt_piminus_energy = std::numeric_limits<float>::lowest();
    _mc_kshrt_piminus_px = std::numeric_limits<float>::lowest();
    _mc_kshrt_piminus_py = std::numeric_limits<float>::lowest();
    _mc_kshrt_piminus_pz = std::numeric_limits<float>::lowest();

    _mc_kshrt_total_energy = std::numeric_limits<float>::lowest();

    _mc_neutrino_vertex_x = std::numeric_limits<float>::lowest();
    _mc_neutrino_vertex_y = std::numeric_limits<float>::lowest();
    _mc_neutrino_vertex_z = std::numeric_limits<float>::lowest();

    _mc_kaon_decay_x = std::numeric_limits<float>::lowest();
    _mc_kaon_decay_y = std::numeric_limits<float>::lowest();
    _mc_kaon_decay_z = std::numeric_limits<float>::lowest();
    _mc_kaon_decay_distance = std::numeric_limits<float>::lowest();

    _mc_piplus_impact_param = std::numeric_limits<float>::lowest();
    _mc_piminus_impact_param = std::numeric_limits<float>::lowest();

    _mc_piplus_phi = std::numeric_limits<float>::lowest();
    _mc_piminus_phi = std::numeric_limits<float>::lowest();

    _mc_piplus_endprocess = ""; 
    _mc_piminus_endprocess = "";

    _mc_kshrt_piminus_tid = -1;
    _mc_kshrt_piplus_tid = -1;

    _mc_is_kshort_decay_pionic = false;

    _mc_has_lambda = false;
    _mc_has_sigma_plus = false;
    _mc_has_sigma_minus = false;
    _mc_has_sigma_zero = false;

    _pfp_trk_d_v.clear();
    _pfp_trk_phi_v.clear();
    _pfp_trk_sep_v.clear();
}

DEFINE_ART_CLASS_TOOL(StrangenessAnalysis)
} 

#endif